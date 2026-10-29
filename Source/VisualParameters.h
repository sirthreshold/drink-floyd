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
    Original = 0,          // 0: Original image colors (Untouched true colors)
    NeonUltraviolet = 1,   // 1: Neon Ultraviolet (Dark violet -> Magenta -> Cyan)
    SolarFlare = 2,        // 2: Solar Flare (Deep red -> Amber -> Sun gold)
    MonochromeIce = 3,     // 3: Monochrome Ice (Midnight blue -> Glacier -> Frost white)
    DarkFloyd = 4,         // 4: Dark Floyd (Deep black -> Toxic emerald -> Bright lime)
    CyberpunkSunset = 5,   // 5: Cyberpunk Sunset (Dark purple -> Hot pink -> Neon orange -> Yellow)
    VaporwaveTwilight = 6, // 6: Vaporwave Twilight (Navy -> Indigo -> Pastel magenta -> Sky blue)
    EmeraldAurora = 7,     // 7: Emerald Aurora (Deep forest -> Neon green -> Electric teal -> White)
    BloodMoon = 8,         // 8: Blood Moon (Obsidian -> Crimson -> Blood orange -> White)
    DeepOceanAbyss = 9,    // 9: Deep Ocean Abyss (Deep trench -> Sapphire -> Aquamarine -> White)
    AcidToxicGreen = 10,   // 10: Acid Toxic Green (Black -> Acid chartreuse -> Lime neon -> White)
    HyperpopBubblegum = 11,// 11: Hyperpop Bubblegum (Electric violet -> Hot bubblegum -> Peach -> Cream)
    GoldenHour = 12,       // 12: Golden Hour (Mahogany -> Amber bronze -> Golden sunshine -> Warm ivory)
    InfraredThermal = 13,  // 13: Infrared Thermal (Deep purple -> Blue -> Yellow -> White)
    RetroSynthwave = 14,   // 14: Retro Synthwave (Deep navy -> Magenta 80s -> Electric orange -> Yellow)
    Custom = 15            // 15: Custom (User picked swatch color gradient)
};

inline juce::String getPalettePresetName(PalettePreset p)
{
    switch (p)
    {
        case PalettePreset::Original:          return "Original (True Colors)";
        case PalettePreset::NeonUltraviolet:   return "Neon Ultraviolet";
        case PalettePreset::SolarFlare:        return "Solar Flare";
        case PalettePreset::MonochromeIce:     return "Monochrome Ice";
        case PalettePreset::DarkFloyd:         return "Dark Floyd (Toxic)";
        case PalettePreset::CyberpunkSunset:   return "Cyberpunk Sunset";
        case PalettePreset::VaporwaveTwilight: return "Vaporwave Twilight";
        case PalettePreset::EmeraldAurora:     return "Emerald Aurora";
        case PalettePreset::BloodMoon:         return "Blood Moon";
        case PalettePreset::DeepOceanAbyss:    return "Deep Ocean Abyss";
        case PalettePreset::AcidToxicGreen:    return "Acid Toxic Green";
        case PalettePreset::HyperpopBubblegum: return "Hyperpop Bubblegum";
        case PalettePreset::GoldenHour:        return "Golden Hour";
        case PalettePreset::InfraredThermal:   return "Infrared Thermal";
        case PalettePreset::RetroSynthwave:    return "Retro Synthwave";
        case PalettePreset::Custom:            return "Custom Swatch";
    }
    return "Original (True Colors)";
}

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
