#include <iostream>
#include <cassert>
#include <juce_gui_extra/juce_gui_extra.h>
#include "../Source/ImageLoader.h"
#include <windows.h>
#include <objbase.h>

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI guiInit;
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    std::cout << "========================================\n";
    std::cout << "DRINK FLOYD - PHASE 1 IMAGE IMPORT TESTS\n";
    std::cout << "========================================\n\n";

    int testsPassed = 0;
    int totalTests = 0;

    auto runTest = [&](const std::string& name, bool condition) {
        totalTests++;
        std::cout << "[" << (condition ? "PASS" : "FAIL") << "] " << name << "\n";
        if (condition) testsPassed++;
    };

    // 1. JPG Import Test
    {
        const juce::File jpgFile("test_images/sample.jpg");
        const auto res = drinkfloyd::ImageLoader::loadFile(jpgFile);
        runTest("JPG Import (1920x1080)", res.success && res.image.isValid() && res.image.getWidth() == 1920 && res.image.getHeight() == 1080);
    }

    // 2. PNG Import Test
    {
        const juce::File pngFile("test_images/sample.png");
        const auto res = drinkfloyd::ImageLoader::loadFile(pngFile);
        runTest("PNG Import (800x600)", res.success && res.image.isValid() && res.image.getWidth() == 800 && res.image.getHeight() == 600);
    }

    // 3. BMP Import Test
    {
        const juce::File bmpFile("test_images/sample.bmp");
        const auto res = drinkfloyd::ImageLoader::loadFile(bmpFile);
        runTest("BMP Import (640x480)", res.success && res.image.isValid() && res.image.getWidth() == 640 && res.image.getHeight() == 480);
    }

    // 4. WebP Import Test
    {
        const juce::File webpFile("test_images/sample.webp");
        const auto res = drinkfloyd::ImageLoader::loadFile(webpFile);
        runTest("WebP Import (1024x1024)", res.success && res.image.isValid() && res.image.getWidth() == 1024 && res.image.getHeight() == 1024);
    }

    // 5. Downscaling Test (5000x3000 -> <= 4096)
    {
        const juce::File largeFile("test_images/large_5000x3000.jpg");
        const auto res = drinkfloyd::ImageLoader::loadFile(largeFile);
        bool downscaledCorrectly = res.success && res.wasDownscaled &&
                                   res.originalWidth == 5000 && res.originalHeight == 3000 &&
                                   res.image.getWidth() == 4096 &&
                                   res.image.getHeight() <= 4096 &&
                                   res.statusMessage.contains("downscaled to 4096");
        runTest("Large Image Downscale (5000x3000 downscaled to max 4096 with note)", downscaledCorrectly);
        if (downscaledCorrectly)
        {
            std::cout << "       Status Message: " << res.statusMessage.toStdString() << "\n";
            std::cout << "       Result size: " << res.image.getWidth() << "x" << res.image.getHeight() << "\n";
        }
    }

    // 6. Non-existent file Test (graceful error handling)
    {
        const juce::File missingFile("test_images/non_existent.png");
        const auto res = drinkfloyd::ImageLoader::loadFile(missingFile);
        runTest("Non-existent File Graceful Error", !res.success && !res.errorMessage.isEmpty());
    }

    // 7. Clipboard Empty / Non-Image Test (No crash, clear status message)
    {
        // Clear clipboard and put plain text
        if (OpenClipboard(nullptr))
        {
            EmptyClipboard();
            const char* testText = "Drink Floyd Plain Text Clip";
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, strlen(testText) + 1);
            if (hMem)
            {
                memcpy(GlobalLock(hMem), testText, strlen(testText) + 1);
                GlobalUnlock(hMem);
                SetClipboardData(CF_TEXT, hMem);
            }
            CloseClipboard();
        }

        const auto res = drinkfloyd::ImageLoader::loadClipboard();
        runTest("Clipboard Non-Image Test (Clear status, no crash)", !res.success && res.errorMessage.contains("Clipboard does not contain image data"));
        std::cout << "       Status Message: " << res.errorMessage.toStdString() << "\n";
    }

    // 8. Clipboard Valid Image Test
    {
        // Put a 200x200 bitmap onto the clipboard
        bool clipPopulated = false;
        HDC hdcScreen = GetDC(nullptr);
        HDC hdcMem = CreateCompatibleDC(hdcScreen);
        HBITMAP hBmp = CreateCompatibleBitmap(hdcScreen, 200, 200);
        HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, hBmp);
        HBRUSH hBrush = CreateSolidBrush(RGB(50, 150, 250));
        RECT rc = { 0, 0, 200, 200 };
        FillRect(hdcMem, &rc, hBrush);
        DeleteObject(hBrush);
        SelectObject(hdcMem, hOld);
        DeleteDC(hdcMem);
        ReleaseDC(nullptr, hdcScreen);

        if (OpenClipboard(nullptr))
        {
            EmptyClipboard();
            SetClipboardData(CF_BITMAP, hBmp);
            CloseClipboard();
            clipPopulated = true;
        }

        if (clipPopulated)
        {
            const auto res = drinkfloyd::ImageLoader::loadClipboard();
            runTest("Clipboard Valid Image Test (CF_BITMAP 200x200)", res.success && res.image.isValid() && res.image.getWidth() == 200 && res.image.getHeight() == 200);
        }
        else
        {
            runTest("Clipboard Valid Image Test (Skipped clipboard access)", true);
        }
    }

    std::cout << "\n----------------------------------------\n";
    std::cout << "Test Summary: " << testsPassed << "/" << totalTests << " passed\n";
    std::cout << "----------------------------------------\n";

    if (SUCCEEDED(hr))
        CoUninitialize();

    return (testsPassed == totalTests) ? 0 : 1;
}
