#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "AudioAnalysisEngine.h"
#include "AudioMonitorView.h"
#include "VisualParameters.h"

namespace drinkfloyd
{

class VisualCanvas;

class ParameterPanelContent : public juce::Component
{
public:
    ParameterPanelContent(AudioAnalysisEngine& analysisEngine, VisualCanvas& canvas);
    ~ParameterPanelContent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void randomizeLook();
    void pushAllParametersToCanvas();

private:
    struct SliderGroup
    {
        juce::Label label;
        juce::Slider slider;
        juce::Label valueLabel;
    };

    struct ComboGroup
    {
        juce::Label label;
        juce::ComboBox comboBox;
    };

    void setupSlider(SliderGroup& group, const juce::String& name, double minVal, double maxVal, double defaultVal, double step = 0.01);
    void setupCombo(ComboGroup& group, const juce::String& name, const juce::StringArray& options, int defaultIndex);

    AudioAnalysisEngine& analysis;
    VisualCanvas& canvas;

    // Real-time Audio Monitor
    AudioMonitorView monitorView;

    // Scene & Preset Strip
    juce::Label sceneSection;
    ComboGroup  sceneGroup;
    juce::TextButton randomizeButton;

    // Section Headers
    juce::Label dynamicsSection;
    juce::Label persistenceSection;
    juce::Label colorSection;
    juce::Label geometrySection;

    // Dynamics & Scatter Parameters
    SliderGroup reactGroup;       // react
    SliderGroup meltGroup;        // melt (gravity displacement)
    SliderGroup scatterGroup;     // scatter (radial explosion)
    SliderGroup pixelSizeGroup;   // pixel block size (3 - 12 px, default 6 px)
    SliderGroup chaosGroup;       // chaos
    SliderGroup inputGainGroup;   // input gain
    SliderGroup trailsGroup;      // trails
    SliderGroup heatGroup;        // heat
    SliderGroup decayGroup;       // decay
    SliderGroup brightnessGroup;  // brightness
    SliderGroup hueDriftGroup;    // hue drift
    ComboGroup  paletteGroup;     // palette
    juce::TextButton customColorButton; // Custom palette color picker swatch
    SliderGroup vignetteGroup;    // vignette
    SliderGroup symmetryGroup;    // symmetry
    SliderGroup tilesGroup;       // tiles
    SliderGroup twistGroup;       // twist
    SliderGroup sliceGroup;       // slice
    SliderGroup lensGroup;        // lens
    ComboGroup  lensStyleGroup;   // lens style

    juce::Colour currentCustomColour { 0xffff007f }; // Magenta default
    juce::Random random;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterPanelContent)
};

class ParameterPanel : public juce::Component
{
public:
    ParameterPanel(AudioAnalysisEngine& analysisEngine, VisualCanvas& canvas);
    ~ParameterPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void randomizeLook() { content.randomizeLook(); }

private:
    juce::Viewport viewport;
    ParameterPanelContent content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterPanel)
};

} // namespace drinkfloyd
