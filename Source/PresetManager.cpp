#include "PresetManager.h"

namespace drinkfloyd
{

static juce::var paramsToVar(const VisualParameters& p)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("scene", static_cast<int>(p.scene));
    obj->setProperty("react", p.react);
    obj->setProperty("chaos", p.chaos);
    obj->setProperty("melt", p.melt);
    obj->setProperty("scatter", p.scatter);
    obj->setProperty("pixelSize", p.pixelSize);
    obj->setProperty("inputGain", p.inputGain);
    obj->setProperty("trails", p.trails);
    obj->setProperty("heat", p.heat);
    obj->setProperty("decay", p.decay);
    obj->setProperty("brightness", p.brightness);
    obj->setProperty("hueDrift", p.hueDrift);
    obj->setProperty("palette", static_cast<int>(p.palette));
    obj->setProperty("customColour", p.customColour.toString());
    obj->setProperty("vignette", p.vignette);
    obj->setProperty("symmetry", p.symmetry);
    obj->setProperty("tiles", p.tiles);
    obj->setProperty("twist", p.twist);
    obj->setProperty("slice", p.slice);
    obj->setProperty("lens", p.lens);
    obj->setProperty("lensStyle", static_cast<int>(p.lensStyle));
    return juce::var(obj);
}

static VisualParameters varToParams(const juce::var& v)
{
    VisualParameters p;
    if (!v.isObject())
        return p;

    p.scene = static_cast<SceneType>(static_cast<int>(v.getProperty("scene", 21)));
    p.react = static_cast<float>(v.getProperty("react", 1.0));
    p.chaos = static_cast<float>(v.getProperty("chaos", 0.25));
    p.melt = static_cast<float>(v.getProperty("melt", 1.0));
    p.scatter = static_cast<float>(v.getProperty("scatter", 1.0));
    p.pixelSize = static_cast<float>(v.getProperty("pixelSize", 5.0));
    p.inputGain = static_cast<float>(v.getProperty("inputGain", 1.0));
    p.trails = static_cast<float>(v.getProperty("trails", 0.60));
    p.heat = static_cast<float>(v.getProperty("heat", 0.30));
    p.decay = static_cast<float>(v.getProperty("decay", 0.40));
    p.brightness = static_cast<float>(v.getProperty("brightness", 1.0));
    p.hueDrift = static_cast<float>(v.getProperty("hueDrift", 0.0));
    p.palette = static_cast<PalettePreset>(static_cast<int>(v.getProperty("palette", 0)));
    p.customColour = juce::Colour::fromString(v.getProperty("customColour", "ffff007f").toString());
    p.vignette = static_cast<float>(v.getProperty("vignette", 0.35));
    p.symmetry = static_cast<float>(v.getProperty("symmetry", 1.0));
    p.tiles = static_cast<float>(v.getProperty("tiles", 1.0));
    p.twist = static_cast<float>(v.getProperty("twist", 0.0));
    p.slice = static_cast<float>(v.getProperty("slice", 0.0));
    p.lens = static_cast<float>(v.getProperty("lens", 0.0));
    p.lensStyle = static_cast<LensStyle>(static_cast<int>(v.getProperty("lensStyle", 0)));

    return p;
}

PresetManager::PresetManager()
{
    initFactoryPresets();
    refreshUserPresets();
}

