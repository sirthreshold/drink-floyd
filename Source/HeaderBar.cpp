#include "HeaderBar.h"

namespace drinkfloyd
{

HeaderBar::HeaderBar()
{
    // App Title
    titleLabel.setText("DRINK FLOYD", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(17.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xfff0f2f5));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);

    // Status Badge
    statusBadge.setText("STANDALONE | PHASE 0", juce::dontSendNotification);
    statusBadge.setFont(juce::FontOptions(10.5f, juce::Font::plain));
    statusBadge.setColour(juce::Label::textColourId, juce::Colour(0xff8a93a5));
    statusBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff1d222c));
    statusBadge.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusBadge);

    // Target Output Resolution profiles
    resolutionInfo.setText("OUT: 1080x1080 | 1080x1920 | 1920x1080", juce::dontSendNotification);
    resolutionInfo.setFont(juce::FontOptions(11.0f, juce::Font::plain));
    resolutionInfo.setColour(juce::Label::textColourId, juce::Colour(0xff6e7687));
    resolutionInfo.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(resolutionInfo);

    // FPS Readout
    fpsLabel.setText("60.0 FPS", juce::dontSendNotification);
    fpsLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    fpsLabel.setColour(juce::Label::textColourId, juce::Colour(0xff3ddc84)); // High-visibility tech green
    fpsLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff151b22));
    fpsLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(fpsLabel);

    lastFrameTime = juce::Time::getMillisecondCounterHiRes();
    startTimerHz(4); // Update FPS readout 4 times per second
}

HeaderBar::~HeaderBar()
{
    stopTimer();
}

void HeaderBar::recordFrame()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const double deltaMs = now - lastFrameTime;
    lastFrameTime = now;

    if (deltaMs > 0.0)
    {
        const double instantFps = 1000.0 / deltaMs;
        // Smooth exponential moving average
        currentFps = (currentFps * 0.85) + (instantFps * 0.15);
    }
    ++frameCount;
}

void HeaderBar::timerCallback()
{
    fpsLabel.setText(juce::String(currentFps, 1) + " FPS", juce::dontSendNotification);
}

void HeaderBar::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Dark sleek background
    g.setColour(juce::Colour(0xff111318));
    g.fillRect(bounds);

    // Bottom hairline separator
    g.setColour(juce::Colour(0xff232732));
    g.drawHorizontalLine(bounds.getBottom() - 1, 0.0f, static_cast<float>(bounds.getWidth()));

    // Title accent mark
    g.setColour(juce::Colour(0xff00b4d8));
    g.fillRoundedRectangle(12.0f, static_cast<float>(bounds.getCentreY() - 7), 4.0f, 14.0f, 2.0f);
}

void HeaderBar::resized()
{
    auto bounds = getLocalBounds().reduced(8, 6);

    // Left title and badge
    titleLabel.setBounds(20, bounds.getY(), 130, bounds.getHeight());
    statusBadge.setBounds(titleLabel.getRight() + 8, bounds.getY() + 3, 140, bounds.getHeight() - 6);

    // Center output info
    const int centerWidth = 260;
    resolutionInfo.setBounds((getWidth() - centerWidth) / 2, bounds.getY(), centerWidth, bounds.getHeight());

    // Right FPS counter
    const int fpsWidth = 90;
    fpsLabel.setBounds(bounds.getRight() - fpsWidth, bounds.getY() + 2, fpsWidth, bounds.getHeight() - 4);
}

} // namespace drinkfloyd
