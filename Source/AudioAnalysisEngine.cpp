#include "AudioAnalysisEngine.h"
#include <algorithm>
#include <cmath>

namespace drinkfloyd
{

AudioAnalysisEngine::AudioAnalysisEngine()
    : ringBuffer(FFT_SIZE * 2, 0.0f),
      window(FFT_SIZE, 0.0f),
      fftReal(FFT_SIZE, 0.0f),
      fftImag(FFT_SIZE, 0.0f),
      magnitudes(FFT_SIZE / 2, 0.0f)
{
    // Precompute Hann window
    constexpr double twoPi = 6.28318530717958647692;
    for (int i = 0; i < FFT_SIZE; ++i)
    {
        window[i] = static_cast<float>(0.5 * (1.0 - std::cos(twoPi * i / (FFT_SIZE - 1))));
    }

    reset();
}

void AudioAnalysisEngine::reset()
{
    std::lock_guard<std::mutex> lock(snapshotMutex);
    std::fill(ringBuffer.begin(), ringBuffer.end(), 0.0f);
    writeIndex = 0;
    samplesSinceLastHop = 0;
    envBass = 0.0f;
    envLowMid = 0.0f;
    envMid = 0.0f;
    envHigh = 0.0f;
    envRawRms = 0.0f;
    prevBassEnergy = 0.0f;
    prevLowMidEnergy = 0.0f;
    prevMidEnergy = 0.0f;
    prevHighEnergy = 0.0f;
    avgBassFlux = 0.02f;
    avgSnareFlux = 0.02f;
    avgHiHatFlux = 0.02f;
    currentBeat = 0.0f;
    silentHopsCount = 0;

    latestSnapshot.bands = FrequencyBands{};
}

void AudioAnalysisEngine::reportStatus(AudioCaptureStatus status, const juce::String& deviceName, const juce::String& message)
{
    std::lock_guard<std::mutex> lock(snapshotMutex);
    latestSnapshot.status = status;
    if (!deviceName.isEmpty())
        latestSnapshot.deviceName = deviceName;
    if (!message.isEmpty())
        latestSnapshot.statusMessage = message;
    else
        latestSnapshot.statusMessage = getStatusString(status);
}

void AudioAnalysisEngine::pushAudioBlock(const float* monoSamples, int numSamples, double sampleRate)
{
    if (monoSamples == nullptr || numSamples <= 0)
        return;

    currentSampleRate = (sampleRate > 1000.0) ? sampleRate : 48000.0;
    const int bufferSize = static_cast<int>(ringBuffer.size());

    // Check if block has sound
    float blockRms = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        blockRms += monoSamples[i] * monoSamples[i];
    }
    blockRms = std::sqrt(blockRms / static_cast<float>(numSamples));

    for (int i = 0; i < numSamples; ++i)
    {
        ringBuffer[writeIndex] = monoSamples[i];
        writeIndex = (writeIndex + 1) % bufferSize;
        samplesSinceLastHop++;

        if (samplesSinceLastHop >= HOP_SIZE)
        {
            samplesSinceLastHop = 0;
            processHop(currentSampleRate);
        }
    }
}

void AudioAnalysisEngine::computeFFT(float* real, float* imag, int n)
{
    // In-place Cooley-Tukey Radix-2 Decimation-in-Time FFT
    // 1. Bit-reversal permutation
    int j = 0;
    for (int i = 0; i < n - 1; ++i)
    {
        if (i < j)
        {
            std::swap(real[i], real[j]);
            std::swap(imag[i], imag[j]);
        }
        int k = n >> 1;
        while (k <= j)
        {
            j -= k;
            k >>= 1;
        }
        j += k;
    }

    // 2. Butterfly computations
    constexpr double pi = 3.14159265358979323846;
    for (int len = 2; len <= n; len <<= 1)
    {
        const double angle = -2.0 * pi / len;
        const double wlenReal = std::cos(angle);
        const double wlenImag = std::sin(angle);

        for (int i = 0; i < n; i += len)
        {
            double wReal = 1.0;
            double wImag = 0.0;

            for (int k = 0; k < len / 2; ++k)
            {
                const int uIdx = i + k;
                const int vIdx = i + k + len / 2;

                const double uReal = real[uIdx];
                const double uImag = imag[uIdx];
                const double vReal = real[vIdx] * wReal - imag[vIdx] * wImag;
                const double vImag = real[vIdx] * wImag + imag[vIdx] * wReal;

                real[uIdx] = static_cast<float>(uReal + vReal);
                imag[uIdx] = static_cast<float>(uImag + vImag);
                real[vIdx] = static_cast<float>(uReal - vReal);
                imag[vIdx] = static_cast<float>(uImag - vImag);

                const double nextWReal = wReal * wlenReal - wImag * wlenImag;
                const double nextWImag = wReal * wlenImag + wImag * wlenReal;
                wReal = nextWReal;
                wImag = nextWImag;
            }
        }
    }
}

