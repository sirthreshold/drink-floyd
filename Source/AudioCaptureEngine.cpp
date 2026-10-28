#include "AudioCaptureEngine.h"

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <ksmedia.h>
#include <vector>
#include <chrono>

namespace drinkfloyd
{

// ==============================================================================
// IMMNotificationClient implementation for runtime default device switching
// ==============================================================================
class AudioNotificationClient : public IMMNotificationClient
{
public:
    explicit AudioNotificationClient(AudioCaptureEngine& owner)
        : engine(owner), refCount(1) {}

    virtual ~AudioNotificationClient() = default;

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override
    {
        if (ppv == nullptr)
            return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IMMNotificationClient))
        {
            *ppv = static_cast<IMMNotificationClient*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() override
    {
        return InterlockedIncrement(&refCount);
    }

    STDMETHODIMP_(ULONG) Release() override
    {
        ULONG ul = InterlockedDecrement(&refCount);
        if (ul == 0)
            delete this;
        return ul;
    }

    // IMMNotificationClient
    STDMETHODIMP OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId) override
    {
        juce::ignoreUnused(pwstrDefaultDeviceId);
        if (flow == eRender && (role == eConsole || role == eMultimedia))
        {
            engine.signalDeviceChanged();
        }
        return S_OK;
    }

    STDMETHODIMP OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) override
    {
        juce::ignoreUnused(pwstrDeviceId, dwNewState);
        return S_OK;
    }

    STDMETHODIMP OnDeviceAdded(LPCWSTR pwstrDeviceId) override
    {
        juce::ignoreUnused(pwstrDeviceId);
        return S_OK;
    }

    STDMETHODIMP OnDeviceRemoved(LPCWSTR pwstrDeviceId) override
    {
        juce::ignoreUnused(pwstrDeviceId);
        return S_OK;
    }

    STDMETHODIMP OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key) override
    {
        juce::ignoreUnused(pwstrDeviceId, key);
        return S_OK;
    }

private:
    AudioCaptureEngine& engine;
    LONG refCount;
};

// ==============================================================================
// AudioCaptureEngine Implementation
// ==============================================================================
AudioCaptureEngine::AudioCaptureEngine(AudioAnalysisEngine& analysisEngine)
    : analysis(analysisEngine)
{
}

AudioCaptureEngine::~AudioCaptureEngine()
{
    stop();
}

bool AudioCaptureEngine::start()
{
    if (isCapturing.load())
        return true;

    shouldStop.store(false);
    deviceChangePending.store(false);

    workerThread = std::thread(&AudioCaptureEngine::captureLoop, this);
    return true;
}

void AudioCaptureEngine::stop()
{
    shouldStop.store(true);
    if (workerThread.joinable())
    {
        workerThread.join();
    }
    isCapturing.store(false);
}

void AudioCaptureEngine::signalDeviceChanged()
{
    deviceChangePending.store(true);
    analysis.reportStatus(AudioCaptureStatus::DeviceChanged, {}, "Default audio output changed. Re-attaching...");
}

juce::String AudioCaptureEngine::getActiveDeviceName() const
{
    std::lock_guard<std::mutex> lock(nameMutex);
    return currentDeviceName;
}

AudioCaptureStatus AudioCaptureEngine::getCurrentStatus() const
{
    return analysis.getSnapshot().status;
}

void AudioCaptureEngine::releaseDeviceResources()
{
    if (pAudioClient != nullptr)
    {
        pAudioClient->Stop();
    }

    if (pCaptureClient != nullptr)
    {
        pCaptureClient->Release();
        pCaptureClient = nullptr;
    }

    if (pAudioClient != nullptr)
    {
        pAudioClient->Release();
        pAudioClient = nullptr;
    }

    if (pDevice != nullptr)
    {
        pDevice->Release();
        pDevice = nullptr;
    }
}

