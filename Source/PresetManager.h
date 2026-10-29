#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <vector>
#include "VisualParameters.h"

namespace drinkfloyd
{

struct Preset
{
    juce::String name;
    VisualParameters params;
    bool isFactory = false;
};

class PresetManager
{
public:
    PresetManager();
    ~PresetManager() = default;

    // Available preset names (factory presets followed by user presets)
    juce::StringArray getPresetNames() const;

    // Load preset by name (returns false if not found)
    bool getPresetByName(const juce::String& name, VisualParameters& outParams) const;

    // Save user preset to disk
    bool saveUserPreset(const juce::String& name, const VisualParameters& params);

    // Delete user preset
    bool deleteUserPreset(const juce::String& name);

    // Refresh user presets from disk
    void refreshUserPresets();

private:
    void initFactoryPresets();
    juce::File getPresetsDirectory() const;

    std::vector<Preset> factoryPresets;
    std::vector<Preset> userPresets;
};

} // namespace drinkfloyd
