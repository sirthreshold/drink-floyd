#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

namespace drinkfloyd
{

class HeaderBar;

class VisualCanvas : public juce::Component,
                     private juce::Timer
{
public:
    explicit VisualCanvas(HeaderBar& headerBar);
    ~VisualCanvas() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    HeaderBar& header;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisualCanvas)
};

} // namespace drinkfloyd
