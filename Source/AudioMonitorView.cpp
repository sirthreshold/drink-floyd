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

    // Dark sleek container box
    g.setColour(juce::Colour(0xff10131a));
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);

    g.setColour(juce::Colour(0xff222733));
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 6.0f, 1.0f);

    int y = 10;
    const int padX = 12;
    const int contentW = w - padX * 2;

    // 1. Title + Device Status
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff00b4d8));
    g.drawText("WASAPI LOOPBACK (DEFAULT OUTPUT)", padX, y, contentW - 60, 16, juce::Justification::left, true);

    // Beat Pulse Indicator Pill
    const float beatAlpha = std::clamp(snapshot.bands.beat, 0.0f, 1.0f);
    juce::Rectangle<float> beatRect(static_cast<float>(w - padX - 50), static_cast<float>(y), 50.0f, 16.0f);
    g.setColour(juce::Colour(0xff181d26));
    g.fillRoundedRectangle(beatRect, 3.0f);

    if (beatAlpha > 0.05f)
    {
        g.setColour(juce::Colour(0xffff3366).withAlpha(beatAlpha));
        g.fillRoundedRectangle(beatRect, 3.0f);
    }
    g.setColour(beatAlpha > 0.3f ? juce::Colours::white : juce::Colour(0xff778193));
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText("BEAT", beatRect, juce::Justification::centred, false);

    y += 20;

    // Device Name
    g.setFont(juce::FontOptions(11.0f, juce::Font::plain));
    g.setColour(juce::Colour(0xff99a3b3));
    juce::String devName = snapshot.deviceName;
    if (devName.length() > 36)
        devName = devName.substring(0, 34) + "...";
    g.drawText(devName, padX, y, contentW, 14, juce::Justification::left, true);

    y += 16;

    // Status Indicator
    juce::Colour statusColour = juce::Colour(0xff7d8799); // Grey for No audio
    juce::String statusIcon = "[o] ";
    if (snapshot.status == AudioCaptureStatus::Listening)
    {
        statusColour = juce::Colour(0xff00e5a3); // Bright teal/green
        statusIcon = "[*] ";
    }
    else if (snapshot.status == AudioCaptureStatus::DeviceChanged)
    {
        statusColour = juce::Colour(0xffffaa33); // Amber
        statusIcon = "[~] ";
    }
    else if (snapshot.status == AudioCaptureStatus::Error)
    {
        statusColour = juce::Colour(0xffff4444); // Red
        statusIcon = "[!] ";
    }

    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.setColour(statusColour);
    g.drawText(statusIcon + snapshot.statusMessage, padX, y, contentW, 14, juce::Justification::left, true);

    y += 20;

    // 2. Frequency Band Meters
    struct BandInfo
    {
        const char* name;
        float value;
        juce::Colour barColour;
    };

    const BandInfo bands[4] = {
        { "BASS",    snapshot.bands.bass,   juce::Colour(0xffff4d6d) }, // Coral Red
        { "LOW-MID", snapshot.bands.lowMid, juce::Colour(0xffff9e00) }, // Orange/Gold
        { "MID",     snapshot.bands.mid,    juce::Colour(0xff00b4d8) }, // Cyan
        { "HIGH",    snapshot.bands.high,   juce::Colour(0xff9d4edd) }  // Violet
    };

    constexpr int barH = 8;
    constexpr int labelW = 55;
    constexpr int valW = 34;
    const int meterW = contentW - labelW - valW - 8;

    for (int i = 0; i < 4; ++i)
    {
        // Band Label
        g.setFont(juce::FontOptions(10.5f, juce::Font::plain));
        g.setColour(juce::Colour(0xffc2c8d2));
        g.drawText(bands[i].name, padX, y, labelW, 14, juce::Justification::left, false);

        // Meter Track Background
        const juce::Rectangle<float> trackRect(static_cast<float>(padX + labelW), static_cast<float>(y + 3), static_cast<float>(meterW), static_cast<float>(barH));
        g.setColour(juce::Colour(0xff1a1e28));
        g.fillRoundedRectangle(trackRect, 3.0f);

        // Active Level Bar
        const float val = std::clamp(bands[i].value, 0.0f, 1.0f);
        if (val > 0.005f)
        {
            const float filledW = std::max(2.0f, meterW * val);
            const juce::Rectangle<float> fillRect(trackRect.getX(), trackRect.getY(), filledW, static_cast<float>(barH));
            g.setColour(bands[i].barColour);
            g.fillRoundedRectangle(fillRect, 3.0f);
        }

        // Numeric Readout (0.00)
        g.setFont(juce::FontOptions(10.0f, juce::Font::plain));
        g.setColour(juce::Colour(0xff7d8799));
        g.drawText(juce::String(val, 2), padX + labelW + meterW + 8, y, valW, 14, juce::Justification::right, false);

        y += 18;
    }
}

} // namespace drinkfloyd
