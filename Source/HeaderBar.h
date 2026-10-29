#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>

namespace drinkfloyd
{

enum class ScaleMode;
enum class PreviewAspect;
class AudioAnalysisEngine;

class HeaderBar : public juce::Component,
                  private juce::Timer
{
public:
    explicit HeaderBar(AudioAnalysisEngine& analysisEngine);
    ~HeaderBar() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void recordFrame();
    void updateScaleMode(ScaleMode mode);
    void updateAspectButtons(int activeIndex);

    std::function<void()> onOpenImage;
    std::function<void()> onPasteImage;
    std::function<void()> onSyncYouTube;
    std::function<void()> onToggleScaleMode;
    std::function<void(int)> onAspectChanged;
    std::function<void()> onToggleFullscreen;

private:
    void timerCallback() override;

    // macOS Traffic Light Dots
    juce::TextButton closeDot;
    juce::TextButton minimizeDot;
    juce::TextButton zoomDot;

    juce::Label titleLabel;
    juce::Label statusBadge;
    juce::Label audioStatusBadge;
    juce::TextButton openButton;
    juce::TextButton pasteButton;
    juce::TextButton syncYouTubeButton;
    juce::TextButton scaleModeButton;
    juce::TextButton fullscreenButton;

    // Output Preview Aspect Selectors
    juce::TextButton resSquareButton;    // 1080x1080 (1:1)
    juce::TextButton resPortraitButton;  // 1080x1920 (9:16)
    juce::TextButton resLandscapeButton; // 1920x1080 (16:9)

    juce::Label fpsLabel;

    AudioAnalysisEngine& analysis;

    double lastFrameTime = 0.0;
    int frameCount = 0;
    double currentFps = 60.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderBar)
};

} // namespace drinkfloyd
