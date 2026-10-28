#include "VisualCanvas.h"
#include "HeaderBar.h"
#include "AudioAnalysisEngine.h"

// Bring JUCE's OpenGL function pointers and constants into scope
using namespace juce::gl;

namespace drinkfloyd
{

// ==============================================================================
// Shaders for textured quad rendering
// ==============================================================================
static const char* vertexShaderSource = R"(
    attribute vec2 aPosition;
    attribute vec2 aTexCoord;
    varying vec2 vTexCoord;

    void main()
    {
        gl_Position = vec4(aPosition, 0.0, 1.0);
        vTexCoord = aTexCoord;
    }
)";

static const char* fragmentShaderSource = R"(
    varying vec2 vTexCoord;
    uniform sampler2D uTexture;

    void main()
    {
        gl_FragColor = texture2D(uTexture, vTexCoord);
    }
)";

// ==============================================================================
// VisualCanvas Implementation
// ==============================================================================
VisualCanvas::VisualCanvas(HeaderBar& headerBar, AudioAnalysisEngine& analysisEngine)
    : header(headerBar),
      analysis(analysisEngine),
      currentStatusText("Ready. No image loaded — drag & drop an image, click Open, or press Ctrl+V.")
{
    setOpaque(true);

    // Setup OpenGL context
    openGLContext.setComponentPaintingEnabled(true);
    openGLContext.setContinuousRepainting(false);
    openGLContext.setRenderer(this);
    openGLContext.attachTo(*this);

    // 60 Hz frame rate timer
    startTimerHz(60);
}

VisualCanvas::~VisualCanvas()
{
    stopTimer();
    openGLContext.detach();
}

void VisualCanvas::timerCallback()
{
    header.recordFrame();
}

// ==============================================================================
// OpenGL Callbacks
// ==============================================================================
void VisualCanvas::newOpenGLContextCreated()
{
    initShader();
}

void VisualCanvas::initShader()
{
    shaderProgram = std::make_unique<juce::OpenGLShaderProgram>(openGLContext);

    if (shaderProgram->addVertexShader(vertexShaderSource)
        && shaderProgram->addFragmentShader(fragmentShaderSource)
        && shaderProgram->link())
    {
        textureUniform = std::make_unique<juce::OpenGLShaderProgram::Uniform>(*shaderProgram, "uTexture");
    }
    else
    {
        shaderProgram.reset();
    }
}

void VisualCanvas::openGLContextClosing()
{
    openGlTexture.release();
    textureUniform.reset();
    shaderProgram.reset();
}

