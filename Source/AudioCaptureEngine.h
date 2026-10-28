#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <thread>
#include <atomic>
#include <memory>
#include <string>
#include "AudioAnalysisEngine.h"

// Forward declare COM interfaces to keep header clean
struct IMMDeviceEnumerator;
struct IMMDevice;
struct IMMNotificationClient;
struct IAudioClient;
struct IAudioCaptureClient;

namespace drinkfloyd
{

class AudioCaptureEngine
{
public:
    explicit AudioCaptureEngine(AudioAnalysisEngine& analysisEngine);
    ~AudioCaptureEngine();

    // Start background WASAPI loopback capture
    bool start();

    // Stop background capture cleanly
    void stop();

    bool isRunning() const noexcept { return isCapturing.load(); }
    juce::String getActiveDeviceName() const;
    AudioCaptureStatus getCurrentStatus() const;

    // Called internally or by notification client when default device changes
    void signalDeviceChanged();

private:
    void captureLoop();
    bool initializeDevice();
    void releaseDeviceResources();

    AudioAnalysisEngine& analysis;

    std::thread workerThread;
    std::atomic<bool> shouldStop { false };
    std::atomic<bool> isCapturing { false };
    std::atomic<bool> deviceChangePending { false };

    // COM Objects & State
    IMMDeviceEnumerator* pEnumerator = nullptr;
    IMMNotificationClient* pNotifyClient = nullptr;
    IMMDevice* pDevice = nullptr;
    IAudioClient* pAudioClient = nullptr;
    IAudioCaptureClient* pCaptureClient = nullptr;

    double sampleRate = 48000.0;
    int channels = 2;
    int bitsPerSample = 32;
    bool isFloat = true;

    mutable std::mutex nameMutex;
    juce::String currentDeviceName = "Default Windows Render Output";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioCaptureEngine)
};

} // namespace drinkfloyd
