#include "YouTubeMediaSync.h"
#include "VisualCanvas.h"

#if JUCE_WINDOWS
#include <windows.h>
#endif

namespace drinkfloyd
{

juce::String YouTubeMediaSync::extractVideoId(const juce::String& text)
{
    // Check common patterns:
    // https://www.youtube.com/watch?v=XXXXXXXXXXX
    // https://youtu.be/XXXXXXXXXXX
    // https://music.youtube.com/watch?v=XXXXXXXXXXX
    juce::String t = text.trim();

    if (t.contains("youtu.be/"))
    {
        juce::String id = t.fromFirstOccurrenceOf("youtu.be/", false, false);
        if (id.contains("?"))
            id = id.upToFirstOccurrenceOf("?", false, false);
        if (id.contains("&"))
            id = id.upToFirstOccurrenceOf("&", false, false);
        if (id.length() >= 11)
            return id.substring(0, 11);
    }
    else if (t.contains("watch?v="))
    {
        juce::String id = t.fromFirstOccurrenceOf("watch?v=", false, false);
        if (id.contains("&"))
            id = id.upToFirstOccurrenceOf("&", false, false);
        if (id.contains("#"))
            id = id.upToFirstOccurrenceOf("#", false, false);
        if (id.length() >= 11)
            return id.substring(0, 11);
    }

    return {};
}

juce::String YouTubeMediaSync::cleanWindowTitle(const juce::String& rawTitle)
{
    juce::String title = rawTitle;

    // Remove common browser application names
    const juce::String toRemove[] = {
        "- Google Chrome",
        "- Microsoft Edge",
        "- Mozilla Firefox",
        "- Brave",
        "- Opera",
        "- Vivaldi",
        " - YouTube Music",
        " - YouTube",
        "- YouTube Music",
        "- YouTube",
        "\u2014 YouTube",
        "YouTube"
    };

    for (const auto& suffix : toRemove)
    {
        title = title.replace(suffix, "").trim();
    }

    // Clean up trailing dashes or spaces
    while (title.endsWithChar('-') || title.endsWithChar('|') || title.endsWithChar(' '))
        title = title.dropLastCharacters(1).trim();

    return title.isNotEmpty() ? title : "YouTube Track";
}

bool YouTubeMediaSync::tryExtractFromClipboard(SyncResult& result)
{
    juce::String clipText = juce::SystemClipboard::getTextFromClipboard();
    if (clipText.isEmpty())
        return false;

    juce::String videoId = extractVideoId(clipText);
    if (videoId.isEmpty())
        return false;

    // Attempt downloading high-res thumbnail
    const juce::String thumbUrls[] = {
        "https://img.youtube.com/vi/" + videoId + "/maxresdefault.jpg",
        "https://img.youtube.com/vi/" + videoId + "/hqdefault.jpg",
        "https://img.youtube.com/vi/" + videoId + "/0.jpg"
    };

    for (const auto& urlStr : thumbUrls)
    {
        juce::URL url(urlStr);
        std::unique_ptr<juce::InputStream> stream(url.createInputStream(
            juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                .withConnectionTimeoutMs(3000)
                .withNumRedirectsToFollow(3)));

        if (stream != nullptr)
        {
            juce::Image img = juce::ImageFileFormat::loadFrom(*stream);
            if (img.isValid() && img.getWidth() > 120 && img.getHeight() > 90)
            {
                result.success = true;
                result.image = img;
                result.videoTitle = "YouTube Video (" + videoId + ")";
                result.statusMessage = "Synced YouTube thumbnail via clipboard link";
                return true;
            }
        }
    }

    return false;
}

#if JUCE_WINDOWS

struct WindowMatch
{
    HWND hwnd = nullptr;
    juce::String title;
};

static BOOL CALLBACK EnumYouTubeWindowsProc(HWND hwnd, LPARAM lParam)
{
    if (!IsWindowVisible(hwnd) || IsIconic(hwnd))
        return TRUE;

    char titleBuf[1024] = { 0 };
    int len = GetWindowTextA(hwnd, titleBuf, sizeof(titleBuf) - 1);
    if (len > 0)
    {
        juce::String title(titleBuf);
        if (title.containsIgnoreCase("YouTube"))
        {
            auto* match = reinterpret_cast<WindowMatch*>(lParam);
            match->hwnd = hwnd;
            match->title = title;
            return FALSE; // Stop enumeration once found
        }
    }
    return TRUE;
}

static juce::Image captureWindowRegion(HWND hwnd, const juce::String& cleanTitle)
{
    RECT rc;
    if (!GetWindowRect(hwnd, &rc))
        return {};

    int winW = rc.right - rc.left;
    int winH = rc.bottom - rc.top;

    if (winW < 320 || winH < 240)
        return {};

    HDC screenDC = GetDC(NULL);
    if (screenDC == nullptr)
        return {};

    HDC memDC = CreateCompatibleDC(screenDC);
    HBITMAP memBmp = CreateCompatibleBitmap(screenDC, winW, winH);
    HGDIOBJ oldMemBmp = SelectObject(memDC, memBmp);

    // Capture window area from desktop
    BOOL bltOk = BitBlt(memDC, 0, 0, winW, winH, screenDC, rc.left, rc.top, SRCCOPY);
    if (!bltOk)
    {
        // Fallback to PrintWindow if BitBlt fails
        PrintWindow(hwnd, memDC, PW_CLIENTONLY);
    }

    // Determine video viewport coordinates:
    // YouTube player is typically in the upper 70% of the browser content area
    int cropX = juce::jmax(0, static_cast<int>(winW * 0.04));
    int cropY = juce::jmax(0, static_cast<int>(winH * 0.12));
    int cropW = juce::jmin(winW - cropX, static_cast<int>(winW * 0.70));
    int cropH = static_cast<int>(cropW * 9.0 / 16.0);

    if (cropY + cropH > winH - 40)
    {
        cropH = winH - cropY - 40;
        cropW = static_cast<int>(cropH * 16.0 / 9.0);
    }

    if (cropW <= 50 || cropH <= 50)
    {
        cropX = 0;
        cropY = 0;
        cropW = winW;
        cropH = winH;
    }

    // Extract DIB bits into juce::Image
    HDC cropDC = CreateCompatibleDC(screenDC);
    HBITMAP cropBmp = CreateCompatibleBitmap(screenDC, cropW, cropH);
    HGDIOBJ oldCropBmp = SelectObject(cropDC, cropBmp);

    BitBlt(cropDC, 0, 0, cropW, cropH, memDC, cropX, cropY, SRCCOPY);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = cropW;
    bmi.bmiHeader.biHeight = -cropH; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    juce::Image resultImg(juce::Image::ARGB, cropW, cropH, false);
    juce::Image::BitmapData destData(resultImg, juce::Image::BitmapData::writeOnly);

    GetDIBits(cropDC, cropBmp, 0, cropH, destData.data, &bmi, DIB_RGB_COLORS);

    // Ensure opaque alpha
    for (int y = 0; y < cropH; ++y)
    {
        juce::uint8* line = destData.getLinePointer(y);
        for (int x = 0; x < cropW; ++x)
        {
            line[3] = 255;
            line += 4;
        }
    }

    // Clean up Win32 GDI handles
    SelectObject(cropDC, oldCropBmp);
    DeleteObject(cropBmp);
    DeleteDC(cropDC);

    SelectObject(memDC, oldMemBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);

    ReleaseDC(NULL, screenDC);

    return resultImg;
}

#endif

bool YouTubeMediaSync::tryCaptureFromBrowserWindow(SyncResult& result)
{
#if JUCE_WINDOWS
    WindowMatch match;
    EnumWindows(EnumYouTubeWindowsProc, reinterpret_cast<LPARAM>(&match));

    if (match.hwnd != nullptr)
    {
        juce::String cleanTitle = cleanWindowTitle(match.title);
        juce::Image capturedImg = captureWindowRegion(match.hwnd, cleanTitle);

        if (capturedImg.isValid() && capturedImg.getWidth() > 100 && capturedImg.getHeight() > 100)
        {
            result.success = true;
            result.videoTitle = cleanTitle;
            result.image = capturedImg;
            result.statusMessage = "Synced active YouTube video: " + cleanTitle;
            return true;
        }
    }
#endif
    return false;
}

YouTubeMediaSync::SyncResult YouTubeMediaSync::performSync()
{
    SyncResult result;

    // Strategy 1: Check clipboard for YouTube link / ID
    if (tryExtractFromClipboard(result))
        return result;

    // Strategy 2: Scan desktop windows for active YouTube browser tab & capture video frame
    if (tryCaptureFromBrowserWindow(result))
        return result;

    result.success = false;
    result.statusMessage = "No active YouTube window or link detected. Play YouTube in your browser or copy a link, then click [Sync YouTube].";
    return result;
}

void YouTubeMediaSync::syncCurrentYouTube(VisualCanvas& canvas,
                                         std::function<void(bool success, const juce::String& message)> onComplete)
{
    // Run detection and snapshot asynchronously to avoid blocking UI thread
    juce::Thread::launch([&canvas, onComplete]()
    {
        SyncResult res = performSync();

        juce::MessageManager::callAsync([&canvas, onComplete, res]()
        {
            if (res.success && res.image.isValid())
            {
                canvas.setDirectImage(res.image, res.videoTitle);
            }

            if (onComplete != nullptr)
            {
                onComplete(res.success, res.statusMessage);
            }
        });
    });
}

} // namespace drinkfloyd
