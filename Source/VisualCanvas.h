#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_opengl/juce_opengl.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include "ImageLoader.h"
#include "VisualParameters.h"

namespace drinkfloyd
{

class HeaderBar;
class AudioAnalysisEngine;

enum class ScaleMode
{
    Fit,   // Keep aspect ratio, letterbox / pillarbox (Default)
    Cover  // Keep aspect ratio, fill canvas completely, crop overflow
};

enum class PreviewAspect
{
    Square1080x1080,   // 1:1
    Portrait1080x1920, // 9:16
    Landscape1920x1080 // 16:9
};

class VisualCanvas : public juce::Component,
                     public juce::OpenGLRenderer,
                     public juce::FileDragAndDropTarget
{
public:
    VisualCanvas(HeaderBar& headerBar, AudioAnalysisEngine& analysisEngine);
    ~VisualCanvas() override;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

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

    void setPreviewAspect(PreviewAspect aspect);
    PreviewAspect getPreviewAspect() const noexcept { return currentPreviewAspect; }

    void setParameters(const VisualParameters& params);
    VisualParameters getParameters() const;
    void setScene(SceneType scene);

    void setFullscreen(bool fs);
    bool getFullscreen() const noexcept { return isFullscreen; }

    bool hasLoadedImage() const noexcept { return hasImage; }
    juce::String getStatusText() const { return currentStatusText; }

    std::function<void()> onToggleFullscreen;

private:
    void onImageLoaded(const ImageImportResult& result);
    void initShaders();
    void releaseFBOs();
    void initBlockMesh();
    void releaseBlockMesh();

    HeaderBar& header;
    AudioAnalysisEngine& analysis;

    // OpenGL Context & Pipeline
    juce::OpenGLContext openGLContext;
    juce::OpenGLTexture sourceTexture;
    juce::OpenGLTexture defaultPatternTexture;

    // Ping-Pong FBOs for frame feedback & trails
    std::unique_ptr<juce::OpenGLFrameBuffer> fboPing;
    std::unique_ptr<juce::OpenGLFrameBuffer> fboPong;
    int fboWidth = 0;
    int fboHeight = 0;

    // Shader Programs
    std::unique_ptr<juce::OpenGLShaderProgram> warpShader;
    std::unique_ptr<juce::OpenGLShaderProgram> meltScatterShader;
    std::unique_ptr<juce::OpenGLShaderProgram> trailsFeedbackShader;
    std::unique_ptr<juce::OpenGLShaderProgram> presentShader;

    // Block Mesh VBO/IBO for Pixel Melt & Scatter
    GLuint blockVbo = 0;
    GLuint blockIbo = 0;
    int blockCount = 0;
    int gridResolution = 180; // 180x180 at 1080p -> default ~6px blocks
    float currentPixelSize = 6.0f;

    // Visual Parameters
    mutable std::mutex paramMutex;
    VisualParameters parameters;

    // Thread-safe image handoff to OpenGL render thread
    std::mutex imageMutex;
    juce::Image pendingImage;
    std::atomic<bool> newTexturePending { false };
    std::atomic<bool> hasImage { false };

    // Display & State
    ScaleMode currentScaleMode = ScaleMode::Fit;
    PreviewAspect currentPreviewAspect = PreviewAspect::Square1080x1080;
    bool isFullscreen = false;
    bool isDragOver = false;
    bool isLoading = false;
    int imageWidth = 0;
    int imageHeight = 0;
    juce::String currentSourceName;
    juce::String currentStatusText;
    bool isStatusError = false;

    double startTime = 0.0;

    // FileChooser owner
    std::unique_ptr<juce::FileChooser> activeFileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisualCanvas)
};

} // namespace drinkfloyd