void PresetManager::initFactoryPresets()
{
    factoryPresets.clear();

    // 1. Default (Sand Canvas Scatter)
    {
        Preset p;
        p.name = "Default (Sand Canvas Scatter)";
        p.isFactory = true;
        p.params.scene = SceneType::SandCanvasScatter;
        p.params.react = 1.0f;
        p.params.chaos = 0.30f;
        p.params.melt = 1.10f;
        p.params.scatter = 1.00f;
        p.params.pixelSize = 4.0f;
        p.params.trails = 0.65f;
        p.params.heat = 0.25f;
        p.params.decay = 0.40f;
        p.params.palette = PalettePreset::Original;
        factoryPresets.push_back(p);
    }

    // 2. Hip Hop / 808 Disperse
    {
        Preset p;
        p.name = "Hip Hop / 808 Disperse";
        p.isFactory = true;
        p.params.scene = SceneType::SandCanvasScatter;
        p.params.react = 1.40f;
        p.params.chaos = 0.45f;
        p.params.melt = 1.50f;
        p.params.scatter = 1.25f;
        p.params.pixelSize = 0.8f; // Microscopic sand grains
        p.params.trails = 0.70f;
        p.params.heat = 0.35f;
        p.params.decay = 0.35f;
        p.params.palette = PalettePreset::Original;
        factoryPresets.push_back(p);
    }

    // 3. House / Kick Quads
    {
        Preset p;
        p.name = "House / Kick Quads";
        p.isFactory = true;
        p.params.scene = SceneType::PixelMeltScatter;
        p.params.react = 1.15f;
        p.params.chaos = 0.20f;
        p.params.melt = 0.90f;
        p.params.scatter = 0.95f;
        p.params.pixelSize = 4.0f;
        p.params.trails = 0.60f;
        p.params.heat = 0.30f;
        p.params.decay = 0.45f;
        p.params.palette = PalettePreset::Original;
        factoryPresets.push_back(p);
    }

    // 4. Microscopic Sand Dust (0.5px)
    {
        Preset p;
        p.name = "Microscopic Sand Dust (0.5px)";
        p.isFactory = true;
        p.params.scene = SceneType::SandCanvasScatter;
        p.params.react = 1.60f;
        p.params.chaos = 0.60f;
        p.params.melt = 1.30f;
        p.params.scatter = 1.70f;
        p.params.pixelSize = 0.5f; // Absolute minimum 0.5px
        p.params.trails = 0.80f;
        p.params.heat = 0.20f;
        p.params.decay = 0.30f;
        p.params.palette = PalettePreset::Original;
        factoryPresets.push_back(p);
    }

    // 5. Liquid Droste Spiral
    {
        Preset p;
        p.name = "Liquid Droste Spiral";
        p.isFactory = true;
        p.params.scene = SceneType::Droste;
        p.params.react = 1.20f;
        p.params.chaos = 0.30f;
        p.params.trails = 0.82f;
        p.params.heat = 0.40f;
        p.params.decay = 0.35f;
        p.params.twist = 0.35f;
        p.params.palette = PalettePreset::NeonUltraviolet;
        factoryPresets.push_back(p);
    }

    // 6. Cyberpunk Datamosh
    {
        Preset p;
        p.name = "Cyberpunk Datamosh";
        p.isFactory = true;
        p.params.scene = SceneType::Mosh;
        p.params.react = 1.35f;
        p.params.chaos = 0.50f;
        p.params.trails = 0.75f;
        p.params.slice = 0.25f;
        p.params.palette = PalettePreset::CyberpunkSunset;
        factoryPresets.push_back(p);
    }

    // 7. Vaporwave Prism
    {
        Preset p;
        p.name = "Vaporwave Prism";
        p.isFactory = true;
        p.params.scene = SceneType::Prism;
        p.params.react = 1.10f;
        p.params.chaos = 0.30f;
        p.params.lens = 0.35f;
        p.params.lensStyle = LensStyle::Ribs;
        p.params.palette = PalettePreset::VaporwaveTwilight;
        factoryPresets.push_back(p);
    }

    // 8. Retro Synthwave CRT
    {
        Preset p;
        p.name = "Retro Synthwave CRT";
        p.isFactory = true;
        p.params.scene = SceneType::Scanline;
        p.params.react = 1.05f;
        p.params.heat = 0.45f;
        p.params.palette = PalettePreset::RetroSynthwave;
        p.params.vignette = 0.55f;
        factoryPresets.push_back(p);
    }

    // 9. Deep Ocean Flow
    {
        Preset p;
        p.name = "Deep Ocean Flow";
        p.isFactory = true;
        p.params.scene = SceneType::Flow;
        p.params.react = 1.25f;
        p.params.chaos = 0.40f;
        p.params.trails = 0.85f;
        p.params.symmetry = 2.0f;
        p.params.palette = PalettePreset::DeepOceanAbyss;
        factoryPresets.push_back(p);
    }

    // 10. Acid Toxic Tiles
    {
        Preset p;
        p.name = "Acid Toxic Tiles";
        p.isFactory = true;
        p.params.scene = SceneType::Tiles;
        p.params.react = 1.30f;
        p.params.tiles = 3.0f;
        p.params.twist = 0.40f;
        p.params.palette = PalettePreset::AcidToxicGreen;
        factoryPresets.push_back(p);
    }

    // 11. landscsape (User Custom Preset)
    {
        Preset p;
        p.name = "landscsape";
        p.isFactory = true;
        p.params.scene = SceneType::PixelMeltScatter;
        p.params.react = 1.17f;
        p.params.chaos = 0.07f;
        p.params.melt = 0.09f;
        p.params.scatter = 0.67f;
        p.params.pixelSize = 1.7f;
        p.params.inputGain = 0.54f;
        p.params.trails = 0.60f;
        p.params.heat = 0.48f;
        p.params.decay = 0.52f;
        p.params.brightness = 0.97f;
        p.params.hueDrift = 0.0f;
        p.params.palette = PalettePreset::Original;
        p.params.customColour = juce::Colour(0xffff0055);
        p.params.vignette = 0.37f;
        p.params.symmetry = 3.0f;
        p.params.tiles = 2.0f;
        p.params.twist = 0.17f;
        p.params.slice = 0.23f;
        p.params.lens = 0.44f;
        p.params.lensStyle = LensStyle::Rings;
        factoryPresets.push_back(p);
    }

    // 12. purple haze (User Custom Preset)
    {
        Preset p;
        p.name = "purple haze";
        p.isFactory = true;
        p.params.scene = SceneType::Tiles;
        p.params.react = 0.90f;
        p.params.chaos = 0.30f;
        p.params.melt = 1.10f;
        p.params.scatter = 1.0f;
        p.params.pixelSize = 0.9f;
        p.params.inputGain = 0.28f;
        p.params.trails = 0.65f;
        p.params.heat = 0.25f;
        p.params.decay = 0.72f;
        p.params.brightness = 1.08f;
        p.params.hueDrift = 0.0f;
        p.params.palette = PalettePreset::Custom;
        p.params.customColour = juce::Colour(0xff9900ff);
        p.params.vignette = 0.35f;
        p.params.symmetry = 1.0f;
        p.params.tiles = 3.0f;
        p.params.twist = -0.02f;
        p.params.slice = 0.0f;
        p.params.lens = 0.31f;
        p.params.lensStyle = LensStyle::Plain;
        factoryPresets.push_back(p);
    }

    // 13. randomizer (User Custom Preset)
    {
        Preset p;
        p.name = "randomizer";
        p.isFactory = true;
        p.params.scene = SceneType::SandCanvasScatter;
        p.params.react = 0.47f;
        p.params.chaos = 0.11f;
        p.params.melt = 0.28f;
        p.params.scatter = 0.34f;
        p.params.pixelSize = 1.3f;
        p.params.inputGain = 2.17f;
        p.params.trails = 0.64f;
        p.params.heat = 0.48f;
        p.params.decay = 0.52f;
        p.params.brightness = 0.97f;
        p.params.hueDrift = 0.56f;
        p.params.palette = PalettePreset::BloodMoon;
        p.params.customColour = juce::Colour(0xff00ff88);
        p.params.vignette = 0.37f;
        p.params.symmetry = 3.0f;
        p.params.tiles = 2.0f;
        p.params.twist = 0.17f;
        p.params.slice = 0.23f;
        p.params.lens = 0.44f;
        p.params.lensStyle = LensStyle::Plain;
        factoryPresets.push_back(p);
    }

    // 14. sand (User Custom Preset)
    {
        Preset p;
        p.name = "sand";
        p.isFactory = true;
        p.params.scene = SceneType::PixelMeltScatter;
        p.params.react = 0.47f;
        p.params.chaos = 0.0f;
        p.params.melt = 0.41f;
        p.params.scatter = 0.21f;
        p.params.pixelSize = 1.6f;
        p.params.inputGain = 2.17f;
        p.params.trails = 0.60f;
        p.params.heat = 0.48f;
        p.params.decay = 0.52f;
        p.params.brightness = 0.97f;
        p.params.hueDrift = 0.0f;
        p.params.palette = PalettePreset::Original;
        p.params.customColour = juce::Colour(0xffff0055);
        p.params.vignette = 0.37f;
        p.params.symmetry = 3.0f;
        p.params.tiles = 2.0f;
        p.params.twist = 0.17f;
        p.params.slice = 0.23f;
        p.params.lens = 0.44f;
        p.params.lensStyle = LensStyle::Rings;
        factoryPresets.push_back(p);
    }

    // 15. My Custom Preset (User Custom Preset)
    {
        Preset p;
        p.name = "My Custom Preset";
        p.isFactory = true;
        p.params.scene = SceneType::PixelMeltScatter;
        p.params.react = 0.47f;
        p.params.chaos = 0.0f;
        p.params.melt = 0.41f;
        p.params.scatter = 0.21f;
        p.params.pixelSize = 1.6f;
        p.params.inputGain = 2.17f;
        p.params.trails = 0.60f;
        p.params.heat = 0.48f;
        p.params.decay = 0.52f;
        p.params.brightness = 0.97f;
        p.params.hueDrift = 0.40f;
        p.params.palette = PalettePreset::Custom;
        p.params.customColour = juce::Colour(0xffff0055);
        p.params.vignette = 0.37f;
        p.params.symmetry = 3.0f;
        p.params.tiles = 2.0f;
        p.params.twist = 0.17f;
        p.params.slice = 0.23f;
        p.params.lens = 0.44f;
        p.params.lensStyle = LensStyle::Rings;
        factoryPresets.push_back(p);
    }
}

