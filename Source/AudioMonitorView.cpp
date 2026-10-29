#include "AudioMonitorView.h"
#include <iomanip>

namespace drinkfloyd
{

AudioMonitorView::AudioMonitorView(AudioAnalysisEngine& engine)
    : analysis(engine)
{
    startTimerHz(60);
}

AudioMonitorView::~AudioMonitorView()
{
    stopTimer();
}

void AudioMonitorView::timerCallback()
{
    snapshot = analysis.getSnapshot();
    repaint();
}

void AudioMonitorView::resized()
{
}

void AudioMonitorView::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds();
    const int w = bounds.getWidth();

    // macOS Dark Card Container
    g.setColour(juce::Colour(0xff171720));
    g.fillRoundedRectangle(bounds.toFloat(), 8.0f);

    g.setColour(juce::Colour(0xff272736));
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 8.0f, 1.0f);

    int y = 10;
    const int padX = 12;
    const int contentW = w - padX * 2;

    // 1. Header: Status Dot + Loopback Title + Beat Pulse
    g.setFont(juce::FontOptions("Segoe UI Variable", 10.5f, juce::Font::bold));
    g.setColour(juce::Colour(0xff8e8e9d));
    g.drawText("SYSTEM AUDIO LOOPBACK", padX, y, contentW - 55, 14, juce::Justification::left, true);

    // Beat Pulse Indicator Pill
    const float beatAlpha = std::clamp(snapshot.bands.beat, 0.0f, 1.0f);
    juce::Rectangle<float> beatRect(static_cast<float>(w - padX - 46), static_cast<float>(y - 1), 46.0f, 16.0f);
    g.setColour(juce::Colour(0xff1d1d28));
    g.fillRoundedRectangle(beatRect, 8.0f);

    if (beatAlpha > 0.04f)
    {
        g.setColour(juce::Colour(0xfff43f5e).withAlpha(beatAlpha));
        g.fillRoundedRectangle(beatRect, 8.0f);
    }
    g.setColour(beatAlpha > 0.25f ? juce::Colours::white : juce::Colour(0xff6b6b7d));
    g.setFont(juce::FontOptions("Segoe UI Variable", 9.5f, juce::Font::bold));
    g.drawText("BEAT", beatRect, juce::Justification::centred, false);

    y += 18;

    // Status Dot + Active Device Name
    juce::Colour dotColour = juce::Colour(0xff6b6b7d);
    if (snapshot.status == AudioCaptureStatus::Listening)
        dotColour = juce::Colour(0xff10b981); // Emerald
    else if (snapshot.status == AudioCaptureStatus::DeviceChanged)
        dotColour = juce::Colour(0xfff59e0b); // Amber
    else if (snapshot.status == AudioCaptureStatus::Error)
        dotColour = juce::Colour(0xfff43f5e); // Red

    // Glowing Dot
    g.setColour(dotColour.withAlpha(0.25f));
    g.fillEllipse(static_cast<float>(padX - 1), static_cast<float>(y + 3), 10.0f, 10.0f);
    g.setColour(dotColour);
    g.fillEllipse(static_cast<float>(padX + 1), static_cast<float>(y + 5), 6.0f, 6.0f);

    g.setFont(juce::FontOptions("Segoe UI Variable", 11.0f, juce::Font::plain));
    g.setColour(juce::Colour(0xffa1a1b0));
    juce::String devName = snapshot.deviceName;
    if (devName.length() > 32)
        devName = devName.substring(0, 30) + "...";
    g.drawText(devName, padX + 14, y, contentW - 14, 16, juce::Justification::left, true);

    y += 20;

    // 2. Frequency Band Meters
    struct BandInfo
    {
        const char* name;
        float value;
        juce::Colour barColour;
    };

    const BandInfo bands[4] = {
        { "BASS",    snapshot.bands.bass,   juce::Colour(0xfff43f5e) }, // Apple Rose
        { "LOW-MID", snapshot.bands.lowMid, juce::Colour(0xfff59e0b) }, // Apple Amber
        { "MID",     snapshot.bands.mid,    juce::Colour(0xff10b981) }, // Apple Emerald
        { "HIGH",    snapshot.bands.high,   juce::Colour(0xff0ea5e9) }  // Apple Sky
    };

    constexpr int barH = 5;
    constexpr int labelW = 55;
    constexpr int valW = 32;
    const int meterW = contentW - labelW - valW - 8;

    for (int i = 0; i < 4; ++i)
    {
        // Band Label
        g.setFont(juce::FontOptions("Segoe UI Variable", 10.5f, juce::Font::bold));
        g.setColour(juce::Colour(0xff8e8e9d));
        g.drawText(bands[i].name, padX, y, labelW, 14, juce::Justification::left, false);

        // Meter Track Background Capsule
        const juce::Rectangle<float> trackRect(static_cast<float>(padX + labelW), static_cast<float>(y + 4), static_cast<float>(meterW), static_cast<float>(barH));
        g.setColour(juce::Colour(0xff13131a));
        g.fillRoundedRectangle(trackRect, 2.5f);

        // Active Level Bar Capsule
        const float val = std::clamp(bands[i].value, 0.0f, 1.0f);
        if (val > 0.005f)
        {
            const float filledW = std::max(3.0f, meterW * val);
            const juce::Rectangle<float> fillRect(trackRect.getX(), trackRect.getY(), filledW, static_cast<float>(barH));
            g.setColour(bands[i].barColour);
            g.fillRoundedRectangle(fillRect, 2.5f);
        }

        // Numeric Readout (0.00)
        g.setFont(juce::FontOptions("Segoe UI Variable", 9.5f, juce::Font::plain));
        g.setColour(juce::Colour(0xff71717a));
        g.drawText(juce::String(val, 2), padX + labelW + meterW + 8, y, valW, 14, juce::Justification::right, false);

        y += 18;
    }
}

} // namespace drinkfloyd