bool AudioCaptureEngine::initializeDevice()
{
    releaseDeviceResources();

    if (pEnumerator == nullptr)
    {
        HRESULT hr = CoCreateInstance(
            __uuidof(MMDeviceEnumerator),
            nullptr,
            CLSCTX_ALL,
            IID_PPV_ARGS(&pEnumerator)
        );

        if (FAILED(hr) || pEnumerator == nullptr)
        {
            analysis.reportStatus(AudioCaptureStatus::Error, {}, "Failed to create MMDeviceEnumerator");
            return false;
        }

        // Register default device notification client
        if (pNotifyClient == nullptr)
        {
            pNotifyClient = new AudioNotificationClient(*this);
            pEnumerator->RegisterEndpointNotificationCallback(pNotifyClient);
        }
    }

    // 1. Get default render endpoint (the active playback device)
    HRESULT hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr) || pDevice == nullptr)
    {
        analysis.reportStatus(AudioCaptureStatus::Error, {}, "No default audio render endpoint available");
        return false;
    }

    // Query friendly device name
    juce::String queriedName = "Default Windows Render Output";
    IPropertyStore* pStore = nullptr;
    if (SUCCEEDED(pDevice->OpenPropertyStore(STGM_READ, &pStore)) && pStore != nullptr)
    {
        PROPVARIANT varName;
        PropVariantInit(&varName);
        if (SUCCEEDED(pStore->GetValue(PKEY_Device_FriendlyName, &varName)))
        {
            if (varName.vt == VT_LPWSTR && varName.pwszVal != nullptr)
            {
                queriedName = juce::String(varName.pwszVal);
            }
            PropVariantClear(&varName);
        }
        pStore->Release();
    }

    {
        std::lock_guard<std::mutex> lock(nameMutex);
        currentDeviceName = queriedName;
    }

    // 2. Activate IAudioClient
    hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&pAudioClient));
    if (FAILED(hr) || pAudioClient == nullptr)
    {
        analysis.reportStatus(AudioCaptureStatus::Error, queriedName, "Failed to activate IAudioClient on render endpoint");
        return false;
    }

    // 3. Query native mix format
    WAVEFORMATEX* pwfx = nullptr;
    hr = pAudioClient->GetMixFormat(&pwfx);
    if (FAILED(hr) || pwfx == nullptr)
    {
        analysis.reportStatus(AudioCaptureStatus::Error, queriedName, "Failed to query mix format");
        return false;
    }

    sampleRate = static_cast<double>(pwfx->nSamplesPerSec);
    channels = static_cast<int>(pwfx->nChannels);
    bitsPerSample = static_cast<int>(pwfx->wBitsPerSample);

    isFloat = false;
    if (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
    {
        isFloat = true;
    }
    else if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        const auto* pExt = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(pwfx);
        if (pExt->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
            isFloat = true;
    }

    // 4. Initialize WASAPI client in loopback mode (AUDCLNT_STREAMFLAGS_LOOPBACK)
    // 200ms buffer size
    constexpr REFERENCE_TIME hnsBufferDuration = 10000000 / 5;
    hr = pAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK,
        hnsBufferDuration,
        0,
        pwfx,
        nullptr
    );

    CoTaskMemFree(pwfx);

    if (FAILED(hr))
    {
        analysis.reportStatus(AudioCaptureStatus::Error, queriedName, "WASAPI Loopback initialization failed (hr=" + juce::String::toHexString((int)hr) + ")");
        return false;
    }

    // 5. Get IAudioCaptureClient service
    hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&pCaptureClient));
    if (FAILED(hr) || pCaptureClient == nullptr)
    {
        analysis.reportStatus(AudioCaptureStatus::Error, queriedName, "Failed to acquire IAudioCaptureClient");
        return false;
    }

    // 6. Start audio capture stream
    hr = pAudioClient->Start();
    if (FAILED(hr))
    {
        analysis.reportStatus(AudioCaptureStatus::Error, queriedName, "Failed to start audio client");
        return false;
    }

    analysis.reportStatus(AudioCaptureStatus::Listening, queriedName, "Listening");
    return true;
}

