#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

namespace drinkfloyd
{

class HeaderBar : public juce::Component,
                  private juce::Timer
{
public:
    HeaderBar();
    ~HeaderBar() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void recordFrame();

private:
    void timerCallback() override;

    juce::Label titleLabel;
    juce::Label statusBadge;
    juce::Label fpsLabel;
    juce::Label resolutionInfo;

    double lastFrameTime = 0.0;
    int frameCount = 0;
    double fpsAccumulator = 0.0;
    double currentFps = 60.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderBar)
};

} // namespace drinkfloyd
