#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "HeaderBar.h"
#include "VisualCanvas.h"
#include "ParameterPanel.h"

namespace drinkfloyd
{

class MainComponent : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    HeaderBar      headerBar;
    VisualCanvas   visualCanvas;
    ParameterPanel parameterPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace drinkfloyd
