#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <juce_gui_extra/juce_gui_extra.h>
#include "../Source/AudioTypes.h"
#include "../Source/AudioAnalysisEngine.h"
#include "../Source/AudioCaptureEngine.h"

int main()
{
    juce::ScopedJuceInitialiser_GUI guiInit;

    std::cout << "========================================\n";
    std::cout << "DRINK FLOYD - PHASE 2 AUDIO ENGINE TESTS\n";
    std::cout << "========================================\n\n";

    int testsPassed = 0;
    int totalTests = 0;

    auto runTest = [&](const std::string& name, bool condition) {
        totalTests++;
        std::cout << "[" << (condition ? "PASS" : "FAIL") << "] " << name << "\n";
        if (condition) testsPassed++;
    };

    drinkfloyd::AudioAnalysisEngine analysis;
    analysis.setInputGain(1.0f);
    analysis.setReact(1.0f);

    constexpr double sampleRate = 48000.0;
    constexpr double twoPi = 6.28318530717958647692;

    auto feedSine = [&](double freqHz, int numSamples, float amplitude = 0.8f) {
        std::vector<float> buffer(numSamples);
        for (int i = 0; i < numSamples; ++i)
        {
            buffer[i] = static_cast<float>(amplitude * std::sin(twoPi * freqHz * i / sampleRate));
        }
        analysis.pushAudioBlock(buffer.data(), numSamples, sampleRate);
    };

    auto feedSilence = [&](int numSamples) {
        std::vector<float> buffer(numSamples, 0.0f);
        analysis.pushAudioBlock(buffer.data(), numSamples, sampleRate);
    };

    // 1. Digital Silence Test -> near-zero band levels
    {
        analysis.reset();
        feedSilence(48000 / 2); // 0.5s of silence
        auto snap = analysis.getSnapshot();
        bool silent = (snap.bands.bass < 0.02f && snap.bands.lowMid < 0.02f &&
                       snap.bands.mid < 0.02f && snap.bands.high < 0.02f);
        runTest("Digital Silence Test (all bands < 0.02)", silent);
        std::cout << "       Bass: " << snap.bands.bass << ", LowMid: " << snap.bands.lowMid 
                  << ", Mid: " << snap.bands.mid << ", High: " << snap.bands.high << "\n";
    }

    // 2. Pure Bass Tone (60 Hz kick/sub-bass) -> Bass band spikes, High stays near zero
    {
        analysis.reset();
        feedSine(60.0, 48000 / 2, 0.9f); // 0.5s of 60 Hz
        auto snap = analysis.getSnapshot();
        bool bassIsolated = (snap.bands.bass > 0.50f && snap.bands.high < 0.08f);
        runTest("Bass Isolation (60 Hz: Bass > 0.50, High < 0.08)", bassIsolated);
        std::cout << "       Bass: " << snap.bands.bass << ", LowMid: " << snap.bands.lowMid 
                  << ", Mid: " << snap.bands.mid << ", High: " << snap.bands.high << "\n";
    }

    // 3. Pure Mid Tone (1000 Hz vocal/synth) -> Mid band spikes, Bass stays near zero
    {
        analysis.reset();
        feedSine(1000.0, 48000 / 2, 0.7f); // 0.5s of 1 kHz
        auto snap = analysis.getSnapshot();
        bool midIsolated = (snap.bands.mid > 0.40f && snap.bands.bass < 0.08f);
        runTest("Mid Isolation (1000 Hz: Mid > 0.40, Bass < 0.08)", midIsolated);
        std::cout << "       Bass: " << snap.bands.bass << ", LowMid: " << snap.bands.lowMid 
                  << ", Mid: " << snap.bands.mid << ", High: " << snap.bands.high << "\n";
    }

    // 4. Pure High Tone (8000 Hz cymbals/air) -> High band spikes, Bass stays near zero
    {
        analysis.reset();
        feedSine(8000.0, 48000 / 2, 0.7f); // 0.5s of 8 kHz
        auto snap = analysis.getSnapshot();
        bool highIsolated = (snap.bands.high > 0.35f && snap.bands.bass < 0.08f);
        runTest("High Isolation (8000 Hz: High > 0.35, Bass < 0.08)", highIsolated);
        std::cout << "       Bass: " << snap.bands.bass << ", LowMid: " << snap.bands.lowMid 
                  << ", Mid: " << snap.bands.mid << ", High: " << snap.bands.high << "\n";
    }

    // 5. Input Gain Slider Wiring Test
    {
        analysis.reset();
        analysis.setInputGain(0.2f);
        feedSine(60.0, 48000 / 4, 0.5f);
        auto snapLowGain = analysis.getSnapshot();

        analysis.reset();
        analysis.setInputGain(2.0f);
        feedSine(60.0, 48000 / 4, 0.5f);
        auto snapHighGain = analysis.getSnapshot();

        bool gainWorks = (snapHighGain.bands.bass > snapLowGain.bands.bass * 2.0f);
        runTest("Input Gain Slider Scaling (2.0x gain > 0.2x gain)", gainWorks);
        std::cout << "       Low gain (0.2x): " << snapLowGain.bands.bass 
                  << ", High gain (2.0x): " << snapHighGain.bands.bass << "\n";
    }

    // 6. React Slider Sensitivity Test
    {
        analysis.reset();
        analysis.setInputGain(1.0f);
        analysis.setReact(0.0f);
        feedSine(60.0, 48000 / 4, 0.8f);
        auto snapZeroReact = analysis.getSnapshot();

        analysis.setReact(1.5f);
        feedSine(60.0, 48000 / 4, 0.8f);
        auto snapHighReact = analysis.getSnapshot();

        bool reactWorks = (snapZeroReact.bands.bass == 0.0f && snapHighReact.bands.bass > 0.5f);
        runTest("React Slider Sensitivity (0.0 = zero output, 1.5 = high response)", reactWorks);
    }

    // 7. Onset / Beat Pulse Test
    {
        analysis.reset();
        analysis.setReact(1.0f);
        feedSilence(48000 / 4); // Silence pre-condition
        feedSine(60.0, 2048, 1.0f); // Sudden kick transient
        auto snapBeat = analysis.getSnapshot();
        bool beatTriggered = (snapBeat.bands.beat > 0.5f);
        runTest("Beat Onset Detection (Transient kick triggers beat)", beatTriggered);
        std::cout << "       Beat level on kick transient: " << snapBeat.bands.beat << "\n";
    }

    // 8. Native WASAPI Loopback Initialization & Active Endpoint Check
    {
        drinkfloyd::AudioCaptureEngine capture(analysis);
        bool started = capture.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        juce::String activeDev = capture.getActiveDeviceName();
        auto snap = analysis.getSnapshot();

        bool deviceDetected = started && (!activeDev.isEmpty()) && (snap.status != drinkfloyd::AudioCaptureStatus::Error);
        runTest("WASAPI Loopback Capture Default Endpoint", deviceDetected);
        std::cout << "       Active Output Device: " << activeDev.toStdString() << "\n";
        std::cout << "       Capture Status: " << snap.statusMessage.toStdString() << "\n";

        capture.stop();
    }

    std::cout << "\n----------------------------------------\n";
    std::cout << "Test Summary: " << testsPassed << "/" << totalTests << " passed\n";
    std::cout << "----------------------------------------\n";

    return (testsPassed == totalTests) ? 0 : 1;
}
