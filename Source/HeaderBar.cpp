#include "HeaderBar.h"
#include "VisualCanvas.h"
#include "AudioAnalysisEngine.h"

namespace drinkfloyd
{

HeaderBar::HeaderBar(AudioAnalysisEngine& analysisEngine)
    : analysis(analysisEngine)
{
    // macOS Traffic Light Dots
    auto setupTrafficDot = [this](juce::TextButton& btn, std::function<void()> onClick)
    {
        btn.setButtonText("");
        btn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        btn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
        btn.onClick = onClick;
        addAndMakeVisible(btn);
    };

    setupTrafficDot(closeDot, [this]()
    {
        if (auto* app = juce::JUCEApplication::getInstance())
            app->systemRequestedQuit();
    });

    setupTrafficDot(minimizeDot, [this]()
    {
        if (auto* peer = getPeer())
            peer->setMinimised(true);
    });

    setupTrafficDot(zoomDot, [this]()
    {
        if (onToggleFullscreen)
            onToggleFullscreen();
    });

    // App Title
    titleLabel.setText("DRINK FLOYD", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions("Segoe UI Variable", 13.5f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xfff4f4f6));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);

    // Status Badge (Apple Indigo Pill)
    statusBadge.setText("v2.0 PRO", juce::dontSendNotification);
    statusBadge.setFont(juce::FontOptions("Segoe UI Variable", 9.5f, juce::Font::bold));
    statusBadge.setColour(juce::Label::textColourId, juce::Colours::white);
    statusBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff6366f1));
    statusBadge.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusBadge);

    // Audio Capture Status Badge (Frosted Capsule)
    audioStatusBadge.setText("Listening...", juce::dontSendNotification);
    audioStatusBadge.setFont(juce::FontOptions("Segoe UI Variable", 10.5f, juce::Font::bold));
    audioStatusBadge.setColour(juce::Label::textColourId, juce::Colour(0xff10b981));
    audioStatusBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff1c2420));
    audioStatusBadge.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(audioStatusBadge);

    // Open Image Button
    openButton.setButtonText("Open...");
    openButton.onClick = [this]() { if (onOpenImage) onOpenImage(); };
    addAndMakeVisible(openButton);

    // Paste Image Button
    pasteButton.setButtonText("Paste");
    pasteButton.onClick = [this]() { if (onPasteImage) onPasteImage(); };
    addAndMakeVisible(pasteButton);

    // Sync YouTube Button (Apple Dark Crimson)
    syncYouTubeButton.setButtonText("Sync YouTube");
    syncYouTubeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffe11d48));
    syncYouTubeButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xfff43f5e));
    syncYouTubeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    syncYouTubeButton.onClick = [this]() { if (onSyncYouTube) onSyncYouTube(); };
    addAndMakeVisible(syncYouTubeButton);

    // Scale Mode Toggle Button (Fit / Cover)
    scaleModeButton.setButtonText("Fit");
    scaleModeButton.onClick = [this]() { if (onToggleScaleMode) onToggleScaleMode(); };
    addAndMakeVisible(scaleModeButton);

    // Fullscreen Button
    fullscreenButton.setButtonText("Fullscreen");
    fullscreenButton.onClick = [this]() { if (onToggleFullscreen) onToggleFullscreen(); };
    addAndMakeVisible(fullscreenButton);

    // Target Output Resolution profiles (1080x1080 / 1080x1920 / 1920x1080)
    auto setupAspectBtn = [this](juce::TextButton& btn, const juce::String& text, int idx)
    {
        btn.setButtonText(text);
        btn.onClick = [this, idx]()
        {
            updateAspectButtons(idx);
            if (onAspectChanged) onAspectChanged(idx);
        };
        addAndMakeVisible(btn);
    };

    setupAspectBtn(resSquareButton,    "1:1",  0);
    setupAspectBtn(resPortraitButton,  "9:16", 1);
    setupAspectBtn(resLandscapeButton, "16:9", 2);

    updateAspectButtons(0); // Default to 1080x1080 (1:1)

    // Real-Time Rendered FPS Readout
    fpsLabel.setText("60.0 FPS", juce::dontSendNotification);
    fpsLabel.setFont(juce::FontOptions("Segoe UI Variable", 11.5f, juce::Font::bold));
    fpsLabel.setColour(juce::Label::textColourId, juce::Colour(0xff10b981));
    fpsLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff181824));
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

