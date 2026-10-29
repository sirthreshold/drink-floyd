#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <vector>
#include <mutex>
#include <atomic>
#include <cmath>
#include "AudioTypes.h"

namespace drinkfloyd
{

class AudioAnalysisEngine
{
public:
    static constexpr int FFT_SIZE = 2048;
    static constexpr int HOP_SIZE = 256; // 5.3ms at 48kHz for instantaneous transient response

    AudioAnalysisEngine();
    ~AudioAnalysisEngine() = default;

    // Called from the audio capture thread
    void pushAudioBlock(const float* monoSamples, int numSamples, double sampleRate);
    void reportStatus(AudioCaptureStatus status, const juce::String& deviceName, const juce::String& message = {});

    // Parameter controls wired from UI
    void setInputGain(float gain) noexcept { inputGain.store(gain, std::memory_order_relaxed); }
    void setReact(float r) noexcept        { reactAmount.store(r, std::memory_order_relaxed); }
    void setAttackMs(float ms) noexcept    { attackMs.store(ms, std::memory_order_relaxed); }
    void setReleaseMs(float ms) noexcept   { releaseMs.store(ms, std::memory_order_relaxed); }

    float getInputGain() const noexcept    { return inputGain.load(std::memory_order_relaxed); }
    float getReact() const noexcept        { return reactAmount.load(std::memory_order_relaxed); }

    // Snapshot query for UI & OpenGL rendering (lock-free thread-safe copy)
    AudioAnalysisSnapshot getSnapshot();

    // Reset internal filters / buffers (e.g. on device change)
    void reset();

private:
    void processHop(double sampleRate);
    void computeFFT(float* real, float* imag, int n);

    // Incoming sample ring buffer
    std::vector<float> ringBuffer;
    int writeIndex = 0;
    int samplesSinceLastHop = 0;
    double currentSampleRate = 48000.0;

    // FFT working buffers
    std::vector<float> window;
    std::vector<float> fftReal;
    std::vector<float> fftImag;
    std::vector<float> magnitudes;

    // Envelope followers for frequency bands
    float envBass = 0.0f;
    float envLowMid = 0.0f;
    float envMid = 0.0f;
    float envHigh = 0.0f;
    float envRawRms = 0.0f;

    // Beat / onset detection state
    float prevBassEnergy = 0.0f;
    float avgBassFlux = 0.02f;
    float currentBeat = 0.0f;

    // Parameters (atomic)
    std::atomic<float> inputGain { 1.0f };
    std::atomic<float> reactAmount { 1.0f };
    std::atomic<float> attackMs { 6.0f };
    std::atomic<float> releaseMs { 160.0f };

    // Status & Snapshot state
    std::mutex snapshotMutex;
    AudioAnalysisSnapshot latestSnapshot;

    // Silence detection
    int silentHopsCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioAnalysisEngine)
};

} // namespace drinkfloyd