void AudioAnalysisEngine::processHop(double sampleRate)
{
    const int bufferSize = static_cast<int>(ringBuffer.size());
    const float gain = inputGain.load(std::memory_order_relaxed);
    const float react = reactAmount.load(std::memory_order_relaxed);
    const float attMs = attackMs.load(std::memory_order_relaxed);
    const float relMs = releaseMs.load(std::memory_order_relaxed);

    // Extract the most recent FFT_SIZE samples with windowing
    float sumSq = 0.0f;
    int readIdx = (writeIndex - FFT_SIZE + bufferSize) % bufferSize;
    for (int i = 0; i < FFT_SIZE; ++i)
    {
        const float sample = ringBuffer[readIdx];
        readIdx = (readIdx + 1) % bufferSize;

        sumSq += sample * sample;
        fftReal[i] = sample * window[i];
        fftImag[i] = 0.0f;
    }

    const float rms = std::sqrt(sumSq / static_cast<float>(FFT_SIZE));

    // Silence detection threshold (~ -65 dBFS)
    constexpr float silenceThreshold = 0.0005f;
    bool isSilent = (rms < silenceThreshold);
    if (isSilent)
        silentHopsCount++;
    else
        silentHopsCount = 0;

    // Run FFT
    computeFFT(fftReal.data(), fftImag.data(), FFT_SIZE);

    // Compute magnitude spectrum for positive frequencies [0, FFT_SIZE / 2)
    const float norm = 2.0f / static_cast<float>(FFT_SIZE);
    const int numBins = FFT_SIZE / 2;
    for (int k = 0; k < numBins; ++k)
    {
        magnitudes[k] = std::sqrt(fftReal[k] * fftReal[k] + fftImag[k] * fftImag[k]) * norm;
    }

    // Map frequency bands using actual bin frequencies: freq = k * sampleRate / FFT_SIZE
    const double binWidth = sampleRate / static_cast<double>(FFT_SIZE);

    auto getBandEnergy = [&](double lowFreq, double highFreq) -> float {
        int startBin = std::clamp(static_cast<int>(std::floor(lowFreq / binWidth)), 1, numBins - 1);
        int endBin   = std::clamp(static_cast<int>(std::ceil(highFreq / binWidth)), startBin + 1, numBins);

        float sum = 0.0f;
        for (int k = startBin; k < endBin; ++k)
            sum += magnitudes[k] * magnitudes[k];

        return std::sqrt(sum / static_cast<float>(endBin - startBin));
    };

    // Frequency boundaries as specified:
    // Bass: 20 Hz - 250 Hz
    // Low-Mid: 250 Hz - 500 Hz
    // Mid: 500 Hz - 2000 Hz
    // High: 2000 Hz - 16000 Hz
    const float rawBass   = getBandEnergy(20.0, 250.0);
    const float rawLowMid = getBandEnergy(250.0, 500.0);
    const float rawMid    = getBandEnergy(500.0, 2000.0);
    const float rawHigh   = getBandEnergy(2000.0, 16000.0);

    // Apply input gain with soft saturation: y = std::tanh(1.5 * x * gain)
    // Extra boost factor on musical high/mid spectrum for balanced visual response
    auto scaleLevel = [gain](float energy, float spectralTilt) -> float {
        const float scaled = energy * gain * spectralTilt;
        return std::tanh(2.5f * scaled);
    };

    const float targetBass   = scaleLevel(rawBass, 3.4f);
    const float targetLowMid = scaleLevel(rawLowMid, 5.5f);
    const float targetMid    = scaleLevel(rawMid, 8.5f);
    const float targetHigh   = scaleLevel(rawHigh, 12.0f);
    const float targetRms    = std::clamp(rms * gain * 3.0f, 0.0f, 1.0f);

    // Calculate attack / release coefficients based on hop duration
    const float dtSec = static_cast<float>(HOP_SIZE) / static_cast<float>(sampleRate);
    const float attCoeff = 1.0f - std::exp(-dtSec / std::max(0.001f, attMs * 0.001f));
    const float relCoeff = 1.0f - std::exp(-dtSec / std::max(0.005f, relMs * 0.001f));

    auto updateEnvelope = [attCoeff, relCoeff](float target, float current) -> float {
        if (target > current)
            return current + attCoeff * (target - current);
        else
            return current + relCoeff * (target - current);
    };

    envBass   = updateEnvelope(targetBass, envBass);
    envLowMid = updateEnvelope(targetLowMid, envLowMid);
    envMid    = updateEnvelope(targetMid, envMid);
    envHigh   = updateEnvelope(targetHigh, envHigh);
    envRawRms = updateEnvelope(targetRms, envRawRms);

    // If long period of silence (> 50 hops, ~250ms), decay smoothly to near-zero
    if (silentHopsCount > 50)
    {
        envBass   *= 0.90f;
        envLowMid *= 0.90f;
        envMid    *= 0.90f;
        envHigh   *= 0.90f;
        envRawRms *= 0.90f;
    }

    // Instantaneous Transient & Multi-Genre Beat Onset Detection
    // 1. Time-domain jump on the current hop (0-5ms zero-latency onset)
    float recentHopSq = 0.0f;
    for (int i = 0; i < HOP_SIZE; ++i)
    {
        int idx = (writeIndex - HOP_SIZE + i + bufferSize) % bufferSize;
        float s = ringBuffer[idx];
        recentHopSq += s * s;
    }
    const float instantHopRms = std::sqrt(recentHopSq / static_cast<float>(HOP_SIZE));
    const float instantFlux = std::max(0.0f, instantHopRms * gain * 3.5f - envRawRms);

    // 2. Multi-band spectral flux:
    // - Bass flux: kicks, 808 transient attacks (House, Techno, Hip Hop kicks)
    // - Snare / Clap flux: mid & low-mid sharp spikes (Hip Hop, Trap, R&B, Rock, Pop snares/rims/claps)
    // - Hi-Hat / Cymbals flux: high frequencies (Trap rolling hats, shakers, cymbals)
    const float bassFlux   = std::max(0.0f, targetBass - prevBassEnergy);
    const float lowMidFlux = std::max(0.0f, targetLowMid - prevLowMidEnergy);
    const float midFlux    = std::max(0.0f, targetMid - prevMidEnergy);
    const float highFlux   = std::max(0.0f, targetHigh - prevHighEnergy);

    prevBassEnergy   = targetBass;
    prevLowMidEnergy = targetLowMid;
    prevMidEnergy    = targetMid;
    prevHighEnergy   = targetHigh;

    avgBassFlux  = 0.90f * avgBassFlux  + 0.10f * bassFlux;
    avgSnareFlux = 0.90f * avgSnareFlux + 0.10f * (midFlux + lowMidFlux * 0.70f);
    avgHiHatFlux = 0.90f * avgHiHatFlux + 0.10f * highFlux;

    const bool bassOnset   = (bassFlux > (avgBassFlux * 1.35f + 0.022f)) && (targetBass > 0.08f);
    const bool snareOnset  = ((midFlux + lowMidFlux * 0.70f) > (avgSnareFlux * 1.30f + 0.024f)) && (targetMid > 0.08f || targetLowMid > 0.09f);
    const bool hiHatOnset  = (highFlux > (avgHiHatFlux * 1.60f + 0.035f)) && (targetHigh > 0.12f);
    const bool broadOnset  = (instantFlux > 0.075f) && (targetBass > 0.07f || targetMid > 0.08f);
    const bool sustained808 = (targetBass > 0.50f);

    if (bassOnset || snareOnset || hiHatOnset || broadOnset)
    {
        float beatVelocity = 1.0f;
        if (snareOnset && !bassOnset)
            beatVelocity = 0.92f;
        else if (hiHatOnset && !bassOnset && !snareOnset)
            beatVelocity = 0.65f;

        currentBeat = std::max(currentBeat, beatVelocity);
    }
    else if (sustained808)
    {
        currentBeat = std::max(currentBeat, targetBass * 0.60f);
    }
    else
    {
        currentBeat *= 0.86f; // rapid snappy beat decay
    }

    // Multiply by react slider sensitivity
    FrequencyBands outBands;
    outBands.bass   = std::clamp(envBass * react, 0.0f, 1.0f);
    outBands.lowMid = std::clamp(envLowMid * react, 0.0f, 1.0f);
    outBands.mid    = std::clamp(envMid * react, 0.0f, 1.0f);
    outBands.high   = std::clamp(envHigh * react, 0.0f, 1.0f);
    outBands.beat   = std::clamp(currentBeat * react, 0.0f, 1.0f);
    outBands.rawRms = std::clamp(envRawRms * react, 0.0f, 1.0f);

    // Atomic / guarded snapshot update
    {
        std::lock_guard<std::mutex> lock(snapshotMutex);
        latestSnapshot.bands = outBands;
        latestSnapshot.inputGain = gain;
        latestSnapshot.react = react;

        // Auto-update status message if audio is detected vs silence
        if (latestSnapshot.status != AudioCaptureStatus::Error &&
            latestSnapshot.status != AudioCaptureStatus::DeviceChanged)
        {
            if (silentHopsCount > 60)
            {
                latestSnapshot.status = AudioCaptureStatus::NoAudio;
                latestSnapshot.statusMessage = "No audio detected";
            }
            else
            {
                latestSnapshot.status = AudioCaptureStatus::Listening;
                latestSnapshot.statusMessage = "Listening";
            }
        }
    }
}

AudioAnalysisSnapshot AudioAnalysisEngine::getSnapshot()
{
    std::lock_guard<std::mutex> lock(snapshotMutex);
    return latestSnapshot;
}

} // namespace drinkfloyd
