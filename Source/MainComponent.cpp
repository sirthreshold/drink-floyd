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

    headerBar.onToggleFullscreen = [this]()
    {
        toggleFullscreen();
    };

    visualCanvas.onToggleFullscreen = [this]()
    {
        toggleFullscreen();
    };

    // Start background WASAPI system audio loopback capture
    audioCaptureEngine.start();

    // Keyboard shortcuts (Ctrl+V, Ctrl+O, F11, F, Esc)
    addKeyListener(this);
    setWantsKeyboardFocus(true);

    setSize(1280, 800);
}

MainComponent::~MainComponent()
{
    audioCaptureEngine.stop();
    removeKeyListener(this);
}

void MainComponent::toggleFullscreen()
{
    auto* topLevel = getTopLevelComponent();
    if (auto* dw = dynamic_cast<juce::DocumentWindow*>(topLevel))
    {
        isFullscreen = !isFullscreen;
        if (isFullscreen)
        {
            savedWindowBounds = dw->getBounds();
            headerBar.setVisible(false);
            parameterPanel.setVisible(false);
            visualCanvas.setFullscreen(true);
            dw->setFullScreen(true);
        }
        else
        {
            dw->setFullScreen(false);
            if (!savedWindowBounds.isEmpty())
                dw->setBounds(savedWindowBounds);
            headerBar.setVisible(true);
            parameterPanel.setVisible(true);
            visualCanvas.setFullscreen(false);
        }
        resized();
    }
}

void MainComponent::exitFullscreen()
{
    if (isFullscreen)
    {
        toggleFullscreen();
    }
}

bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component* /*originatingComponent*/)
{
    // Esc: exit fullscreen
    if (key.isKeyCode(juce::KeyPress::escapeKey))
    {
        if (isFullscreen)
        {
            exitFullscreen();
            return true;
        }
    }

    // F11: toggle fullscreen
    if (key.isKeyCode(juce::KeyPress::F11Key))
    {
        toggleFullscreen();
        return true;
    }

    // F: toggle fullscreen (when no modifiers pressed)
    if (!key.getModifiers().isAnyModifierKeyDown() && (key.getTextCharacter() == 'f' || key.getTextCharacter() == 'F'))
    {
        toggleFullscreen();
        return true;
    }

    // Ctrl shortcuts
    if (key.getModifiers().isCommandDown())
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

    if (isFullscreen)
    {
        visualCanvas.setBounds(area);
    }
    else
    {
        // Top Header: 46px height
        headerBar.setBounds(area.removeFromTop(46));

        // Right Sidebar (Parameter Panel): fixed 320px width
        parameterPanel.setBounds(area.removeFromRight(320));

        // Remaining area: Visual Canvas (centered letterbox within this area)
        visualCanvas.setBounds(area);
    }
}

} // namespace drinkfloyd
