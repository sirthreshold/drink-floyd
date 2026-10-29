#include "ParameterPanel.h"
#include "VisualCanvas.h"

namespace drinkfloyd
{

// ==============================================================================
// ParameterPanelContent
// ==============================================================================
ParameterPanelContent::ParameterPanelContent(AudioAnalysisEngine& analysisEngine, VisualCanvas& visualCanvas)
    : analysis(analysisEngine),
      canvas(visualCanvas),
      monitorView(analysisEngine)
{
    // Audio Monitor View at the top
    addAndMakeVisible(monitorView);

    auto setupHeader = [this](juce::Label& lbl, const juce::String& text)
    {
        lbl.setText(text, juce::dontSendNotification);
        lbl.setFont(juce::FontOptions("Segoe UI Variable", 10.0f, juce::Font::bold));
        lbl.setColour(juce::Label::textColourId, juce::Colour(0xff8a8a9e));
        addAndMakeVisible(lbl);
    };

    // 0. PRESETS & PROFILES
    setupHeader(presetSection, "PRESETS & PROFILES");
    setupCombo(presetGroup, "Preset", presetManager.getPresetNames(), 1);
    presetGroup.comboBox.onChange = [this]()
    {
        juce::String chosenName = presetGroup.comboBox.getText();
        loadPresetByName(chosenName);
    };

    savePresetButton.setButtonText("SAVE PRESET");
    savePresetButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0ea5e9));
    savePresetButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff38bdf8));
    savePresetButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    savePresetButton.onClick = [this]() { promptSavePreset(); };
    addAndMakeVisible(savePresetButton);

    // 1. SCENE & LOOK (Pixel Melt & Scatter is default scene #1)
    setupHeader(sceneSection, "SCENE & GENERATIVE LOOK");
    setupCombo(sceneGroup, "Active Scene", {
        "Pixel Melt & Scatter (Reference)",
        "Smear",
        "Ripple (Beat Wave)",
        "Flow (Liquid Swirl)",
        "Tiles (Kaleido Grid)",
        "None (Bypass)",
        "Droste (Recursive Spiral)",
        "Infinite (Zoom Tunnel)",
        "Corridor (3D Grid)",
        "Cube (3D Facets)",
        "Planet (Polar Warp)",
        "Windows (Cascade Glitch)",
        "Melt (Liquid Drips)",
        "Shatter (Polygon Explode)",
        "Prism (Chromatic Aberration)",
        "Terrain (3D Waves)",
        "Breathe (Radial Pulse)",
        "Mosh (Datamosh Glitch)",
        "Sort (Pixel Sorting)",
        "Coral (Branching Curl)",
        "Scanline (CRT Retro)",
        "Sand Melt & Canvas Scatter (Wide Disperse)"
    }, 1);

    randomizeButton.setButtonText("RANDOMIZE LOOK");
    randomizeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff6366f1));
    randomizeButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff818cf8));
    randomizeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    randomizeButton.onClick = [this]() { randomizeLook(); };
    addAndMakeVisible(randomizeButton);

    setupHeader(dynamicsSection,    "AUDIO DYNAMICS");
    setupHeader(persistenceSection, "TEMPORAL / PERSISTENCE");
    setupHeader(colorSection,       "COLOR & LIGHT");
    setupHeader(geometrySection,    "GEOMETRY & OPTICS");

    // 1. Dynamics & Scatter
    setupSlider(reactGroup,       "React",       0.0, 2.0, 1.00);
    setupSlider(meltGroup,        "Melt",        0.0, 2.0, 1.00);
    setupSlider(scatterGroup,     "Scatter",     0.0, 2.0, 1.00);
    setupSlider(pixelSizeGroup,   "Pixel Size",  0.5, 12.0, 6.00, 0.1);
    setupSlider(chaosGroup,       "Chaos",       0.0, 1.0, 0.25);
    setupSlider(inputGainGroup,   "Input Gain",  0.0, 3.0, 1.00);

    // 2. Persistence
    setupSlider(trailsGroup, "Trails", 0.0, 0.98, 0.60);
    setupSlider(heatGroup,   "Heat",   0.0, 1.0,  0.30);
    setupSlider(decayGroup,  "Decay",  0.0, 1.0,  0.40);

    // 3. Color & Light (Hue Drift defaults to 0.0 for original natural colors)
    setupSlider(brightnessGroup, "Brightness", 0.1, 2.0, 1.00);
    setupSlider(hueDriftGroup,   "Hue Drift",   0.0, 1.0, 0.00);
    setupCombo(paletteGroup,     "Palette",    {
        "Original (True Colors)",
        "Neon Ultraviolet",
        "Solar Flare",
        "Monochrome Ice",
        "Dark Floyd (Toxic)",
        "Cyberpunk Sunset",
        "Vaporwave Twilight",
        "Emerald Aurora",
        "Blood Moon",
        "Deep Ocean Abyss",
        "Acid Toxic Green",
        "Hyperpop Bubblegum",
        "Golden Hour",
        "Infrared Thermal",
        "Retro Synthwave",
        "Custom Swatch"
    }, 1);

    // Custom Color Swatch button (cycles gradient swatch colors and activates Custom palette)
    customColorButton.setButtonText("Swatch");
    customColorButton.setColour(juce::TextButton::buttonColourId, currentCustomColour);
    customColorButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    customColorButton.onClick = [this]()
    {
        static const juce::Colour paletteChoices[] = {
            juce::Colour(0xffff007f), // Hot Magenta
            juce::Colour(0xff00e5ff), // Electric Cyan
            juce::Colour(0xffff9900), // Solar Amber
            juce::Colour(0xff9900ff), // Vivid Violet
            juce::Colour(0xff00ff88), // Neon Mint
            juce::Colour(0xffff3333), // Laser Crimson
            juce::Colour(0xffffdd00), // Acid Gold
            juce::Colour(0xff00ffea), // Neo Aqua
            juce::Colour(0xffff0055)  // Cyber Rose
        };
        static int colorIdx = 0;
        colorIdx = (colorIdx + 1) % 9;
        currentCustomColour = paletteChoices[colorIdx];
        customColorButton.setColour(juce::TextButton::buttonColourId, currentCustomColour);

        paletteGroup.comboBox.setSelectedId(16, juce::dontSendNotification); // Select Custom (16)
        pushAllParametersToCanvas();
    };
    addAndMakeVisible(customColorButton);

    setupSlider(vignetteGroup,   "Vignette",    0.0, 1.0, 0.35);

    // 4. Geometry & Optics (Lens defaults to 0.0 for clean rest view)
    setupSlider(symmetryGroup,  "Symmetry",   1.0, 12.0, 1.0, 1.0);
    setupSlider(tilesGroup,     "Tiles",      1.0, 16.0, 1.0, 1.0);
    setupSlider(twistGroup,     "Twist",     -1.0,  1.0, 0.0);
    setupSlider(sliceGroup,     "Slice",      0.0,  1.0, 0.0);
    setupSlider(lensGroup,      "Lens",       0.0,  1.0, 0.00);
    setupCombo(lensStyleGroup,  "Lens Style", { "Plain (Convex)", "Ribs (Fresnel Lines)", "Rings (Concentric)" }, 1);

    // Initial push of all parameters to canvas
    pushAllParametersToCanvas();
}

