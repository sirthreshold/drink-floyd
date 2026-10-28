#include "HeaderBar.h"
#include "VisualCanvas.h"
#include "AudioAnalysisEngine.h"

namespace drinkfloyd
{

HeaderBar::HeaderBar(AudioAnalysisEngine& analysisEngine)
    : analysis(analysisEngine)
{
    // App Title
    titleLabel.setText("DRINK FLOYD", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(17.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xfff0f2f5));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);

    // Status Badge
    statusBadge.setText("STANDALONE | PHASE 2", juce::dontSendNotification);
    statusBadge.setFont(juce::FontOptions(10.5f, juce::Font::plain));
    statusBadge.setColour(juce::Label::textColourId, juce::Colour(0xff8a93a5));
    statusBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff1d222c));
    statusBadge.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusBadge);

    // Audio Capture Status Badge
    audioStatusBadge.setText("[Audio Initializing...]", juce::dontSendNotification);
    audioStatusBadge.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    audioStatusBadge.setColour(juce::Label::textColourId, juce::Colour(0xff00e5a3));
    audioStatusBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff141d22));
    audioStatusBadge.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(audioStatusBadge);

    // Open Image Button
    openButton.setButtonText("Open Image...");
    openButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a1e28));
    openButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff272e3d));
    openButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffd5dbe5));
    openButton.onClick = [this]() { if (onOpenImage) onOpenImage(); };
    addAndMakeVisible(openButton);

    // Paste Image Button
    pasteButton.setButtonText("Paste (Ctrl+V)");
    pasteButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a1e28));
    pasteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff272e3d));
    pasteButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffd5dbe5));
    pasteButton.onClick = [this]() { if (onPasteImage) onPasteImage(); };
    addAndMakeVisible(pasteButton);

    // Scale Mode Toggle Button (Fit / Cover)
    scaleModeButton.setButtonText("Mode: Fit");
    scaleModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff152230));
    scaleModeButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff1f3248));
    scaleModeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00b4d8));
    scaleModeButton.onClick = [this]() { if (onToggleScaleMode) onToggleScaleMode(); };
    addAndMakeVisible(scaleModeButton);

    // Target Output Resolution profiles
    resolutionInfo.setText("OUT: 1080x1080 | 1080x1920 | 1920x1080", juce::dontSendNotification);
    resolutionInfo.setFont(juce::FontOptions(11.0f, juce::Font::plain));
    resolutionInfo.setColour(juce::Label::textColourId, juce::Colour(0xff6e7687));
    resolutionInfo.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(resolutionInfo);

    // FPS Readout
    fpsLabel.setText("60.0 FPS", juce::dontSendNotification);
    fpsLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    fpsLabel.setColour(juce::Label::textColourId, juce::Colour(0xff3ddc84));
    fpsLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff151b22));
    fpsLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(fpsLabel);

    lastFrameTime = juce::Time::getMillisecondCounterHiRes();
    startTimerHz(4);
}

HeaderBar::~HeaderBar()
{
    stopTimer();
}

void HeaderBar::updateScaleMode(ScaleMode mode)
{
    scaleModeButton.setButtonText(mode == ScaleMode::Fit ? "Mode: Fit" : "Mode: Cover");
}

void HeaderBar::recordFrame()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const double deltaMs = now - lastFrameTime;
    lastFrameTime = now;

    if (deltaMs > 0.0)
    {
        const double instantFps = 1000.0 / deltaMs;
        currentFps = (currentFps * 0.85) + (instantFps * 0.15);
    }
    ++frameCount;
}

void HeaderBar::timerCallback()
{
    fpsLabel.setText(juce::String(currentFps, 1) + " FPS", juce::dontSendNotification);

    const auto snap = analysis.getSnapshot();
    juce::String devName = snap.deviceName;
    if (devName.length() > 28)
        devName = devName.substring(0, 26) + "...";

    juce::String audioText = "[" + snap.statusMessage + " | " + devName + "]";
    audioStatusBadge.setText(audioText, juce::dontSendNotification);

    if (snap.status == AudioCaptureStatus::Listening)
    {
        audioStatusBadge.setColour(juce::Label::textColourId, juce::Colour(0xff00e5a3));
        audioStatusBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff12241f));
    }
    else if (snap.status == AudioCaptureStatus::DeviceChanged)
    {
        audioStatusBadge.setColour(juce::Label::textColourId, juce::Colour(0xffffaa33));
        audioStatusBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff251e14));
    }
    else
    {
        audioStatusBadge.setColour(juce::Label::textColourId, juce::Colour(0xff7d8799));
        audioStatusBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff161a22));
    }
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
    titleLabel.setBounds(20, bounds.getY(), 120, bounds.getHeight());
    statusBadge.setBounds(titleLabel.getRight() + 4, bounds.getY() + 3, 130, bounds.getHeight() - 6);

    // Action buttons next to badge
    openButton.setBounds(statusBadge.getRight() + 10, bounds.getY() + 2, 92, bounds.getHeight() - 4);
    pasteButton.setBounds(openButton.getRight() + 6, bounds.getY() + 2, 100, bounds.getHeight() - 4);
    scaleModeButton.setBounds(pasteButton.getRight() + 6, bounds.getY() + 2, 80, bounds.getHeight() - 4);

    // Right FPS counter
    const int fpsWidth = 80;
    fpsLabel.setBounds(bounds.getRight() - fpsWidth, bounds.getY() + 2, fpsWidth, bounds.getHeight() - 4);

    // Audio status badge and resolution info in center-right
    const int audioWidth = 260;
    audioStatusBadge.setBounds(fpsLabel.getX() - audioWidth - 8, bounds.getY() + 2, audioWidth, bounds.getHeight() - 4);

    const int remainingW = audioStatusBadge.getX() - scaleModeButton.getRight() - 16;
    if (remainingW > 160)
    {
        resolutionInfo.setVisible(true);
        resolutionInfo.setBounds(scaleModeButton.getRight() + 8, bounds.getY(), remainingW, bounds.getHeight());
    }
    else
    {
        resolutionInfo.setVisible(false);
    }
}

} // namespace drinkfloyd