void VisualCanvas::renderOpenGL()
{
    juce::OpenGLHelpers::clear(juce::Colour(0xff0a0c10));

    // Upload new texture if one arrived from background loader
    if (newTexturePending.exchange(false))
    {
        juce::Image imageToUpload;
        {
            std::lock_guard<std::mutex> lock(imageMutex);
            imageToUpload = pendingImage;
        }

        if (imageToUpload.isValid())
        {
            openGlTexture.loadImage(imageToUpload);
            hasImage = true;
        }
        else
        {
            openGlTexture.release();
            hasImage = false;
        }
    }

    if (!hasImage || openGlTexture.getTextureID() == 0 || shaderProgram == nullptr)
        return;

    const auto desktopScale = static_cast<float>(openGLContext.getRenderingScale());
    const int viewW = juce::roundToInt(static_cast<float>(getWidth()) * desktopScale);
    const int viewH = juce::roundToInt(static_cast<float>(getHeight()) * desktopScale);

    if (viewW <= 0 || viewH <= 0)
        return;

    glViewport(0, 0, viewW, viewH);

    // Calculate aspect ratios
    const float canvasAspect = static_cast<float>(viewW) / static_cast<float>(viewH);
    const float imgAspect    = static_cast<float>(imageWidth) / static_cast<float>(imageHeight);

    float sx = 1.0f;
    float sy = 1.0f;

    if (currentScaleMode == ScaleMode::Fit)
    {
        // Fit mode: keep aspect ratio, entire image visible within canvas
        if (imgAspect >= canvasAspect)
        {
            sx = 1.0f;
            sy = canvasAspect / imgAspect;
        }
        else
        {
            sx = imgAspect / canvasAspect;
            sy = 1.0f;
        }
    }
    else // Cover mode: keep aspect ratio, fill canvas completely, crop overflow
    {
        if (imgAspect >= canvasAspect)
        {
            sx = imgAspect / canvasAspect;
            sy = 1.0f;
        }
        else
        {
            sx = 1.0f;
            sy = canvasAspect / imgAspect;
        }
    }

    // Audio-reactive test value: dynamic scale pulsation driven by bass band & react parameter
    const auto audioSnap = analysis.getSnapshot();
    const float bassEnergy = audioSnap.bands.bass * audioSnap.react;
    const float audioPulse = 1.0f + 0.08f * bassEnergy;
    sx *= audioPulse;
    sy *= audioPulse;

    // Quad vertices in Normalized Device Coordinates (NDC: -1 to +1)
    // Texture coordinates: top is v=1.0, bottom is v=0.0 in JUCE OpenGLTexture
    struct Vertex
    {
        float x, y;
        float u, v;
    };

    const Vertex vertices[4] = {
        { -sx, -sy, 0.0f, 0.0f }, // Bottom-Left
        {  sx, -sy, 1.0f, 0.0f }, // Bottom-Right
        { -sx,  sy, 0.0f, 1.0f }, // Top-Left
        {  sx,  sy, 1.0f, 1.0f }  // Top-Right
    };

    shaderProgram->use();

    glActiveTexture(GL_TEXTURE0);
    openGlTexture.bind();

    if (textureUniform != nullptr)
        textureUniform->set(0);

    const GLint posAttrib = openGLContext.extensions.glGetAttribLocation(shaderProgram->getProgramID(), "aPosition");
    const GLint texAttrib = openGLContext.extensions.glGetAttribLocation(shaderProgram->getProgramID(), "aTexCoord");

    if (posAttrib >= 0)
    {
        openGLContext.extensions.glEnableVertexAttribArray(static_cast<GLuint>(posAttrib));
        openGLContext.extensions.glVertexAttribPointer(static_cast<GLuint>(posAttrib), 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), &vertices[0].x);
    }

    if (texAttrib >= 0)
    {
        openGLContext.extensions.glEnableVertexAttribArray(static_cast<GLuint>(texAttrib));
        openGLContext.extensions.glVertexAttribPointer(static_cast<GLuint>(texAttrib), 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), &vertices[0].u);
    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if (posAttrib >= 0)
        openGLContext.extensions.glDisableVertexAttribArray(static_cast<GLuint>(posAttrib));

    if (texAttrib >= 0)
        openGLContext.extensions.glDisableVertexAttribArray(static_cast<GLuint>(texAttrib));

    openGlTexture.unbind();
}