juce::File PresetManager::getPresetsDirectory() const
{
    // Check local Presets folder in current directory or user app data
    auto localDir = juce::File::getCurrentWorkingDirectory().getChildFile("Presets");
    if (!localDir.exists())
    {
        auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                  .getChildFile("DrinkFloyd")
                                  .getChildFile("Presets");
        appDataDir.createDirectory();
        return appDataDir;
    }
    return localDir;
}

void PresetManager::refreshUserPresets()
{
    userPresets.clear();

    std::vector<juce::File> dirs;
    dirs.push_back(juce::File::getCurrentWorkingDirectory().getChildFile("Presets"));
    dirs.push_back(juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory().getChildFile("Presets"));
    dirs.push_back(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("DrinkFloyd").getChildFile("Presets"));

    std::vector<juce::String> seenNames;

    for (const auto& dir : dirs)
    {
        if (dir.isDirectory())
        {
            auto files = dir.findChildFiles(juce::File::findFiles, false, "*.json");
            for (const auto& file : files)
            {
                juce::String presetName = file.getFileNameWithoutExtension();
                bool alreadySeen = false;
                for (const auto& s : seenNames)
                {
                    if (s.equalsIgnoreCase(presetName))
                    {
                        alreadySeen = true;
                        break;
                    }
                }
                if (alreadySeen)
                    continue;

                juce::var parsedJson;
                if (juce::JSON::parse(file.loadFileAsString(), parsedJson).wasOk())
                {
                    Preset p;
                    p.name = presetName;
                    p.isFactory = false;
                    p.params = varToParams(parsedJson);
                    userPresets.push_back(p);
                    seenNames.push_back(presetName);
                }
            }
        }
    }
}

