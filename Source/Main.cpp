#include <juce_gui_extra/juce_gui_extra.h>
#include "MainComponent.h"

namespace drinkfloyd
{

class MainWindow : public juce::DocumentWindow
{
public:
    explicit MainWindow(const juce::String& name)
        : DocumentWindow(name,
                         juce::Colour(0xff0e1015),
                         DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new MainComponent(), true);

        #if JUCE_IOS || JUCE_ANDROID
        setFullScreen(true);
        #else
        setResizable(true, true);
        setResizeLimits(960, 600, 3840, 2160);
        centreWithSize(getWidth(), getHeight());
        #endif

        setVisible(true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

class DrinkFloydApplication : public juce::JUCEApplication
{
public:
    DrinkFloydApplication() = default;

    const juce::String getApplicationName() override       { return "Drink Floyd"; }
    const juce::String getApplicationVersion() override    { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    void initialise(const juce::String& /*commandLineParameters*/) override
    {
        mainWindow = std::make_unique<MainWindow>(getApplicationName());
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String& /*commandLineParameters*/) override
    {
    }

private:
    std::unique_ptr<MainWindow> mainWindow;
};

} // namespace drinkfloyd

// ==============================================================================
// JUCE Application Entry Point
// ==============================================================================
START_JUCE_APPLICATION(drinkfloyd::DrinkFloydApplication)