// ==============================================================================
// 2D Component Paint (UI Overlays & Status Bar)
// ==============================================================================
void VisualCanvas::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds();
    const int w = bounds.getWidth();
    constexpr int statusBarHeight = 28;

    // Viewport area above the bottom status bar
    const auto viewportArea = bounds.withTrimmedBottom(statusBarHeight);

    // 1. Placeholder state if no image loaded
    if (!hasImage)
    {
        g.setColour(juce::Colour(0xff0a0c10));
        g.fillRect(viewportArea);

        // Subtle background grid
        g.setColour(juce::Colour(0x0affffff));
        constexpr int gridSize = 40;
        for (int x = 0; x < w; x += gridSize)
            g.drawVerticalLine(x, 0.0f, static_cast<float>(viewportArea.getHeight()));
        for (int y = 0; y < viewportArea.getHeight(); y += gridSize)
            g.drawHorizontalLine(y, 0.0f, static_cast<float>(w));

        // Center drop-zone box
        const int boxW = juce::jmin(460, w - 40);
        const int boxH = juce::jmin(230, viewportArea.getHeight() - 40);
        const juce::Rectangle<int> dropBox((w - boxW) / 2, (viewportArea.getHeight() - boxH) / 2, boxW, boxH);

        // Dark box surface
        g.setColour(juce::Colour(0xff12151e));
        g.fillRoundedRectangle(dropBox.toFloat(), 8.0f);

        // Dashed border (highlighted cyan if dragging file over)
        g.setColour(isDragOver ? juce::Colour(0xff00b4d8) : juce::Colour(0xff272e3d));
        const float dashLengths[2] = { 6.0f, 4.0f };
        g.drawDashedLine(juce::Line<float>(static_cast<float>(dropBox.getX() + 8), static_cast<float>(dropBox.getY()), static_cast<float>(dropBox.getRight() - 8), static_cast<float>(dropBox.getY())), dashLengths, 2, isDragOver ? 2.0f : 1.5f);
        g.drawDashedLine(juce::Line<float>(static_cast<float>(dropBox.getX() + 8), static_cast<float>(dropBox.getBottom()), static_cast<float>(dropBox.getRight() - 8), static_cast<float>(dropBox.getBottom())), dashLengths, 2, isDragOver ? 2.0f : 1.5f);
        g.drawDashedLine(juce::Line<float>(static_cast<float>(dropBox.getX()), static_cast<float>(dropBox.getY() + 8), static_cast<float>(dropBox.getX()), static_cast<float>(dropBox.getBottom() - 8)), dashLengths, 2, isDragOver ? 2.0f : 1.5f);
        g.drawDashedLine(juce::Line<float>(static_cast<float>(dropBox.getRight()), static_cast<float>(dropBox.getY() + 8), static_cast<float>(dropBox.getRight()), static_cast<float>(dropBox.getBottom() - 8)), dashLengths, 2, isDragOver ? 2.0f : 1.5f);

        // Center drop icon / plus badge
        const int iconY = dropBox.getY() + 24;
        g.setColour(isDragOver ? juce::Colour(0xff00b4d8) : juce::Colour(0xff3a4354));
        g.drawRoundedRectangle(static_cast<float>(dropBox.getCentreX() - 18), static_cast<float>(iconY), 36.0f, 36.0f, 6.0f, 1.5f);
        g.drawLine(static_cast<float>(dropBox.getCentreX() - 8), static_cast<float>(iconY + 18), static_cast<float>(dropBox.getCentreX() + 8), static_cast<float>(iconY + 18), 2.0f);
        g.drawLine(static_cast<float>(dropBox.getCentreX()), static_cast<float>(iconY + 10), static_cast<float>(dropBox.getCentreX()), static_cast<float>(iconY + 26), 2.0f);

        // Primary placeholder text
        g.setColour(isDragOver ? juce::Colour(0xff00b4d8) : juce::Colour(0xffe1e6ef));
        g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
        g.drawText(isDragOver ? "RELEASE TO IMPORT IMAGE" : "DRAG & DROP IMAGE HERE",
                   dropBox.withTrimmedTop(68).withHeight(28),
                   juce::Justification::centred,
                   false);

        // Secondary hint
        g.setColour(juce::Colour(0xff7d8799));
        g.setFont(juce::FontOptions(12.0f, juce::Font::plain));
        g.drawText(isLoading ? "Loading and processing image..." : "or click here to browse / press Ctrl+V to paste screenshot",
                   dropBox.withTrimmedTop(98).withHeight(22),
                   juce::Justification::centred,
                   false);

        // Supported format pills
        g.setColour(juce::Colour(0xff4a5364));
        g.setFont(juce::FontOptions(11.0f, juce::Font::plain));
        g.drawText("Supported formats: JPG  \u2022  PNG  \u2022  WebP  \u2022  BMP  (up to 4096x4096)",
                   dropBox.withTrimmedTop(140).withHeight(20),
                   juce::Justification::centred,
                   false);
    }
    else if (isDragOver)
    {
        // Drag over active image
        g.setColour(juce::Colour(0x5000b4d8));
        g.fillRect(viewportArea);
        g.setColour(juce::Colour(0xff00b4d8));
        g.drawRect(viewportArea, 3);

        g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
        g.drawText("RELEASE TO REPLACE IMAGE", viewportArea, juce::Justification::centred, false);
    }

    // 2. Bottom Status Line (always present)
    const auto statusArea = juce::Rectangle<int>(0, bounds.getBottom() - statusBarHeight, w, statusBarHeight);

    // Dark sleek background
    g.setColour(juce::Colour(0xff10131a));
    g.fillRect(statusArea);

    // Top hairline divider
    g.setColour(juce::Colour(0xff232732));
    g.drawHorizontalLine(statusArea.getY(), 0.0f, static_cast<float>(w));

    // Status text (color-coded for errors/warnings vs normal)
    if (isStatusError)
        g.setColour(juce::Colour(0xffff9966)); // Warning amber
    else if (hasImage)
        g.setColour(juce::Colour(0xffd0d7e5)); // Clean white-slate
    else
        g.setColour(juce::Colour(0xff7d8799)); // Muted slate

    g.setFont(juce::FontOptions(11.5f, juce::Font::plain));
    g.drawText(currentStatusText,
               statusArea.reduced(12, 0).withTrimmedRight(140),
               juce::Justification::centredLeft,
               true);

    // Right-hand Scale Mode Indicator Pill
    const auto modePillRect = statusArea.reduced(10, 4).removeFromRight(120);
    g.setColour(juce::Colour(0xff181c25));
    g.fillRoundedRectangle(modePillRect.toFloat(), 4.0f);

    g.setColour(juce::Colour(0xff2c3444));
    g.drawRoundedRectangle(modePillRect.toFloat(), 4.0f, 1.0f);

    g.setColour(juce::Colour(0xff00b4d8));
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    const juce::String modeLabel = (currentScaleMode == ScaleMode::Fit ? "MODE: FIT" : "MODE: COVER");
    g.drawText(modeLabel, modePillRect, juce::Justification::centred, false);
}