void ParameterPanelContent::setupSlider(SliderGroup& group, const juce::String& name, double minVal, double maxVal, double defaultVal, double step)
{
    group.label.setText(name, juce::dontSendNotification);
    group.label.setFont(juce::FontOptions("Segoe UI Variable", 11.5f, juce::Font::plain));
    group.label.setColour(juce::Label::textColourId, juce::Colour(0xffd4d4dc));
    addAndMakeVisible(group.label);

    group.slider.setSliderStyle(juce::Slider::LinearHorizontal);
    group.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    group.slider.setRange(minVal, maxVal, step);
    group.slider.setValue(defaultVal, juce::dontSendNotification);
    addAndMakeVisible(group.slider);

    group.valueLabel.setText(juce::String(defaultVal, (step < 1.0 ? 2 : 0)), juce::dontSendNotification);
    group.valueLabel.setFont(juce::FontOptions("Segoe UI Variable", 10.5f, juce::Font::plain));
    group.valueLabel.setColour(juce::Label::textColourId, juce::Colour(0xff71717a));
    group.valueLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(group.valueLabel);

    group.slider.onValueChange = [this, &group, step]()
    {
        group.valueLabel.setText(juce::String(group.slider.getValue(), (step < 1.0 ? 2 : 0)), juce::dontSendNotification);
        pushAllParametersToCanvas();
    };
}

