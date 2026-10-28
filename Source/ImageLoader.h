#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>

namespace drinkfloyd
{

struct ImageImportResult
{
    juce::Image image;
    juce::String sourceName;      // File name or "Clipboard Screenshot"
    int originalWidth = 0;
    int originalHeight = 0;
    int finalWidth = 0;
    int finalHeight = 0;
    bool wasDownscaled = false;
    bool success = false;
    juce::String statusMessage;
    juce::String errorMessage;
};

class ImageLoader
{
public:
    static constexpr int MaxTextureDimension = 4096;

    // Asynchronously loads an image from a file (JPG, PNG, WebP, BMP)
    static void loadFileAsync(const juce::File& file,
                              std::function<void(ImageImportResult)> callback);

    // Asynchronously imports an image from the Windows clipboard
    static void loadClipboardAsync(std::function<void(ImageImportResult)> callback);

    // Synchronous loaders (designed to run on background worker threads)
    static ImageImportResult loadFile(const juce::File& file);
    static ImageImportResult loadClipboard();
    static ImageImportResult loadFromMemory(const void* data, size_t size, const juce::String& name);

    // Native Windows Imaging Component (WIC) decoder (JPG, PNG, WebP, BMP)
    static juce::Image decodeWithWIC(const void* data, size_t size);

    // Post-processes image: clamps to MaxTextureDimension, downscaling if needed
    static ImageImportResult processLoadedImage(juce::Image image, const juce::String& sourceName);

    // Checks if a file extension is supported
    static bool isSupportedExtension(const juce::String& extension);
};

} // namespace drinkfloyd