void VisualCanvas::resized()
{
    openGLContext.triggerRepaint();
}

void VisualCanvas::mouseDown(const juce::MouseEvent& e)
{
    // Clicking on the placeholder opens the file dialog
    if (!hasImage && e.y < getHeight() - 28)
    {
        openFileDialog();
    }
}

// ==============================================================================
// Drag & Drop Implementation
// ==============================================================================
bool VisualCanvas::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& file : files)
    {
        if (ImageLoader::isSupportedExtension(juce::File(file).getFileExtension()))
            return true;
    }
    return false;
}

void VisualCanvas::fileDragEnter(const juce::StringArray&, int, int)
{
    isDragOver = true;
    repaint();
}

void VisualCanvas::fileDragMove(const juce::StringArray&, int, int)
{
}

void VisualCanvas::fileDragExit(const juce::StringArray&)
{
    isDragOver = false;
    repaint();
}

void VisualCanvas::filesDropped(const juce::StringArray& files, int, int)
{
    isDragOver = false;
    repaint();

    for (const auto& filePath : files)
    {
        const juce::File file(filePath);
        if (ImageLoader::isSupportedExtension(file.getFileExtension()))
        {
            isLoading = true;
            isStatusError = false;
            currentStatusText = "Loading " + file.getFileName() + "...";
            repaint();

            ImageLoader::loadFileAsync(file, [this](const ImageImportResult& result)
            {
                onImageLoaded(result);
            });
            break;
        }
    }
}

// ==============================================================================
// Actions: Open File & Clipboard
// ==============================================================================
void VisualCanvas::openFileDialog()
{
    activeFileChooser = std::make_unique<juce::FileChooser>(
        "Select Image to Import (JPG, PNG, WebP, BMP)",
        juce::File::getSpecialLocation(juce::File::userPicturesDirectory),
        "*.jpg;*.jpeg;*.png;*.webp;*.bmp"
    );

    constexpr int chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    activeFileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
    {
        const auto chosenFile = fc.getResult();
        if (chosenFile.existsAsFile())
        {
            isLoading = true;
            isStatusError = false;
            currentStatusText = "Loading " + chosenFile.getFileName() + "...";
            repaint();

            ImageLoader::loadFileAsync(chosenFile, [this](const ImageImportResult& result)
            {
                onImageLoaded(result);
            });
        }
        activeFileChooser.reset();
    });
}

void VisualCanvas::pasteFromClipboard()
{
    isLoading = true;
    isStatusError = false;
    currentStatusText = "Reading clipboard...";
    repaint();

    ImageLoader::loadClipboardAsync([this](const ImageImportResult& result)
    {
        onImageLoaded(result);
    });
}

void VisualCanvas::onImageLoaded(const ImageImportResult& result)
{
    isLoading = false;

    if (!result.success)
    {
        isStatusError = true;
        currentStatusText = result.errorMessage;
        repaint();
        return;
    }

    isStatusError = false;
    imageWidth = result.finalWidth;
    imageHeight = result.finalHeight;
    currentSourceName = result.sourceName;
    currentStatusText = "Loaded: " + result.statusMessage;

    {
        std::lock_guard<std::mutex> lock(imageMutex);
        pendingImage = result.image;
    }

    newTexturePending = true;
    openGLContext.triggerRepaint();
    repaint();
}

void VisualCanvas::setScaleMode(ScaleMode mode)
{
    if (currentScaleMode != mode)
    {
        currentScaleMode = mode;
        openGLContext.triggerRepaint();
        repaint();
    }
}

void VisualCanvas::toggleScaleMode()
{
    setScaleMode(currentScaleMode == ScaleMode::Fit ? ScaleMode::Cover : ScaleMode::Fit);
}

} // namespace drinkfloyd
