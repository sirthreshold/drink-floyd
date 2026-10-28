#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <windows.h>
#include <mmsystem.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "../Source/AudioTypes.h"
#include "../Source/AudioAnalysisEngine.h"
#include "../Source/AudioCaptureEngine.h"

int main()
{
    juce::ScopedJuceInitialiser_GUI guiInit;

    std::cout << "========================================================\n";
    std::cout << "DRINK FLOYD - LIVE WASAPI SYSTEM LOOPBACK PLAYBACK TEST\n";
    std::cout << "========================================================\n\n";

    drinkfloyd::AudioAnalysisEngine analysis;
    drinkfloyd::AudioCaptureEngine capture(analysis);

    if (!capture.start())
    {
        std::cerr << "Failed to start WASAPI loopback capture!\n";
        return 1;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    std::cout << "Default Output Device: " << capture.getActiveDeviceName().toStdString() << "\n";
    std::cout << "Initial Status: " << drinkfloyd::getStatusString(capture.getCurrentStatus()).toStdString() << "\n\n";

    // 1. Initial baseline (silence before playback)
    auto snapBefore = analysis.getSnapshot();
    std::cout << "[T = 0.0s] Baseline before playback:\n";
    std::cout << "  Status: " << snapBefore.statusMessage.toStdString() << "\n";
    std::cout << "  Levels -> Bass: " << snapBefore.bands.bass 
              << " | Low-Mid: " << snapBefore.bands.lowMid 
              << " | Mid: " << snapBefore.bands.mid 
              << " | High: " << snapBefore.bands.high 
              << " | Beat: " << snapBefore.bands.beat << "\n\n";

    // 2. Start Windows System Audio Playback
    std::cout << "Playing 'test_bass_kick.wav' through Windows default render endpoint...\n";
    PlaySoundA("test_bass_kick.wav", NULL, SND_ASYNC | SND_FILENAME);

    // 3. Monitor live WASAPI loopback captured bands across several intervals
    float maxBassObserved = 0.0f;
    float maxHighObserved = 0.0f;
    float maxBeatObserved = 0.0f;

    for (int i = 1; i <= 10; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        auto snap = analysis.getSnapshot();

        if (snap.bands.bass > maxBassObserved) maxBassObserved = snap.bands.bass;
        if (snap.bands.high > maxHighObserved) maxHighObserved = snap.bands.high;
        if (snap.bands.beat > maxBeatObserved) maxBeatObserved = snap.bands.beat;

        std::cout << "[T = " << (i * 0.25f) << "s] "
                  << "Status: " << snap.statusMessage.toStdString()
                  << " | Bass: " << snap.bands.bass
                  << " | LowMid: " << snap.bands.lowMid
                  << " | Mid: " << snap.bands.mid
                  << " | High: " << snap.bands.high
                  << " | Beat: " << snap.bands.beat << "\n";
    }

    // Stop playback
    PlaySoundA(NULL, NULL, 0);
    std::cout << "\nPlayback finished. Waiting for decay...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    auto snapAfter = analysis.getSnapshot();
    std::cout << "[T = 3.5s] After playback stopped:\n";
    std::cout << "  Status: " << snapAfter.statusMessage.toStdString() << "\n";
    std::cout << "  Levels -> Bass: " << snapAfter.bands.bass 
              << " | Low-Mid: " << snapAfter.bands.lowMid 
              << " | Mid: " << snapAfter.bands.mid 
              << " | High: " << snapAfter.bands.high << "\n\n";

    capture.stop();

    std::cout << "========================================================\n";
    std::cout << "SUMMARY OF LIVE SYSTEM AUDIO OBSERVATIONS:\n";
    std::cout << "  - Peak Bass Band Level: " << maxBassObserved << "\n";
    std::cout << "  - Peak High Band Level: " << maxHighObserved << "\n";
    std::cout << "  - Peak Beat Onset Level: " << maxBeatObserved << "\n";
    std::cout << "  - Reactivity Verified: " << (maxBassObserved > 0.30f ? "YES (Strong Bass Spike)" : "LOW") << "\n";
    std::cout << "========================================================\n";

    return (maxBassObserved > 0.20f) ? 0 : 1;
}