void HeaderBar::updateAspectButtons(int activeIndex)
{
    auto setBtnActive = [](juce::TextButton& btn, bool active)
    {
        btn.setColour(juce::TextButton::buttonColourId, active ? juce::Colour(0xff6366f1) : juce::Colour(0xff1d1d28));
        btn.setColour(juce::TextButton::textColourOffId, active ? juce::Colours::white : juce::Colour(0xffa1a1b0));
    };

    setBtnActive(resSquareButton,    activeIndex == 0);
    setBtnActive(resPortraitButton,  activeIndex == 1);
    setBtnActive(resLandscapeButton, activeIndex == 2);
}

void HeaderBar::recordFrame()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const double deltaMs = now - lastFrameTime;
    lastFrameTime = now;

    if (deltaMs > 0.0 && deltaMs < 200.0)
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
    if (devName.length() > 24)
        devName = devName.substring(0, 22) + "...";

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

    // macOS Dark header background
    g.setColour(juce::Colour(0xff16161d));
    g.fillRect(bounds);

    // Bottom hairline separator
    g.setColour(juce::Colour(0xff262634));
    g.drawHorizontalLine(bounds.getBottom() - 1, 0.0f, static_cast<float>(bounds.getWidth()));

    // Draw macOS traffic light circles
    auto drawDot = [&](juce::TextButton& btn, juce::Colour fillCol, juce::Colour borderCol)
    {
        auto b = btn.getBounds().toFloat();
        g.setColour(fillCol);
        g.fillEllipse(b);
        g.setColour(borderCol);
        g.drawEllipse(b, 0.8f);
    };

    drawDot(closeDot,    juce::Colour(0xffff5f56), juce::Colour(0xffe0443e));
    drawDot(minimizeDot, juce::Colour(0xffffbd2e), juce::Colour(0xffd89e24));
    drawDot(zoomDot,     juce::Colour(0xff27c93f), juce::Colour(0xff1aab29));
}

void HeaderBar::resized()
{
    auto bounds = getLocalBounds().reduced(8, 6);

    // Left: macOS Traffic light dots
    closeDot.setBounds(14, bounds.getCentreY() - 6, 12, 12);
    minimizeDot.setBounds(32, bounds.getCentreY() - 6, 12, 12);
    zoomDot.setBounds(50, bounds.getCentreY() - 6, 12, 12);

    // App title and pill badge
    titleLabel.setBounds(74, bounds.getY(), 105, bounds.getHeight());
    statusBadge.setBounds(titleLabel.getRight() + 2, bounds.getY() + 6, 56, bounds.getHeight() - 12);

    // Action buttons next to title
    openButton.setBounds(statusBadge.getRight() + 10, bounds.getY() + 2, 65, bounds.getHeight() - 4);
    pasteButton.setBounds(openButton.getRight() + 5, bounds.getY() + 2, 60, bounds.getHeight() - 4);
    syncYouTubeButton.setBounds(pasteButton.getRight() + 5, bounds.getY() + 2, 94, bounds.getHeight() - 4);
    scaleModeButton.setBounds(syncYouTubeButton.getRight() + 5, bounds.getY() + 2, 70, bounds.getHeight() - 4);

    // Right FPS counter
    const int fpsWidth = 75;
    fpsLabel.setBounds(bounds.getRight() - fpsWidth, bounds.getY() + 2, fpsWidth, bounds.getHeight() - 4);

    // Audio status badge
    const int audioWidth = 190;
    audioStatusBadge.setBounds(fpsLabel.getX() - audioWidth - 8, bounds.getY() + 2, audioWidth, bounds.getHeight() - 4);

    // Center output resolution preview buttons
    const int resBtnW = 46;
    int curX = scaleModeButton.getRight() + 10;
    resSquareButton.setBounds(curX, bounds.getY() + 2, resBtnW, bounds.getHeight() - 4);
    curX += resBtnW + 3;
    resPortraitButton.setBounds(curX, bounds.getY() + 2, resBtnW, bounds.getHeight() - 4);
    curX += resBtnW + 3;
    resLandscapeButton.setBounds(curX, bounds.getY() + 2, resBtnW, bounds.getHeight() - 4);
    curX += resBtnW + 6;

    // Fullscreen Button
    fullscreenButton.setBounds(curX, bounds.getY() + 2, 76, bounds.getHeight() - 4);
}

} // namespace drinkfloyd