void ParameterPanelContent::setupCombo(ComboGroup& group, const juce::String& name, const juce::StringArray& options, int defaultIndex)
{
    group.label.setText(name, juce::dontSendNotification);
    group.label.setFont(juce::FontOptions("Segoe UI Variable", 11.5f, juce::Font::plain));
    group.label.setColour(juce::Label::textColourId, juce::Colour(0xffd4d4dc));
    addAndMakeVisible(group.label);

    for (int i = 0; i < options.size(); ++i)
        group.comboBox.addItem(options[i], i + 1);

    group.comboBox.setSelectedId(defaultIndex, juce::dontSendNotification);
    group.comboBox.onChange = [this]() { pushAllParametersToCanvas(); };
    addAndMakeVisible(group.comboBox);
}

void ParameterPanelContent::pushAllParametersToCanvas()
{
    VisualParameters p;

    // Scene
    const int sceneId = sceneGroup.comboBox.getSelectedId();
    p.scene = static_cast<SceneType>(std::clamp(sceneId - 1, 0, 21));

    // Dynamics & Scatter
    p.react = static_cast<float>(reactGroup.slider.getValue());
    p.melt = static_cast<float>(meltGroup.slider.getValue());
    p.scatter = static_cast<float>(scatterGroup.slider.getValue());
    p.pixelSize = static_cast<float>(pixelSizeGroup.slider.getValue());
    p.chaos = static_cast<float>(chaosGroup.slider.getValue());
    p.inputGain = static_cast<float>(inputGainGroup.slider.getValue());
    analysis.setInputGain(p.inputGain);
    analysis.setReact(p.react);

    // Persistence
    p.trails = static_cast<float>(trailsGroup.slider.getValue());
    p.heat = static_cast<float>(heatGroup.slider.getValue());
    p.decay = static_cast<float>(decayGroup.slider.getValue());

    // Color & Light
    p.brightness = static_cast<float>(brightnessGroup.slider.getValue());
    p.hueDrift = static_cast<float>(hueDriftGroup.slider.getValue());

    const int palId = paletteGroup.comboBox.getSelectedId();
    p.palette = static_cast<PalettePreset>(std::clamp(palId - 1, 0, 15));
    p.customColour = currentCustomColour;
    p.vignette = static_cast<float>(vignetteGroup.slider.getValue());

    // Geometry & Optics
    p.symmetry = static_cast<float>(symmetryGroup.slider.getValue());
    p.tiles = static_cast<float>(tilesGroup.slider.getValue());
    p.twist = static_cast<float>(twistGroup.slider.getValue());
    p.slice = static_cast<float>(sliceGroup.slider.getValue());
    p.lens = static_cast<float>(lensGroup.slider.getValue());

    const int lensId = lensStyleGroup.comboBox.getSelectedId();
    p.lensStyle = static_cast<LensStyle>(std::clamp(lensId - 1, 0, 2));

    canvas.setParameters(p);
}