void AudioCaptureEngine::captureLoop()
{
    HRESULT hrCo = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    isCapturing.store(true);

    if (!initializeDevice())
    {
        // Failed initial attach, will retry in loop
    }

    std::vector<float> monoBuffer;
    monoBuffer.reserve(4096);

    auto lastPacketTime = std::chrono::steady_clock::now();

    while (!shouldStop.load())
    {
        // 1. Check if device change requested
        if (deviceChangePending.load())
        {
            deviceChangePending.store(false);
            analysis.reportStatus(AudioCaptureStatus::DeviceChanged, getActiveDeviceName(), "Switching default audio device...");
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            if (!initializeDevice())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
        }

        if (pCaptureClient == nullptr || pAudioClient == nullptr)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            if (!initializeDevice())
                continue;
        }

        // 2. Poll packets from WASAPI loopback
        UINT32 packetLength = 0;
        HRESULT hr = pCaptureClient->GetNextPacketSize(&packetLength);

        if (FAILED(hr))
        {
            if (hr == AUDCLNT_E_DEVICE_INVALIDATED || hr == AUDCLNT_E_RESOURCES_INVALIDATED)
            {
                // Device invalidated (e.g. headphones unplugged, rate changed)
                signalDeviceChanged();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (packetLength == 0)
        {
            // When no music is playing through the render device in Windows,
            // loopback packet length stays 0.
            auto now = std::chrono::steady_clock::now();
            auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPacketTime).count();

            if (elapsedMs > 100)
            {
                // Push synthetic silence chunk so envelopes and meters decay to zero
                std::vector<float> silence(512, 0.0f);
                analysis.pushAudioBlock(silence.data(), static_cast<int>(silence.size()), sampleRate);
                lastPacketTime = now;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        // 3. Read packet data
        while (packetLength > 0 && !shouldStop.load())
        {
            BYTE* pData = nullptr;
            UINT32 numFramesAvailable = 0;
            DWORD flags = 0;

            hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr);
            if (FAILED(hr))
            {
                if (hr == AUDCLNT_E_DEVICE_INVALIDATED || hr == AUDCLNT_E_RESOURCES_INVALIDATED)
                {
                    signalDeviceChanged();
                }
                break;
            }

            lastPacketTime = std::chrono::steady_clock::now();

            if (numFramesAvailable > 0)
            {
                if (monoBuffer.size() < numFramesAvailable)
                    monoBuffer.resize(numFramesAvailable);

                if (flags & AUDCLNT_BUFFERFLAGS_SILENT || pData == nullptr)
                {
                    std::fill(monoBuffer.begin(), monoBuffer.begin() + numFramesAvailable, 0.0f);
                }
                else
                {
                    // Convert incoming interleaved audio to mono normalized float [-1.0, 1.0]
                    const int ch = std::max(1, channels);

                    if (isFloat && bitsPerSample == 32)
                    {
                        const float* src = reinterpret_cast<const float*>(pData);
                        for (UINT32 i = 0; i < numFramesAvailable; ++i)
                        {
                            float sum = 0.0f;
                            for (int c = 0; c < ch; ++c)
                                sum += src[i * ch + c];
                            monoBuffer[i] = sum / static_cast<float>(ch);
                        }
                    }
                    else if (!isFloat && bitsPerSample == 16)
                    {
                        const int16_t* src = reinterpret_cast<const int16_t*>(pData);
                        for (UINT32 i = 0; i < numFramesAvailable; ++i)
                        {
                            float sum = 0.0f;
                            for (int c = 0; c < ch; ++c)
                                sum += static_cast<float>(src[i * ch + c]) / 32768.0f;
                            monoBuffer[i] = sum / static_cast<float>(ch);
                        }
                    }
                    else if (!isFloat && bitsPerSample == 24)
                    {
                        const uint8_t* src = pData;
                        for (UINT32 i = 0; i < numFramesAvailable; ++i)
                        {
                            float sum = 0.0f;
                            for (int c = 0; c < ch; ++c)
                            {
                                const int idx = (i * ch + c) * 3;
                                const int32_t val = (static_cast<int32_t>(src[idx]) << 8) |
                                                    (static_cast<int32_t>(src[idx + 1]) << 16) |
                                                    (static_cast<int32_t>(src[idx + 2]) << 24);
                                sum += static_cast<float>(val) / 2147483648.0f;
                            }
                            monoBuffer[i] = sum / static_cast<float>(ch);
                        }
                    }
                    else if (!isFloat && bitsPerSample == 32)
                    {
                        const int32_t* src = reinterpret_cast<const int32_t*>(pData);
                        for (UINT32 i = 0; i < numFramesAvailable; ++i)
                        {
                            float sum = 0.0f;
                            for (int c = 0; c < ch; ++c)
                                sum += static_cast<float>(src[i * ch + c]) / 2147483648.0f;
                            monoBuffer[i] = sum / static_cast<float>(ch);
                        }
                    }
                    else
                    {
                        // Fallback generic float assumption
                        const float* src = reinterpret_cast<const float*>(pData);
                        for (UINT32 i = 0; i < numFramesAvailable; ++i)
                        {
                            monoBuffer[i] = src[i * ch];
                        }
                    }
                }

                // Push samples into analysis engine
                analysis.pushAudioBlock(monoBuffer.data(), static_cast<int>(numFramesAvailable), sampleRate);
            }

            pCaptureClient->ReleaseBuffer(numFramesAvailable);

            hr = pCaptureClient->GetNextPacketSize(&packetLength);
            if (FAILED(hr))
                break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    // Teardown
    releaseDeviceResources();

    if (pEnumerator != nullptr)
    {
        if (pNotifyClient != nullptr)
        {
            pEnumerator->UnregisterEndpointNotificationCallback(pNotifyClient);
            pNotifyClient->Release();
            pNotifyClient = nullptr;
        }
        pEnumerator->Release();
        pEnumerator = nullptr;
    }

    if (SUCCEEDED(hrCo))
    {
        CoUninitialize();
    }

    isCapturing.store(false);
}

} // namespace drinkfloyd
