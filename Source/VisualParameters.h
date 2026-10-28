#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

namespace drinkfloyd
{

enum class SceneType : int
{
    PixelMeltScatter = 0, // Primary reference scene (Default)
    Smear = 1,
    Ripple = 2,
    Flow = 3,
    Tiles = 4
};

inline juce::String getSceneName(SceneType scene)
{
    switch (scene)
    {
        case SceneType::PixelMeltScatter: return "Pixel Melt & Scatter";
        case SceneType::Smear:            return "Smear";
        case SceneType::Ripple:           return "Ripple";
        case SceneType::Flow:             return "Flow";
        case SceneType::Tiles:            return "Tiles";
    }
    return "Pixel Melt & Scatter";
}

enum class PalettePreset : int
{
    PrismCore = 0,
    NeonUltraviolet = 1,
    SolarFlare = 2,
    MonochromeIce = 3,
    DarkFloyd = 4,
    Custom = 5
};

enum class LensStyle : int
{
    Plain = 0,
    Ribs = 1,
    Rings = 2
};

struct VisualParameters
{
    // Scene (Pixel Melt & Scatter is active by default)
    SceneType scene = SceneType::PixelMeltScatter;

    // Dynamics
    float react = 1.0f;
    float chaos = 0.25f;
    float melt = 1.0f;
    float scatter = 1.0f;
    float inputGain = 1.0f;

    // Persistence
    float trails = 0.60f;
    float heat = 0.30f;
    float decay = 0.40f;

    // Color & Light (Hue Drift defaults to 0.0 for natural original colors)
    float brightness = 1.0f;
    float hueDrift = 0.0f;
    PalettePreset palette = PalettePreset::PrismCore;
    juce::Colour customColour = juce::Colour(0xffff007f); // Vibrant magenta default
    float vignette = 0.35f;

    // Geometry & Optics
    float symmetry = 1.0f;
    float tiles = 1.0f;
    float twist = 0.0f;
    float slice = 0.0f;
    float lens = 0.0f; // Clean rest view by default in Pixel Melt & Scatter
    LensStyle lensStyle = LensStyle::Plain;
};

} // namespace drinkfloyd
