#include "MainComponent.h"

namespace drinkfloyd
{

MainComponent::MainComponent()
    : visualCanvas(headerBar)
{
    addAndMakeVisible(headerBar);
    addAndMakeVisible(visualCanvas);
    addAndMakeVisible(parameterPanel);

    // Initial default size for a desktop visualizer window
    setSize(1280, 800);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0e1015));
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    // 1. Header Bar at the top (height 44px)
    constexpr int headerHeight = 44;
    headerBar.setBounds(area.removeFromTop(headerHeight));

    // 2. Right parameter panel (fixed comfortable width of 340px)
    constexpr int parameterPanelWidth = 340;
    parameterPanel.setBounds(area.removeFromRight(parameterPanelWidth));

    // 3. Left remaining area is the large visual canvas
    visualCanvas.setBounds(area);
}

} // namespace drinkfloyd
