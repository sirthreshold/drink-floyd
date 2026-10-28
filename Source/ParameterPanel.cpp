#include "ParameterPanel.h"

namespace drinkfloyd
{

// ==============================================================================
// ParameterPanelContent
// ==============================================================================
ParameterPanelContent::ParameterPanelContent()
{
    auto setupHeader = [this](juce::Label& lbl, const juce::String& text)
    {
        lbl.setText(text, juce::dontSendNotification);
        lbl.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        lbl.setColour(juce::Label::textColourId, juce::Colour(0xff00b4d8));
        addAndMakeVisible(lbl);
    };

    setupHeader(dynamicsSection,    "AUDIO DYNAMICS");
    setupHeader(persistenceSection, "TEMPORAL / PERSISTENCE");
    setupHeader(colorSection,       "COLOR & LIGHT");
    setupHeader(geometrySection,    "GEOMETRY & OPTICS");

    // 1. Dynamics
    setupSlider(reactGroup,     "React",      0.0, 1.0, 0.50);
    setupSlider(chaosGroup,     "Chaos",      0.0, 1.0, 0.20);
    setupSlider(inputGainGroup, "Input Gain", 0.0, 2.0, 1.00);

    // 2. Persistence
    setupSlider(trailsGroup, "Trails", 0.0, 1.0, 0.65);
    setupSlider(heatGroup,   "Heat",   0.0, 1.0, 0.30);
    setupSlider(decayGroup,  "Decay",  0.0, 1.0, 0.40);

    // 3. Color & Light
    setupSlider(brightnessGroup, "Brightness", 0.0, 2.0, 1.00);
    setupSlider(hueDriftGroup,   "Hue Drift",   0.0, 1.0, 0.15);
    setupCombo(paletteGroup,     "Palette",    { "Prism Core", "Neon Ultraviolet", "Solar Flare", "Monochrome Ice", "Dark Floyd" }, 1);
    setupSlider(vignetteGroup,   "Vignette",    0.0, 1.0, 0.35);

    // 4. Geometry & Optics
    setupSlider(symmetryGroup,  "Symmetry",   1.0, 12.0, 4.0, 1.0);
    setupSlider(tilesGroup,     "Tiles",      1.0, 16.0, 1.0, 1.0);
    setupSlider(twistGroup,     "Twist",     -1.0,  1.0, 0.0);
    setupSlider(sliceGroup,     "Slice",      0.0,  1.0, 0.0);
    setupSlider(lensGroup,      "Lens",       0.0,  1.0, 0.25);
    setupCombo(lensStyleGroup,  "Lens Style", { "Fisheye Convex", "Barrel Radial", "Pincushion", "Prismatic Split", "Anamorphic" }, 1);
}

void ParameterPanelContent::setupSlider(SliderGroup& group, const juce::String& name, double minVal, double maxVal, double defaultVal, double step)
{
    group.label.setText(name, juce::dontSendNotification);
    group.label.setFont(juce::FontOptions(12.0f, juce::Font::plain));
    group.label.setColour(juce::Label::textColourId, juce::Colour(0xffc5cad3));
    addAndMakeVisible(group.label);

    group.slider.setSliderStyle(juce::Slider::LinearHorizontal);
    group.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    group.slider.setRange(minVal, maxVal, step);
    group.slider.setValue(defaultVal, juce::dontSendNotification);
    group.slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xff00b4d8));
    group.slider.setColour(juce::Slider::trackColourId, juce::Colour(0xff1f2533));
    group.slider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff161a22));
    addAndMakeVisible(group.slider);

    group.valueLabel.setText(juce::String(defaultVal, (step < 1.0 ? 2 : 0)), juce::dontSendNotification);
    group.valueLabel.setFont(juce::FontOptions(11.0f, juce::Font::plain));
    group.valueLabel.setColour(juce::Label::textColourId, juce::Colour(0xff7d8799));
    group.valueLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(group.valueLabel);

    group.slider.onValueChange = [&group, step]()
    {
        group.valueLabel.setText(juce::String(group.slider.getValue(), (step < 1.0 ? 2 : 0)), juce::dontSendNotification);
    };
}

