#include "MainComponent.h"
#if JUCE_WINDOWS
#include <windows.h>
#endif

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
    visualCanvas.addKeyListener(this);
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
    isFullscreen = !isFullscreen;
    auto* topLevel = getTopLevelComponent();
    auto* dw = dynamic_cast<juce::DocumentWindow*>(topLevel);

#if JUCE_WINDOWS
    if (dw != nullptr)
    {
        HWND hwnd = reinterpret_cast<HWND>(dw->getWindowHandle());
        if (hwnd != nullptr)
        {
            if (isFullscreen)
            {
                savedWindowBounds = dw->getBounds();
                savedWindowStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
                savedWindowExStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

                RECT r = { 0, 0, 0, 0 };
                GetWindowRect(hwnd, &r);
                savedWindowRect[0] = r.left;
                savedWindowRect[1] = r.top;
                savedWindowRect[2] = r.right;
                savedWindowRect[3] = r.bottom;

                HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                MONITORINFO mi;
                mi.cbSize = sizeof(mi);
                GetMonitorInfo(hMon, &mi);

                // Strip native decorations (title bar, borders, sizing frame) without destroying HWND
                SetWindowLongPtr(hwnd, GWL_STYLE, savedWindowStyle & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU));
                SetWindowLongPtr(hwnd, GWL_EXSTYLE, savedWindowExStyle & ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE));

                // Position across the full monitor resolution
                SetWindowPos(hwnd, HWND_TOP,
                             mi.rcMonitor.left, mi.rcMonitor.top,
                             mi.rcMonitor.right - mi.rcMonitor.left,
                             mi.rcMonitor.bottom - mi.rcMonitor.top,
                             SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            }
            else
            {
                // Restore previous window styles and position
                SetWindowLongPtr(hwnd, GWL_STYLE, savedWindowStyle);
                SetWindowLongPtr(hwnd, GWL_EXSTYLE, savedWindowExStyle);
                SetWindowPos(hwnd, nullptr,
                             savedWindowRect[0], savedWindowRect[1],
                             savedWindowRect[2] - savedWindowRect[0],
                             savedWindowRect[3] - savedWindowRect[1],
                             SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

                if (!savedWindowBounds.isEmpty())
                    dw->setBounds(savedWindowBounds);
            }
        }
    }
#else
    if (dw != nullptr)
    {
        dw->setFullScreen(isFullscreen);
    }
#endif

    if (isFullscreen)
    {
        headerBar.setVisible(false);
        parameterPanel.setVisible(false);
        visualCanvas.setFullscreen(true);
    }
    else
    {
        headerBar.setVisible(true);
        parameterPanel.setVisible(true);
        visualCanvas.setFullscreen(false);
    }

    resized();
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
