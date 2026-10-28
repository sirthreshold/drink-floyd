#include "ImageLoader.h"

#include <windows.h>
#include <wincodec.h>
#include <shellapi.h>
#include <thread>
#include <algorithm>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

namespace drinkfloyd
{

bool ImageLoader::isSupportedExtension(const juce::String& extension)
{
    const auto ext = extension.toLowerCase();
    return ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".webp" || ext == ".bmp";
}

void ImageLoader::loadFileAsync(const juce::File& file, std::function<void(ImageImportResult)> callback)
{
    std::thread([file, callback]()
    {
        CoInitialize(nullptr);
        auto result = loadFile(file);
        CoUninitialize();

        juce::MessageManager::callAsync([callback, result]()
        {
            callback(result);
        });
    }).detach();
}

void ImageLoader::loadClipboardAsync(std::function<void(ImageImportResult)> callback)
{
    std::thread([callback]()
    {
        CoInitialize(nullptr);
        auto result = loadClipboard();
        CoUninitialize();

        juce::MessageManager::callAsync([callback, result]()
        {
            callback(result);
        });
    }).detach();
}

ImageImportResult ImageLoader::loadFile(const juce::File& file)
{
    ImageImportResult res;
    if (!file.existsAsFile())
    {
        res.success = false;
        res.errorMessage = "File not found: " + file.getFullPathName();
        return res;
    }

    juce::MemoryBlock block;
    if (!file.loadFileAsData(block) || block.getSize() == 0)
    {
        res.success = false;
        res.errorMessage = "Failed to read file contents: " + file.getFileName();
        return res;
    }

    return loadFromMemory(block.getData(), block.getSize(), file.getFileName());
}

ImageImportResult ImageLoader::loadFromMemory(const void* data, size_t size, const juce::String& name)
{
    ImageImportResult res;
    if (data == nullptr || size == 0)
    {
        res.success = false;
        res.errorMessage = "Invalid image buffer.";
        return res;
    }

    // 1. Try Windows Imaging Component (WIC) - natively supports JPG, PNG, WebP, BMP
    juce::Image image = decodeWithWIC(data, size);

    // 2. Fall back to JUCE's built-in loaders (PNG/JPEG) if WIC returned invalid
    if (!image.isValid())
    {
        image = juce::ImageFileFormat::loadFrom(data, size);
    }

    if (!image.isValid())
    {
        res.success = false;
        res.errorMessage = "Unsupported or corrupt image format: " + name;
        return res;
    }

    return processLoadedImage(image, name);
}

juce::Image ImageLoader::decodeWithWIC(const void* data, size_t size)
{
    if (data == nullptr || size == 0)
        return {};

    IWICImagingFactory* pFactory = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));
    if (FAILED(hr) || !pFactory)
        return {};

    IWICStream* pStream = nullptr;
    hr = pFactory->CreateStream(&pStream);
    if (FAILED(hr) || !pStream)
    {
        pFactory->Release();
        return {};
    }

    hr = pStream->InitializeFromMemory(reinterpret_cast<BYTE*>(const_cast<void*>(data)), static_cast<DWORD>(size));
    if (FAILED(hr))
    {
        pStream->Release();
        pFactory->Release();
        return {};
    }

    IWICBitmapDecoder* pDecoder = nullptr;
    hr = pFactory->CreateDecoderFromStream(pStream, nullptr, WICDecodeMetadataCacheOnDemand, &pDecoder);
    if (FAILED(hr) || !pDecoder)
    {
        pStream->Release();
        pFactory->Release();
        return {};
    }

    IWICBitmapFrameDecode* pFrame = nullptr;
    hr = pDecoder->GetFrame(0, &pFrame);
    if (FAILED(hr) || !pFrame)
    {
        pDecoder->Release();
        pStream->Release();
        pFactory->Release();
        return {};
    }

    UINT width = 0, height = 0;
    pFrame->GetSize(&width, &height);
    if (width == 0 || height == 0)
    {
        pFrame->Release();
        pDecoder->Release();
        pStream->Release();
        pFactory->Release();
        return {};
    }

    IWICFormatConverter* pConverter = nullptr;
    hr = pFactory->CreateFormatConverter(&pConverter);
    if (FAILED(hr) || !pConverter)
    {
        pFrame->Release();
        pDecoder->Release();
        pStream->Release();
        pFactory->Release();
        return {};
    }

    hr = pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr))
    {
        pConverter->Release();
        pFrame->Release();
        pDecoder->Release();
        pStream->Release();
        pFactory->Release();
        return {};
    }

    juce::Image image(juce::Image::ARGB, static_cast<int>(width), static_cast<int>(height), false);
    {
        juce::Image::BitmapData bdata(image, juce::Image::BitmapData::writeOnly);
        const UINT stride = static_cast<UINT>(bdata.lineStride);
        const UINT totalBytes = stride * height;
        pConverter->CopyPixels(nullptr, stride, totalBytes, bdata.data);
    }

    pConverter->Release();
    pFrame->Release();
    pDecoder->Release();
    pStream->Release();
    pFactory->Release();

    return image;
}

