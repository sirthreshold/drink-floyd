#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

namespace drinkfloyd
{

enum class SceneType : int
{
    Smear = 0,
    Ripple = 1,
    Flow = 2,
    Tiles = 3
};

inline juce::String getSceneName(SceneType scene)
{
    switch (scene)
    {
        case SceneType::Smear:  return "Smear";
        case SceneType::Ripple: return "Ripple";
        case SceneType::Flow:   return "Flow";
        case SceneType::Tiles:  return "Tiles";
    }
    return "Smear";
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
    // Scene
    SceneType scene = SceneType::Smear;

    // Dynamics
    float react = 1.0f;
    float chaos = 0.20f;
    float inputGain = 1.0f;

    // Persistence
    float trails = 0.60f;
    float heat = 0.30f;
    float decay = 0.40f;

    // Color & Light
    float brightness = 1.0f;
    float hueDrift = 0.15f;
    PalettePreset palette = PalettePreset::PrismCore;
    juce::Colour customColour = juce::Colour(0xffff007f); // Vibrant magenta default
    float vignette = 0.35f;

    // Geometry & Optics
    float symmetry = 1.0f;
    float tiles = 1.0f;
    float twist = 0.0f;
    float slice = 0.0f;
    float lens = 0.25f;
    LensStyle lensStyle = LensStyle::Plain;
};

} // namespace drinkfloyd
