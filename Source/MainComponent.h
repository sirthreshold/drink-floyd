#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "AudioAnalysisEngine.h"
#include "AudioCaptureEngine.h"
#include "HeaderBar.h"
#include "VisualCanvas.h"
#include "ParameterPanel.h"

namespace drinkfloyd
{

class MainComponent : public juce::Component,
                      public juce::KeyListener
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;

    void toggleFullscreen();
    void exitFullscreen();

private:
    // Audio capture & analysis engines
    AudioAnalysisEngine audioAnalysisEngine;
    AudioCaptureEngine  audioCaptureEngine;

    // UI Components
    HeaderBar      headerBar;
    VisualCanvas   visualCanvas;
    ParameterPanel parameterPanel;

    bool isFullscreen = false;
    juce::Rectangle<int> savedWindowBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace drinkfloyd