void ParameterPanelContent::setupCombo(ComboGroup& group, const juce::String& name, const juce::StringArray& options, int defaultIndex)
{
    group.label.setText(name, juce::dontSendNotification);
    group.label.setFont(juce::FontOptions(12.0f, juce::Font::plain));
    group.label.setColour(juce::Label::textColourId, juce::Colour(0xffc5cad3));
    addAndMakeVisible(group.label);

    for (int i = 0; i < options.size(); ++i)
        group.comboBox.addItem(options[i], i + 1);

    group.comboBox.setSelectedId(defaultIndex, juce::dontSendNotification);
    group.comboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff161a22));
    group.comboBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff272d3b));
    group.comboBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffdce1ea));
    group.comboBox.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xff8a93a5));
    addAndMakeVisible(group.comboBox);
}

void ParameterPanelContent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff14161d));
    g.fillAll();
}

void ParameterPanelContent::resized()
{
    int y = 14;
    const int x = 16;
    const int panelWidth = getWidth() - 32;
    constexpr int rowHeight = 24;
    constexpr int rowSpacing = 8;
    constexpr int sectionSpacing = 18;

    auto layoutSlider = [&](SliderGroup& sg)
    {
        sg.label.setBounds(x, y, 90, rowHeight);
        sg.valueLabel.setBounds(x + panelWidth - 45, y, 45, rowHeight);
        sg.slider.setBounds(x + 92, y, panelWidth - 140, rowHeight);
        y += rowHeight + rowSpacing;
    };

    auto layoutCombo = [&](ComboGroup& cg)
    {
        cg.label.setBounds(x, y, 90, rowHeight);
        cg.comboBox.setBounds(x + 92, y, panelWidth - 92, rowHeight);
        y += rowHeight + rowSpacing;
    };

    auto layoutSection = [&](juce::Label& lbl)
    {
        lbl.setBounds(x, y, panelWidth, 18);
        y += 18 + 8;
    };

    // DYNAMICS
    layoutSection(dynamicsSection);
    layoutSlider(reactGroup);
    layoutSlider(chaosGroup);
    layoutSlider(inputGainGroup);
    y += sectionSpacing;

    // PERSISTENCE
    layoutSection(persistenceSection);
    layoutSlider(trailsGroup);
    layoutSlider(heatGroup);
    layoutSlider(decayGroup);
    y += sectionSpacing;

    // COLOR & LIGHT
    layoutSection(colorSection);
    layoutSlider(brightnessGroup);
    layoutSlider(hueDriftGroup);
    layoutCombo(paletteGroup);
    layoutSlider(vignetteGroup);
    y += sectionSpacing;

    // GEOMETRY & OPTICS
    layoutSection(geometrySection);
    layoutSlider(symmetryGroup);
    layoutSlider(tilesGroup);
    layoutSlider(twistGroup);
    layoutSlider(sliceGroup);
    layoutSlider(lensGroup);
    layoutCombo(lensStyleGroup);
    y += 20;

    // Set height so Viewport scrolls when content is larger than window
    setSize(getWidth(), y);
}

// ==============================================================================
// ParameterPanel
// ==============================================================================
ParameterPanel::ParameterPanel()
{
    viewport.setViewedComponent(&content, false);
    viewport.setScrollBarsShown(true, false);
    viewport.setScrollBarThickness(8);
    viewport.getVerticalScrollBar().setColour(juce::ScrollBar::thumbColourId, juce::Colour(0xff2b3242));
    viewport.getVerticalScrollBar().setColour(juce::ScrollBar::trackColourId, juce::Colour(0xff14161d));
    addAndMakeVisible(viewport);
}

void ParameterPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    g.setColour(juce::Colour(0xff14161d));
    g.fillRect(bounds);

    // Left dividing border separating canvas from parameter panel
    g.setColour(juce::Colour(0xff232732));
    g.drawVerticalLine(0, 0.0f, static_cast<float>(bounds.getHeight()));
}

void ParameterPanel::resized()
{
    viewport.setBounds(getLocalBounds());
    content.setSize(viewport.getMaximumVisibleWidth(), content.getHeight());
    content.resized();
}

} // namespace drinkfloyd