juce::StringArray PresetManager::getPresetNames() const
{
    juce::StringArray names;
    for (const auto& fp : factoryPresets)
        names.add(fp.name);
    for (const auto& up : userPresets)
        names.add("[User] " + up.name);
    return names;
}

bool PresetManager::getPresetByName(const juce::String& name, VisualParameters& outParams) const
{
    for (const auto& fp : factoryPresets)
    {
        if (fp.name.equalsIgnoreCase(name))
        {
            outParams = fp.params;
            return true;
        }
    }

    juce::String cleanName = name;
    if (cleanName.startsWith("[User] "))
        cleanName = cleanName.substring(7);

    for (const auto& up : userPresets)
    {
        if (up.name.equalsIgnoreCase(cleanName))
        {
            outParams = up.params;
            return true;
        }
    }

    return false;
}

bool PresetManager::saveUserPreset(const juce::String& name, const VisualParameters& params)
{
    if (name.trim().isEmpty())
        return false;

    juce::String sanitized = juce::File::createLegalFileName(name.trim());
    auto dir = getPresetsDirectory();
    dir.createDirectory();

    auto file = dir.getChildFile(sanitized + ".json");
    auto jsonVar = paramsToVar(params);
    juce::String jsonStr = juce::JSON::toString(jsonVar, false);

    bool ok = file.replaceWithText(jsonStr);
    if (ok)
        refreshUserPresets();

    return ok;
}

bool PresetManager::deleteUserPreset(const juce::String& name)
{
    juce::String cleanName = name;
    if (cleanName.startsWith("[User] "))
        cleanName = cleanName.substring(7);

    auto dir = getPresetsDirectory();
    auto file = dir.getChildFile(cleanName + ".json");
    if (file.existsAsFile())
    {
        bool ok = file.deleteFile();
        if (ok)
            refreshUserPresets();
        return ok;
    }
    return false;
}

} // namespace drinkfloyd