ImageImportResult ImageLoader::loadClipboard()
{
    ImageImportResult res;

    if (!OpenClipboard(nullptr))
    {
        res.success = false;
        res.errorMessage = "Unable to open Windows clipboard (may be locked by another application).";
        return res;
    }

    // 1. Check if files were copied in Explorer (CF_HDROP)
    if (HANDLE hDrop = GetClipboardData(CF_HDROP))
    {
        HDROP drop = static_cast<HDROP>(hDrop);
        UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
        if (count > 0)
        {
            WCHAR filePath[MAX_PATH];
            if (DragQueryFileW(drop, 0, filePath, MAX_PATH) > 0)
            {
                CloseClipboard();
                const juce::File targetFile { juce::String(filePath) };
                if (isSupportedExtension(targetFile.getFileExtension()))
                {
                    return loadFile(targetFile);
                }
            }
        }
    }

    // 2. Check for image data (CF_BITMAP or synthesized from CF_DIB / CF_DIBV5)
    HBITMAP hBitmap = static_cast<HBITMAP>(GetClipboardData(CF_BITMAP));
    if (hBitmap != nullptr)
    {
        BITMAP bmp = {};
        if (GetObject(hBitmap, sizeof(BITMAP), &bmp))
        {
            const int width = bmp.bmWidth;
            const int height = bmp.bmHeight;
            if (width > 0 && height > 0)
            {
                BITMAPINFO bi = {};
                bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bi.bmiHeader.biWidth = width;
                bi.bmiHeader.biHeight = -height; // top-down orientation
                bi.bmiHeader.biPlanes = 1;
                bi.bmiHeader.biBitCount = 32;
                bi.bmiHeader.biCompression = BI_RGB;

                juce::Image image(juce::Image::ARGB, width, height, false);
                {
                    juce::Image::BitmapData bdata(image, juce::Image::BitmapData::writeOnly);
                    HDC hdc = GetDC(nullptr);
                    GetDIBits(hdc, hBitmap, 0, static_cast<UINT>(height), bdata.data, &bi, DIB_RGB_COLORS);
                    ReleaseDC(nullptr, hdc);

                    // Ensure opaque alpha if all alpha bytes are 0 (standard GDI clipboard behavior)
                    bool hasAlpha = false;
                    for (int y = 0; y < height && !hasAlpha; ++y)
                    {
                        const uint8_t* row = bdata.getPixelPointer(0, y);
                        for (int x = 0; x < width; ++x)
                        {
                            if (row[x * 4 + 3] != 0)
                            {
                                hasAlpha = true;
                                break;
                            }
                        }
                    }
                    if (!hasAlpha)
                    {
                        for (int y = 0; y < height; ++y)
                        {
                            uint8_t* row = const_cast<uint8_t*>(bdata.getPixelPointer(0, y));
                            for (int x = 0; x < width; ++x)
                            {
                                row[x * 4 + 3] = 255;
                            }
                        }
                    }
                }
                CloseClipboard();
                return processLoadedImage(image, "Clipboard Screenshot");
            }
        }
    }

    CloseClipboard();

    res.success = false;
    res.errorMessage = "Clipboard does not contain image data (copied text or empty).";
    return res;
}

ImageImportResult ImageLoader::processLoadedImage(juce::Image image, const juce::String& sourceName)
{
    ImageImportResult res;
    if (!image.isValid())
    {
        res.success = false;
        res.errorMessage = "Failed to process image data.";
        return res;
    }

    const int origW = image.getWidth();
    const int origH = image.getHeight();
    res.originalWidth = origW;
    res.originalHeight = origH;
    res.sourceName = sourceName;
    res.success = true;

    // Handle images larger than 4096 in either dimension
    if (origW > MaxTextureDimension || origH > MaxTextureDimension)
    {
        const float scale = std::min(static_cast<float>(MaxTextureDimension) / static_cast<float>(origW),
                                     static_cast<float>(MaxTextureDimension) / static_cast<float>(origH));
        const int targetW = std::clamp(juce::roundToInt(static_cast<float>(origW) * scale), 1, MaxTextureDimension);
        const int targetH = std::clamp(juce::roundToInt(static_cast<float>(origH) * scale), 1, MaxTextureDimension);

        res.image = image.rescaled(targetW, targetH, juce::Graphics::highResamplingQuality);
        res.finalWidth = targetW;
        res.finalHeight = targetH;
        res.wasDownscaled = true;
        res.statusMessage = sourceName + " (" + juce::String(origW) + "x" + juce::String(origH)
                          + " -> downscaled to " + juce::String(targetW) + "x" + juce::String(targetH)
                          + " max texture limit)";
    }
    else
    {
        res.image = image;
        res.finalWidth = origW;
        res.finalHeight = origH;
        res.wasDownscaled = false;
        res.statusMessage = sourceName + " (" + juce::String(origW) + "x" + juce::String(origH) + ")";
    }

    return res;
}

} // namespace drinkfloyd
