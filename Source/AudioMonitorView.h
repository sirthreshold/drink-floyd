#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "AudioAnalysisEngine.h"

namespace drinkfloyd
{

class AudioMonitorView : public juce::Component,
                         private juce::Timer
{
public:
    explicit AudioMonitorView(AudioAnalysisEngine& engine);
    ~AudioMonitorView() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    AudioAnalysisEngine& analysis;
    AudioAnalysisSnapshot snapshot;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioMonitorView)
};

} // namespace drinkfloyd
