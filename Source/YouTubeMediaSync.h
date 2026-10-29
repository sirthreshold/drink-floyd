#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>

namespace drinkfloyd
{

class VisualCanvas;

class YouTubeMediaSync
{
public:
    struct SyncResult
    {
        bool success = false;
        juce::String videoTitle;
        juce::String statusMessage;
        juce::Image image;
    };

    // Asynchronously detect running YouTube video and update VisualCanvas
    static void syncCurrentYouTube(VisualCanvas& canvas, 
                                   std::function<void(bool success, const juce::String& message)> onComplete = nullptr);

    // Synchronous capture/fetch (called from background thread)
    static SyncResult performSync();

private:
    static bool tryExtractFromClipboard(SyncResult& result);
    static bool tryCaptureFromBrowserWindow(SyncResult& result);
    static juce::String extractVideoId(const juce::String& text);
    static juce::String cleanWindowTitle(const juce::String& rawTitle);
};

} // namespace drinkfloyd
