#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

namespace drinkfloyd
{

class ParameterPanelContent : public juce::Component
{
public:
    ParameterPanelContent();
    ~ParameterPanelContent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

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

    // Section Headers
    juce::Label dynamicsSection;
    juce::Label persistenceSection;
    juce::Label colorSection;
    juce::Label geometrySection;

    // 16 Specified Parameters
    SliderGroup reactGroup;       // react
    SliderGroup chaosGroup;       // chaos
    SliderGroup inputGainGroup;   // input gain
    SliderGroup trailsGroup;      // trails
    SliderGroup heatGroup;        // heat
    SliderGroup decayGroup;       // decay
    SliderGroup brightnessGroup;  // brightness
    SliderGroup hueDriftGroup;    // hue drift
    ComboGroup  paletteGroup;     // palette
    SliderGroup vignetteGroup;    // vignette
    SliderGroup symmetryGroup;    // symmetry
    SliderGroup tilesGroup;       // tiles
    SliderGroup twistGroup;       // twist
    SliderGroup sliceGroup;       // slice
    SliderGroup lensGroup;        // lens
    ComboGroup  lensStyleGroup;   // lens style

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterPanelContent)
};

class ParameterPanel : public juce::Component
{
public:
    ParameterPanel();
    ~ParameterPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Viewport viewport;
    ParameterPanelContent content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterPanel)
};

} // namespace drinkfloyd