void ParameterPanelContent::applyParametersToUI(const VisualParameters& p)
{
    sceneGroup.comboBox.setSelectedId(static_cast<int>(p.scene) + 1, juce::dontSendNotification);
    reactGroup.slider.setValue(p.react, juce::dontSendNotification);
    reactGroup.valueLabel.setText(juce::String(p.react, 2), juce::dontSendNotification);
    meltGroup.slider.setValue(p.melt, juce::dontSendNotification);
    meltGroup.valueLabel.setText(juce::String(p.melt, 2), juce::dontSendNotification);
    scatterGroup.slider.setValue(p.scatter, juce::dontSendNotification);
    scatterGroup.valueLabel.setText(juce::String(p.scatter, 2), juce::dontSendNotification);
    pixelSizeGroup.slider.setValue(p.pixelSize, juce::dontSendNotification);
    pixelSizeGroup.valueLabel.setText(juce::String(p.pixelSize, 1), juce::dontSendNotification);
    chaosGroup.slider.setValue(p.chaos, juce::dontSendNotification);
    chaosGroup.valueLabel.setText(juce::String(p.chaos, 2), juce::dontSendNotification);
    inputGainGroup.slider.setValue(p.inputGain, juce::dontSendNotification);
    inputGainGroup.valueLabel.setText(juce::String(p.inputGain, 2), juce::dontSendNotification);

    trailsGroup.slider.setValue(p.trails, juce::dontSendNotification);
    trailsGroup.valueLabel.setText(juce::String(p.trails, 2), juce::dontSendNotification);
    heatGroup.slider.setValue(p.heat, juce::dontSendNotification);
    heatGroup.valueLabel.setText(juce::String(p.heat, 2), juce::dontSendNotification);
    decayGroup.slider.setValue(p.decay, juce::dontSendNotification);
    decayGroup.valueLabel.setText(juce::String(p.decay, 2), juce::dontSendNotification);

    brightnessGroup.slider.setValue(p.brightness, juce::dontSendNotification);
    brightnessGroup.valueLabel.setText(juce::String(p.brightness, 2), juce::dontSendNotification);
    hueDriftGroup.slider.setValue(p.hueDrift, juce::dontSendNotification);
    hueDriftGroup.valueLabel.setText(juce::String(p.hueDrift, 2), juce::dontSendNotification);

    paletteGroup.comboBox.setSelectedId(static_cast<int>(p.palette) + 1, juce::dontSendNotification);
    currentCustomColour = p.customColour;
    customColorButton.setColour(juce::TextButton::buttonColourId, currentCustomColour);
    vignetteGroup.slider.setValue(p.vignette, juce::dontSendNotification);
    vignetteGroup.valueLabel.setText(juce::String(p.vignette, 2), juce::dontSendNotification);

    symmetryGroup.slider.setValue(p.symmetry, juce::dontSendNotification);
    symmetryGroup.valueLabel.setText(juce::String(p.symmetry, 0), juce::dontSendNotification);
    tilesGroup.slider.setValue(p.tiles, juce::dontSendNotification);
    tilesGroup.valueLabel.setText(juce::String(p.tiles, 0), juce::dontSendNotification);
    twistGroup.slider.setValue(p.twist, juce::dontSendNotification);
    twistGroup.valueLabel.setText(juce::String(p.twist, 2), juce::dontSendNotification);
    sliceGroup.slider.setValue(p.slice, juce::dontSendNotification);
    sliceGroup.valueLabel.setText(juce::String(p.slice, 2), juce::dontSendNotification);
    lensGroup.slider.setValue(p.lens, juce::dontSendNotification);
    lensGroup.valueLabel.setText(juce::String(p.lens, 2), juce::dontSendNotification);
    lensStyleGroup.comboBox.setSelectedId(static_cast<int>(p.lensStyle) + 1, juce::dontSendNotification);

    analysis.setInputGain(p.inputGain);
    analysis.setReact(p.react);
    canvas.setParameters(p);
}

