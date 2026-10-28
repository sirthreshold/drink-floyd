#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

namespace drinkfloyd
{

enum class AudioCaptureStatus
{
    Initializing,
    Listening,
    NoAudio,
    DeviceChanged,
    Error
};

inline juce::String getStatusString(AudioCaptureStatus status)
{
    switch (status)
    {
        case AudioCaptureStatus::Initializing:  return "Initializing...";
        case AudioCaptureStatus::Listening:     return "Listening";
        case AudioCaptureStatus::NoAudio:       return "No audio detected";
        case AudioCaptureStatus::DeviceChanged: return "Device changed";
        case AudioCaptureStatus::Error:         return "Audio Error";
    }
    return "Unknown";
}

struct FrequencyBands
{
    float bass = 0.0f;     // ~20 Hz - 250 Hz
    float lowMid = 0.0f;   // ~250 Hz - 500 Hz
    float mid = 0.0f;      // ~500 Hz - 2000 Hz
    float high = 0.0f;     // ~2000 Hz - 16000+ Hz
    float beat = 0.0f;     // Beat / onset transient energy (0.0 to 1.0)
    float rawRms = 0.0f;   // Overall broad RMS level
};

struct AudioAnalysisSnapshot
{
    AudioCaptureStatus status = AudioCaptureStatus::Initializing;
    juce::String deviceName = "Detecting default audio output...";
    juce::String statusMessage = "Initializing...";
    FrequencyBands bands;
    float inputGain = 1.0f;
    float react = 1.0f;
};

} // namespace drinkfloyd
