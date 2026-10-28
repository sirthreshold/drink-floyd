#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>

namespace drinkfloyd
{

enum class ScaleMode;
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

    std::function<void()> onOpenImage;
    std::function<void()> onPasteImage;
    std::function<void()> onToggleScaleMode;

private:
    void timerCallback() override;

    juce::Label titleLabel;
    juce::Label statusBadge;
    juce::Label audioStatusBadge;
    juce::TextButton openButton;
    juce::TextButton pasteButton;
    juce::TextButton scaleModeButton;
    juce::Label fpsLabel;
    juce::Label resolutionInfo;

    AudioAnalysisEngine& analysis;

    double lastFrameTime = 0.0;
    int frameCount = 0;
    double fpsAccumulator = 0.0;
    double currentFps = 60.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderBar)
};

} // namespace drinkfloyd