void ParameterPanelContent::refreshPresetList(const juce::String& selectName)
{
    presetManager.refreshUserPresets();
    auto names = presetManager.getPresetNames();
    presetGroup.comboBox.clear(juce::dontSendNotification);
    for (int i = 0; i < names.size(); ++i)
        presetGroup.comboBox.addItem(names[i], i + 1);

    if (selectName.isNotEmpty())
    {
        for (int i = 0; i < names.size(); ++i)
        {
            if (names[i].equalsIgnoreCase(selectName) || names[i].equalsIgnoreCase("[User] " + selectName))
            {
                presetGroup.comboBox.setSelectedId(i + 1, juce::dontSendNotification);
                return;
            }
        }
    }
}

void ParameterPanelContent::loadPresetByName(const juce::String& name)
{
    VisualParameters p;
    if (presetManager.getPresetByName(name, p))
    {
        applyParametersToUI(p);
    }
}

void ParameterPanelContent::promptSavePreset()
{
    auto* alert = new juce::AlertWindow("Save Custom Preset", "Enter a name for your custom look:", juce::AlertWindow::QuestionIcon);
    alert->addTextEditor("presetName", "My Custom Preset", "Preset Name:");
    alert->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    alert->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    alert->enterModalState(true, juce::ModalCallbackFunction::create([this, alert](int result)
    {
        if (result == 1)
        {
            juce::String name = alert->getTextEditorContents("presetName").trim();
            if (name.isNotEmpty())
            {
                VisualParameters currentParams = canvas.getParameters();
                if (presetManager.saveUserPreset(name, currentParams))
                {
                    refreshPresetList(name);
                }
            }
        }
        delete alert;
    }));
}

void ParameterPanelContent::randomizeLook()
{
    // Randomize scene (1 to 22)
    sceneGroup.comboBox.setSelectedId(random.nextInt(juce::Range<int>(1, 23)), juce::sendNotification);

    const int currentSceneId = sceneGroup.comboBox.getSelectedId();

    // Safe artistic parameter ranges (never black, never broken)
    reactGroup.slider.setValue(random.nextFloat() * 0.80f + 0.60f, juce::sendNotification);
    meltGroup.slider.setValue(random.nextFloat() * 0.80f + 0.60f, juce::sendNotification);
    scatterGroup.slider.setValue(random.nextFloat() * 0.80f + 0.60f, juce::sendNotification);
    chaosGroup.slider.setValue(random.nextFloat() * 0.35f + 0.15f, juce::sendNotification);
    trailsGroup.slider.setValue(random.nextFloat() * 0.35f + 0.45f, juce::sendNotification);
    heatGroup.slider.setValue(random.nextFloat() * 0.35f + 0.15f, juce::sendNotification);
    decayGroup.slider.setValue(random.nextFloat() * 0.30f + 0.25f, juce::sendNotification);
    brightnessGroup.slider.setValue(random.nextFloat() * 0.35f + 0.90f, juce::sendNotification); // 0.90 - 1.25
    vignetteGroup.slider.setValue(random.nextFloat() * 0.35f + 0.20f, juce::sendNotification);

    if (currentSceneId == 1 || currentSceneId == 22) // Pixel Melt & Scatter or Sand Canvas Scatter: natural colors
    {
        pixelSizeGroup.slider.setValue(random.nextFloat() * 3.5f + 1.5f, juce::sendNotification);
        hueDriftGroup.slider.setValue(0.0, juce::sendNotification);
        paletteGroup.comboBox.setSelectedId(1, juce::sendNotification); // Original
        lensGroup.slider.setValue(0.0, juce::sendNotification);
    }
    else
    {
        hueDriftGroup.slider.setValue(random.nextFloat() * 0.60f, juce::sendNotification);
        paletteGroup.comboBox.setSelectedId(random.nextInt(juce::Range<int>(1, 6)), juce::sendNotification);
        lensGroup.slider.setValue(random.nextFloat() * 0.35f + 0.10f, juce::sendNotification);
    }

    const int symChoices[] = { 1, 2, 3, 4 };
    symmetryGroup.slider.setValue(symChoices[random.nextInt(4)], juce::sendNotification);

    const int tileChoices[] = { 1, 2, 3 };
    tilesGroup.slider.setValue(tileChoices[random.nextInt(3)], juce::sendNotification);

    twistGroup.slider.setValue((random.nextFloat() - 0.5f) * 0.60f, juce::sendNotification);
    sliceGroup.slider.setValue(random.nextFloat() * 0.25f, juce::sendNotification);

    lensStyleGroup.comboBox.setSelectedId(random.nextInt(juce::Range<int>(1, 4)), juce::sendNotification);

    pushAllParametersToCanvas();
}

