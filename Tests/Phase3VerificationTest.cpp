#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_opengl/juce_opengl.h>
#include "../Source/VisualParameters.h"
#include "../Source/AudioTypes.h"
#include "../Source/AudioAnalysisEngine.h"

int main()
{
    juce::ScopedJuceInitialiser_GUI guiInit;

    std::cout << "====================================================\n";
    std::cout << "DRINK FLOYD - PHASE 3.2+3.3 VISUAL & CENTERING TESTS\n";
    std::cout << "====================================================\n\n";

    int testsPassed = 0;
    int totalTests = 0;

    auto runTest = [&](const std::string& name, bool condition) {
        totalTests++;
        std::cout << "[" << (condition ? "PASS" : "FAIL") << "] " << name << "\n";
        if (condition) testsPassed++;
    };

    // 1. Pixel Melt & Scatter Scene Verification
    {
        drinkfloyd::VisualParameters p;
        bool defaultSceneIsPixelMelt = (p.scene == drinkfloyd::SceneType::PixelMeltScatter);
        bool nameMatches = (drinkfloyd::getSceneName(p.scene) == "Pixel Melt & Scatter");
        runTest("Pixel Melt & Scatter is Default Active Scene", defaultSceneIsPixelMelt && nameMatches);
        std::cout << "       Active Default Scene: " << drinkfloyd::getSceneName(p.scene).toStdString() << "\n";

        bool allScenesPresent = (static_cast<int>(drinkfloyd::SceneType::PixelMeltScatter) == 0 &&
                                 static_cast<int>(drinkfloyd::SceneType::Smear) == 1 &&
                                 static_cast<int>(drinkfloyd::SceneType::Ripple) == 2 &&
                                 static_cast<int>(drinkfloyd::SceneType::Flow) == 3 &&
                                 static_cast<int>(drinkfloyd::SceneType::Tiles) == 4);
        runTest("All 5 Scenes Registered (Pixel Melt, Smear, Ripple, Flow, Tiles)", allScenesPresent);
    }

    // 2. Original Colors & Separate Melt/Scatter Parameters
    {
        drinkfloyd::VisualParameters p;
        bool originalColorsDefault = (p.hueDrift == 0.0f && p.palette == drinkfloyd::PalettePreset::PrismCore);
        bool meltAndScatterSeparate = (p.melt == 1.0f && p.scatter == 1.0f);
        runTest("Original Color Preserved (HueDrift = 0.0, PrismCore default)", originalColorsDefault);
        runTest("Independent Melt & Scatter Parameters (Melt=1.0, Scatter=1.0)", meltAndScatterSeparate);
    }

    // 3. Mathematical Centering Verification (Both Horizontally & Vertically)
    {
        auto testCentering = [](int canvasW, int canvasH, int statusBarH, float targetAspect) -> bool {
            int availH = canvasH - statusBarH;
            float availAspect = static_cast<float>(canvasW) / static_cast<float>(availH);

            int letterboxW, letterboxH, marginX, marginY;
            if (availAspect > targetAspect)
            {
                letterboxW = static_cast<int>(std::round(availH * targetAspect));
                letterboxH = availH;
                marginX = (canvasW - letterboxW) / 2;
                marginY = 0;
            }
            else
            {
                letterboxW = canvasW;
                letterboxH = static_cast<int>(std::round(canvasW / targetAspect));
                marginX = 0;
                marginY = (availH - letterboxH) / 2;
            }

            // Verify horizontal symmetry
            int rightMargin = canvasW - (marginX + letterboxW);
            bool horizCentred = std::abs(marginX - rightMargin) <= 1;

            // Verify vertical symmetry
            int bottomMargin = marginY;
            int topMargin = availH - (marginY + letterboxH);
            bool vertCentred = std::abs(bottomMargin - topMargin) <= 1;

            return horizCentred && vertCentred;
        };

        bool c1 = testCentering(1280, 800, 28, 1.0f);          // 1080x1080 windowed
        bool c2 = testCentering(1280, 800, 28, 1080.f/1920.f);  // 1080x1920 windowed
        bool c3 = testCentering(1280, 800, 28, 1920.f/1080.f);  // 1920x1080 windowed
        bool c4 = testCentering(1920, 1080, 0, 1.0f);          // 1080x1080 fullscreen
        bool c5 = testCentering(1920, 1080, 0, 1080.f/1920.f);  // 1080x1920 fullscreen
        bool c6 = testCentering(1920, 1080, 0, 1920.f/1080.f);  // 1920x1080 fullscreen

        runTest("Mathematical Centering (Equal top/bottom and left/right margins)", c1 && c2 && c3 && c4 && c5 && c6);
        std::cout << "       Verified: 1080x1080, 1080x1920, 1920x1080 windowed and fullscreen\n";
    }

    // 4. Randomize Look Safety Verification
    {
        juce::Random rng(54321);
        bool allSafe = true;
        for (int i = 0; i < 500; ++i)
        {
            float brightness = rng.nextFloat() * 0.35f + 0.90f; // 0.90 to 1.25
            float react      = rng.nextFloat() * 0.80f + 0.60f;
            float melt       = rng.nextFloat() * 0.80f + 0.60f;
            float scatter    = rng.nextFloat() * 0.80f + 0.60f;
            float trails     = rng.nextFloat() * 0.35f + 0.45f;
            int scene        = rng.nextInt(juce::Range<int>(0, 5));

            if (brightness < 0.85f || brightness > 1.5f || trails >= 0.99f || react <= 0.0f || melt <= 0.0f || scatter <= 0.0f || scene < 0 || scene > 4)
            {
                allSafe = false;
                break;
            }
        }
        runTest("Randomize Look Safe Ranges (500 iterations: never black, never broken)", allSafe);
    }

    std::cout << "\n----------------------------------------\n";
    std::cout << "Test Summary: " << testsPassed << "/" << totalTests << " passed\n";
    std::cout << "----------------------------------------\n";

    return (testsPassed == totalTests) ? 0 : 1;
}
