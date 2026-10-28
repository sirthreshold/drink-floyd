#include "MainComponent.h"

namespace drinkfloyd
{

MainComponent::MainComponent()
    : audioCaptureEngine(audioAnalysisEngine),
      headerBar(audioAnalysisEngine),
      visualCanvas(headerBar, audioAnalysisEngine),
      parameterPanel(audioAnalysisEngine, visualCanvas)
{
    addAndMakeVisible(headerBar);
    addAndMakeVisible(visualCanvas);
    addAndMakeVisible(parameterPanel);

    // Wire HeaderBar buttons to VisualCanvas actions
    headerBar.onOpenImage = [this]()
    {
        visualCanvas.openFileDialog();
    };

    headerBar.onPasteImage = [this]()
    {
        visualCanvas.pasteFromClipboard();
    };

    headerBar.onToggleScaleMode = [this]()
    {
        visualCanvas.toggleScaleMode();
        headerBar.updateScaleMode(visualCanvas.getScaleMode());
    };

    headerBar.onAspectChanged = [this](int idx)
    {
        if (idx == 0) visualCanvas.setPreviewAspect(PreviewAspect::Square1080x1080);
        else if (idx == 1) visualCanvas.setPreviewAspect(PreviewAspect::Portrait1080x1920);
        else if (idx == 2) visualCanvas.setPreviewAspect(PreviewAspect::Landscape1920x1080);
    };

    // Start background WASAPI system audio loopback capture
    audioCaptureEngine.start();

    // Keyboard shortcuts (Ctrl+V paste, Ctrl+O open)
    addKeyListener(this);
    setWantsKeyboardFocus(true);

    setSize(1280, 800);
}

MainComponent::~MainComponent()
{
    audioCaptureEngine.stop();
    removeKeyListener(this);
}

bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component* /*originatingComponent*/)
{
    if (key.getModifiers().isCommandDown()) // Ctrl on Windows
    {
        if (key.getKeyCode() == 'V' || key.getKeyCode() == 'v')
        {
            visualCanvas.pasteFromClipboard();
            return true;
        }

        if (key.getKeyCode() == 'O' || key.getKeyCode() == 'o')
        {
            visualCanvas.openFileDialog();
            return true;
        }
    }

    return false;
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0e1015));
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    // Top Header: 46px height
    headerBar.setBounds(area.removeFromTop(46));

    // Right Sidebar (Parameter Panel): fixed 320px width
    parameterPanel.setBounds(area.removeFromRight(320));

    // Remaining area: Visual Canvas
    visualCanvas.setBounds(area);
}

} // namespace drinkfloyd