void ParameterPanelContent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff15151c));
    g.fillAll();
}

void ParameterPanelContent::resized()
{
    int y = 14;
    const int x = 16;
    const int panelWidth = getWidth() - 32;
    constexpr int rowHeight = 24;
    constexpr int rowSpacing = 8;
    constexpr int sectionSpacing = 16;

    // Audio Loopback & FFT Band Monitor
    monitorView.setBounds(x, y, panelWidth, 142);
    y += 142 + 14;

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
        y += 18 + 6;
    };

    // 0. PRESETS & PROFILES
    layoutSection(presetSection);
    presetGroup.label.setBounds(x, y, 55, rowHeight);
    presetGroup.comboBox.setBounds(x + 58, y, panelWidth - 58 - 100, rowHeight);
    savePresetButton.setBounds(x + panelWidth - 94, y, 94, rowHeight);
    y += rowHeight + sectionSpacing;

    // 1. SCENE & LOOK
    layoutSection(sceneSection);
    layoutCombo(sceneGroup);
    randomizeButton.setBounds(x, y, panelWidth, 28);
    y += 28 + sectionSpacing;

    // 1. DYNAMICS & SCATTER
    layoutSection(dynamicsSection);
    layoutSlider(reactGroup);
    layoutSlider(meltGroup);
    layoutSlider(scatterGroup);
    layoutSlider(pixelSizeGroup);
    layoutSlider(chaosGroup);
    layoutSlider(inputGainGroup);
    y += sectionSpacing;

    // 2. PERSISTENCE
    layoutSection(persistenceSection);
    layoutSlider(trailsGroup);
    layoutSlider(heatGroup);
    layoutSlider(decayGroup);
    y += sectionSpacing;

    // 3. COLOR & LIGHT
    layoutSection(colorSection);
    layoutSlider(brightnessGroup);
    layoutSlider(hueDriftGroup);

    paletteGroup.label.setBounds(x, y, 90, rowHeight);
    paletteGroup.comboBox.setBounds(x + 92, y, panelWidth - 92 - 60, rowHeight);
    customColorButton.setBounds(x + panelWidth - 54, y, 54, rowHeight);
    y += rowHeight + rowSpacing;

    layoutSlider(vignetteGroup);
    y += sectionSpacing;

    // 4. GEOMETRY & OPTICS
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
ParameterPanel::ParameterPanel(AudioAnalysisEngine& analysisEngine, VisualCanvas& canvas)
    : content(analysisEngine, canvas)
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
    g.setColour(juce::Colour(0xff15151c));
    g.fillRect(bounds);

    g.setColour(juce::Colour(0xff242432));
    g.drawVerticalLine(0, 0.0f, static_cast<float>(bounds.getHeight()));
}

void ParameterPanel::resized()
{
    viewport.setBounds(getLocalBounds());
    content.setSize(viewport.getMaximumVisibleWidth(), content.getHeight());
    content.resized();
}

} // namespace drinkfloyd
