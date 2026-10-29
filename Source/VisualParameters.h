#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

namespace drinkfloyd
{

enum class SceneType : int
{
    PixelMeltScatter = 0, // Primary reference scene (Default)
    Smear = 1,            // Fluid directional melt & slide
    Ripple = 2,           // Beat ripple wave
    Flow = 3,             // Liquid flow swirl
    Tiles = 4,            // Repeating kaleido tiles
    None = 5,             // Bypass (Pure image)
    Droste = 6,           // Recursive logarithmic spiral
    Infinite = 7,         // Hyperspace zoom tunnel
    Corridor = 8,         // 3D perspective grid tunnel
    Cube = 9,             // 3D isometric cube distortion
    Planet = 10,          // Little planet polar warp
    Windows = 11,         // Cascading windows glitch
    Melt = 12,            // Viscous liquid downward drip
    Shatter = 13,         // Fragmented polygon shatter explosion
    Prism = 14,           // Chromatic dispersion refraction
    Terrain = 15,         // 3D audio waveform ribbons / terrain
    Breathe = 16,         // Radial heartbeat / breath expansion
    Mosh = 17,            // Datamosh macroblock motion glitch
    Sort = 18,            // Directional luminance pixel sorting
    Coral = 19,           // Organic reaction-diffusion branching curl
    Scanline = 20,        // Retro CRT scanlines & phosphor warp
    SandCanvasScatter = 21 // Wide canvas sand melt & full disperse (partial image reveal)
};

inline juce::String getSceneName(SceneType scene)
{
    switch (scene)
    {
        case SceneType::PixelMeltScatter:  return "Pixel Melt & Scatter";
        case SceneType::Smear:             return "Smear";
        case SceneType::Ripple:            return "Ripple";
        case SceneType::Flow:              return "Flow";
        case SceneType::Tiles:             return "Tiles";
        case SceneType::None:              return "None";
        case SceneType::Droste:            return "Droste";
        case SceneType::Infinite:          return "Infinite";
        case SceneType::Corridor:          return "Corridor";
        case SceneType::Cube:              return "Cube";
        case SceneType::Planet:            return "Planet";
        case SceneType::Windows:           return "Windows";
        case SceneType::Melt:              return "Melt";
        case SceneType::Shatter:           return "Shatter";
        case SceneType::Prism:             return "Prism";
        case SceneType::Terrain:           return "Terrain";
        case SceneType::Breathe:           return "Breathe";
        case SceneType::Mosh:              return "Mosh";
        case SceneType::Sort:              return "Sort";
        case SceneType::Coral:             return "Coral";
        case SceneType::Scanline:          return "Scanline";
        case SceneType::SandCanvasScatter: return "Sand Melt & Canvas Scatter";
    }
    return "Pixel Melt & Scatter";
}

enum class PalettePreset : int
{
    Original = 0,
    PrismCore = 0, // Alias for backwards compatibility
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
    float pixelSize = 6.0f; // Block size in px at 1080p (range 0.5 to 12.0, default 6.0)
    float inputGain = 1.0f;

    // Persistence
    float trails = 0.60f;
    float heat = 0.30f;
    float decay = 0.40f;

    // Color & Light (Hue Drift defaults to 0.0 for natural original colors)
    float brightness = 1.0f;
    float hueDrift = 0.0f;
    PalettePreset palette = PalettePreset::Original;
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
