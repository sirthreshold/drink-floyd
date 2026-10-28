#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_opengl/juce_opengl.h>
#include <atomic>
#include <memory>
#include <mutex>
#include "ImageLoader.h"

namespace drinkfloyd
{

class HeaderBar;
class AudioAnalysisEngine;

enum class ScaleMode
{
    Fit,   // Keep aspect ratio, letterbox / pillarbox (Default)
    Cover  // Keep aspect ratio, fill canvas completely, crop overflow
};

class VisualCanvas : public juce::Component,
                     public juce::OpenGLRenderer,
                     public juce::FileDragAndDropTarget,
                     private juce::Timer
{
public:
    VisualCanvas(HeaderBar& headerBar, AudioAnalysisEngine& analysisEngine);
    ~VisualCanvas() override;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    // OpenGLRenderer callbacks
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    // FileDragAndDropTarget callbacks
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragMove(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    // Public actions
    void openFileDialog();
    void pasteFromClipboard();
    void setScaleMode(ScaleMode mode);
    void toggleScaleMode();
    ScaleMode getScaleMode() const noexcept { return currentScaleMode; }

    bool hasLoadedImage() const noexcept { return hasImage; }
    juce::String getStatusText() const { return currentStatusText; }

private:
    void timerCallback() override;
    void onImageLoaded(const ImageImportResult& result);
    void initShader();

    HeaderBar& header;
    AudioAnalysisEngine& analysis;

    // OpenGL Context & Texture
    juce::OpenGLContext openGLContext;
    juce::OpenGLTexture openGlTexture;
    std::unique_ptr<juce::OpenGLShaderProgram> shaderProgram;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> textureUniform;

    // Thread-safe image handoff to OpenGL render thread
    std::mutex imageMutex;
    juce::Image pendingImage;
    std::atomic<bool> newTexturePending { false };
    std::atomic<bool> hasImage { false };

    // Display & State
    ScaleMode currentScaleMode = ScaleMode::Fit;
    bool isDragOver = false;
    bool isLoading = false;
    int imageWidth = 0;
    int imageHeight = 0;
    juce::String currentSourceName;
    juce::String currentStatusText;
    bool isStatusError = false;

    // FileChooser owner
    std::unique_ptr<juce::FileChooser> activeFileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisualCanvas)
};

} // namespace drinkfloyd
