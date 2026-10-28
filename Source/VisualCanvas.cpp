#include "VisualCanvas.h"
#include "HeaderBar.h"
#include "AudioAnalysisEngine.h"
#include <cmath>

// Bring JUCE's OpenGL function pointers and constants into scope
using namespace juce::gl;

namespace drinkfloyd
{

// ==============================================================================
// Vertex Shader (Common for Pass 1 & Pass 2)
// ==============================================================================
static const char* commonVertexShader = R"(
    attribute vec2 aPosition;
    attribute vec2 aTexCoord;
    varying vec2 vTexCoord;

    void main()
    {
        gl_Position = vec4(aPosition, 0.0, 1.0);
        vTexCoord = aTexCoord;
    }
)";

// ==============================================================================
// Pass 1: Audio-Reactive Warp & Persistence Fragment Shader
// ==============================================================================
static const char* warpFragmentShader = R"(
    varying vec2 vTexCoord;

    uniform sampler2D u_imageTexture;
    uniform sampler2D u_prevTexture;

    // Audio band uniforms
    uniform float u_bass;
    uniform float u_lowMid;
    uniform float u_mid;
    uniform float u_high;
    uniform float u_beat;

    // Visual Parameters
    uniform int   u_scene;      // 0=Smear, 1=Ripple, 2=Flow, 3=Tiles
    uniform float u_react;
    uniform float u_chaos;
    uniform float u_trails;
    uniform float u_heat;
    uniform float u_decay;
    uniform float u_symmetry;
    uniform float u_tiles;
    uniform float u_twist;
    uniform float u_slice;
    uniform float u_lens;
    uniform int   u_lensStyle;  // 0=Plain, 1=Ribs, 2=Rings

    uniform float u_time;
    uniform vec2  u_resolution;
    uniform vec2  u_aspectScale; // For Fit vs Cover mapping: (sx, sy)

    vec2 rotateVec(vec2 v, float angle)
    {
        float c = cos(angle);
        float s = sin(angle);
        return vec2(v.x * c - v.y * s, v.x * s + v.y * c);
    }

    void main()
    {
        vec2 uv = vTexCoord;
        vec2 p = uv - vec2(0.5);

        // 1. Lens Distortion (Presets: Plain, Ribs, Rings)
        float r = length(p);
        if (u_lens > 0.001)
        {
            if (u_lensStyle == 0) // Plain: spherical convex / barrel
            {
                float dist = r * (1.0 + u_lens * (r * r * 2.8 - 0.2));
                if (r > 0.0001) p = (p / r) * dist;
            }
            else if (u_lensStyle == 1) // Ribs: cylindrical fresnel fluting
            {
                p.x += sin(p.y * 32.0 + u_time * 2.5) * 0.035 * u_lens * (1.0 + u_mid * u_react * 2.5);
                float dist = r * (1.0 + u_lens * r * 0.85);
                if (r > 0.0001) p = (p / r) * dist;
            }
            else // Rings: concentric circular fresnel rings
            {
                float ring = sin(r * 38.0 - u_time * 4.5) * 0.032 * u_lens * (1.0 + u_bass * u_react * 2.5);
                float dist = r + ring;
                if (r > 0.0001) p = (p / r) * dist;
            }
            r = length(p);
        }

        // 2. Symmetry (Kaleidoscopic / Rotational folding)
        if (u_symmetry > 1.05)
        {
            float numSegments = floor(u_symmetry);
            float angle = atan(p.y, p.x);
            float segAngle = 6.2831853 / numSegments;
            angle = mod(angle + segAngle * 0.5, segAngle) - segAngle * 0.5;
            angle = abs(angle);
            p = vec2(cos(angle), sin(angle)) * r;
        }

        // 3. Twist
        if (abs(u_twist) > 0.001)
        {
            float twistAngle = u_twist * (1.0 - clamp(r * 1.5, 0.0, 1.0)) * 3.14159 * (1.0 + u_lowMid * u_react * 2.2);
            p = rotateVec(p, twistAngle);
        }

        // 4. Tiles Grid
        if (u_tiles > 1.01)
        {
            p = fract(p * u_tiles + vec2(0.5)) - vec2(0.5);
        }

        // 5. Slice (Horizontal raster strip shifts)
        if (u_slice > 0.001)
        {
            float strip = floor((p.y + 0.5) * 16.0);
            float shift = sin(strip * 4.13 + u_time * 3.2) * u_slice * 0.15 * (1.0 + u_mid * u_react * 3.0);
            p.x += shift;
        }

        // 6. Scene-Specific Per-Pixel Audio Reactive Warps
        if (u_scene == 0) // SMEAR: Directional gravitational pixel melting & chromatic sliding
        {
            float meltNoise = sin(p.x * 24.0 + u_time * 2.5) * u_chaos * 0.05;
            float meltSpeed = (0.03 + u_bass * u_react * 0.16 + u_beat * u_react * 0.08) * (1.0 + meltNoise);
            p.y -= meltSpeed;
            p.x += sin(p.y * 14.0 + u_time * 1.2) * (0.012 + u_lowMid * u_react * 0.05);
        }
        else if (u_scene == 1) // RIPPLE: Shockwave liquid ripples propagating outwards with beat
        {
            float rippleFreq = 26.0 + u_mid * u_react * 16.0;
            float rippleAmp  = (0.035 + u_bass * u_react * 0.15 + u_beat * u_react * 0.12) * (1.0 + u_chaos * 0.85);
            float wave = sin(r * rippleFreq - u_time * 7.5 - u_beat * 5.0) * rippleAmp;
            vec2 dir = (r > 0.0001) ? normalize(p) : vec2(0.0, 1.0);
            p += dir * wave;
        }
        else if (u_scene == 2) // FLOW: Liquid advection / curl noise fluid swirl
        {
            float t = u_time * 1.6;
            float vx = sin(p.y * 7.5 + t + u_bass * u_react * 4.5) + cos(p.x * 5.0 - t * 0.8) * u_chaos;
            float vy = cos(p.x * 7.5 - t + u_lowMid * u_react * 4.5) + sin(p.y * 5.0 + t * 0.8) * u_chaos;
            p += vec2(vx, vy) * (0.035 + u_react * 0.09);
        }
        else if (u_scene == 3) // TILES: Dynamic mosaic permutation vibrating to audio bands
        {
            float gridSub = floor(1.0 + u_bass * u_react * 2.0);
            vec2 q = fract(p * gridSub + vec2(0.5)) - vec2(0.5);
            float rot = (u_mid * u_react * 1.6 + u_beat * 0.9) * sign(sin(floor((p.x + 0.5) * 4.0) + floor((p.y + 0.5) * 4.0)));
            p = mix(p, rotateVec(q, rot), 0.70);
        }

        // Map back to UV coordinates [0, 1]
        vec2 warpUv = p + vec2(0.5);

        // Aspect Ratio Scale mapping (Fit vs Cover)
        vec2 srcUv = (warpUv - vec2(0.5)) / u_aspectScale + vec2(0.5);

        // Sample current source image
        vec4 currentCol;
        if (srcUv.x < 0.0 || srcUv.x > 1.0 || srcUv.y < 0.0 || srcUv.y > 1.0)
        {
            currentCol = vec4(0.04, 0.05, 0.07, 1.0);
        }
        else
        {
            currentCol = texture2D(u_imageTexture, srcUv);
        }

        // Chromatic dispersion along audio transients
        float dispersion = (u_bass * 0.016 + u_high * 0.014) * u_react;
        if (dispersion > 0.001)
        {
            vec2 uvR = (warpUv - vec2(0.5) + vec2(dispersion, 0.0)) / u_aspectScale + vec2(0.5);
            vec2 uvB = (warpUv - vec2(0.5) - vec2(dispersion, 0.0)) / u_aspectScale + vec2(0.5);
            if (uvR.x >= 0.0 && uvR.x <= 1.0 && uvR.y >= 0.0 && uvR.y <= 1.0)
                currentCol.r = texture2D(u_imageTexture, uvR).r;
            if (uvB.x >= 0.0 && uvB.x <= 1.0 && uvB.y >= 0.0 && uvB.y <= 1.0)
                currentCol.b = texture2D(u_imageTexture, uvB).b;
        }

        // Read previous frame from feedback texture
        vec4 prevCol = texture2D(u_prevTexture, uv);

        // Persistence & Trails blend
        float trailWeight = clamp(u_trails * (1.0 - u_decay * 0.45), 0.0, 0.96);
        vec4 result = mix(currentCol, prevCol, trailWeight);

        // Heat: Thermal glow injected into feedback on bass & beat peaks
        float heatEnergy = (u_bass * 0.75 + u_beat * 0.65) * u_react * u_heat;
        if (heatEnergy > 0.004)
        {
            vec3 heatColor = vec3(1.0, 0.42, 0.08) * heatEnergy * (1.0 - clamp(r * 1.3, 0.0, 1.0));
            result.rgb += heatColor;
        }

        gl_FragColor = result;
    }
)";

// ==============================================================================
// Pass 2: Canvas Presentation, Palette & Vignette Fragment Shader
// ==============================================================================
static const char* presentFragmentShader = R"(
    varying vec2 vTexCoord;

    uniform sampler2D u_frameTexture; // From Pass 1 FBO

    uniform float u_brightness;
    uniform float u_hueDrift;
    uniform int   u_palette;      // 0=PrismCore, 1=NeonUltraviolet, 2=SolarFlare, 3=MonochromeIce, 4=DarkFloyd, 5=Custom
    uniform vec3  u_customColour; // User chosen custom RGB
    uniform float u_vignette;
    uniform float u_time;

    vec3 rotateHue(vec3 col, float angle)
    {
        const vec3 k = vec3(0.57735, 0.57735, 0.57735);
        float cosAngle = cos(angle);
        return col * cosAngle + cross(k, col) * sin(angle) + k * dot(k, col) * (1.0 - cosAngle);
    }

    void main()
    {
        vec2 uv = vTexCoord;
        vec4 col = texture2D(u_frameTexture, uv);

        // 1. Hue Drift
        if (abs(u_hueDrift) > 0.001)
        {
            col.rgb = rotateHue(col.rgb, u_hueDrift * 6.2831853);
        }

        // 2. Palette Presets (Maps luminance across distinct psychedelic gradients)
        float lum = clamp(dot(col.rgb, vec3(0.299, 0.587, 0.114)), 0.0, 1.0);

        if (u_palette == 1) // Neon Ultraviolet: deep violet -> hot magenta -> neon cyan
        {
            vec3 c0 = vec3(0.08, 0.02, 0.22);
            vec3 c1 = vec3(0.92, 0.05, 0.55);
            vec3 c2 = vec3(0.0, 0.95, 0.95);
            vec3 pal = (lum < 0.5) ? mix(c0, c1, lum * 2.0) : mix(c1, c2, (lum - 0.5) * 2.0);
            col.rgb = mix(col.rgb, pal, 0.75);
        }
        else if (u_palette == 2) // Solar Flare: obsidian black -> deep ruby -> burning amber -> white hot
        {
            vec3 c0 = vec3(0.05, 0.01, 0.01);
            vec3 c1 = vec3(0.85, 0.12, 0.05);
            vec3 c2 = vec3(1.0, 0.85, 0.2);
            vec3 pal = (lum < 0.5) ? mix(c0, c1, lum * 2.0) : mix(c1, c2, (lum - 0.5) * 2.0);
            col.rgb = mix(col.rgb, pal, 0.80);
        }
        else if (u_palette == 3) // Monochrome Ice: dark navy -> frosted cyan -> silver white
        {
            vec3 c0 = vec3(0.04, 0.08, 0.15);
            vec3 c1 = vec3(0.25, 0.65, 0.85);
            vec3 c2 = vec3(0.92, 0.96, 1.0);
            vec3 pal = (lum < 0.5) ? mix(c0, c1, lum * 2.0) : mix(c1, c2, (lum - 0.5) * 2.0);
            col.rgb = mix(col.rgb, pal, 0.85);
        }
        else if (u_palette == 4) // Dark Floyd: pitch black -> emerald forest -> electric spectrum
        {
            vec3 c0 = vec3(0.02, 0.04, 0.03);
            vec3 c1 = vec3(0.0, 0.85, 0.45);
            vec3 c2 = vec3(0.95, 0.85, 0.15);
            vec3 pal = (lum < 0.5) ? mix(c0, c1, lum * 2.0) : mix(c1, c2, (lum - 0.5) * 2.0);
            col.rgb = mix(col.rgb, pal, 0.75);
        }
        else if (u_palette == 5) // Custom Palette: gradients through user selected customColour
        {
            vec3 c0 = vec3(0.02, 0.02, 0.03);
            vec3 c1 = u_customColour;
            vec3 c2 = vec3(1.0, 1.0, 1.0);
            vec3 pal = (lum < 0.5) ? mix(c0, c1, lum * 2.0) : mix(c1, c2, (lum - 0.5) * 2.0);
            col.rgb = mix(col.rgb, pal, 0.80);
        }

        // 3. Brightness
        col.rgb *= u_brightness;

        // 4. Vignette
        if (u_vignette > 0.001)
        {
            float d = length(uv - vec2(0.5)) * 1.4142;
            float vig = clamp(1.0 - d * d * u_vignette, 0.0, 1.0);
            col.rgb *= vig;
        }

        gl_FragColor = col;
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

    // Setup OpenGL context: continuous repainting for true 60 FPS GPU rendering
    openGLContext.setComponentPaintingEnabled(true);
    openGLContext.setContinuousRepainting(true);
    openGLContext.setRenderer(this);
    openGLContext.attachTo(*this);

    startTime = juce::Time::getMillisecondCounterHiRes();
}

VisualCanvas::~VisualCanvas()
{
    openGLContext.detach();
}

void VisualCanvas::setParameters(const VisualParameters& params)
{
    std::lock_guard<std::mutex> lock(paramMutex);
    parameters = params;
}

VisualParameters VisualCanvas::getParameters() const
{
    std::lock_guard<std::mutex> lock(paramMutex);
    return parameters;
}

void VisualCanvas::setScene(SceneType scene)
{
    std::lock_guard<std::mutex> lock(paramMutex);
    parameters.scene = scene;
}

void VisualCanvas::setPreviewAspect(PreviewAspect aspect)
{
    currentPreviewAspect = aspect;
    repaint();
}

// ==============================================================================
// OpenGL Context Management
// ==============================================================================
void VisualCanvas::newOpenGLContextCreated()
{
    initShaders();

    // Create default procedural Floyd test image pattern
    juce::Image defaultImg(juce::Image::ARGB, 512, 512, true);
    {
        juce::Graphics g(defaultImg);
        g.fillAll(juce::Colour(0xff0d1117));

        // Prism rainbow gradient circle
        juce::ColourGradient grad(juce::Colour(0xff00b4d8), 256.0f, 256.0f,
                                  juce::Colour(0xffff007f), 480.0f, 480.0f, true);
        grad.addColour(0.3, juce::Colour(0xff7928ca));
        grad.addColour(0.6, juce::Colour(0xffff9900));
        grad.addColour(0.9, juce::Colour(0xff3ddc84));
        g.setGradientFill(grad);
        g.fillEllipse(64.0f, 64.0f, 384.0f, 384.0f);

        g.setColour(juce::Colour(0xff0a0d14));
        g.fillEllipse(128.0f, 128.0f, 256.0f, 256.0f);

        // Core prism triangle
        juce::Path triangle;
        triangle.addTriangle(256.0f, 160.0f, 170.0f, 330.0f, 342.0f, 330.0f);
        g.setColour(juce::Colours::white);
        g.strokePath(triangle, juce::PathStrokeType(4.0f));
    }
    defaultPatternTexture.loadImage(defaultImg);
}

void VisualCanvas::openGLContextClosing()
{
    warpShader.reset();
    presentShader.reset();
    releaseFBOs();
    sourceTexture.release();
    defaultPatternTexture.release();
}

void VisualCanvas::releaseFBOs()
{
    if (fboPing != nullptr) { fboPing->release(); fboPing.reset(); }
    if (fboPong != nullptr) { fboPong->release(); fboPong.reset(); }
    fboWidth = 0;
    fboHeight = 0;
}

void VisualCanvas::initShaders()
{
    // Pass 1: Warp Shader
    warpShader = std::make_unique<juce::OpenGLShaderProgram>(openGLContext);
    if (!warpShader->addVertexShader(commonVertexShader) ||
        !warpShader->addFragmentShader(warpFragmentShader) ||
        !warpShader->link())
    {
        DBG("Warp shader error: " + warpShader->getLastError());
    }

    // Pass 2: Present Shader
    presentShader = std::make_unique<juce::OpenGLShaderProgram>(openGLContext);
    if (!presentShader->addVertexShader(commonVertexShader) ||
        !presentShader->addFragmentShader(presentFragmentShader) ||
        !presentShader->link())
    {
        DBG("Present shader error: " + presentShader->getLastError());
    }
}

// ==============================================================================
// OpenGL Render Pipeline (Pass 1 -> Ping-Pong FBO -> Pass 2 Presentation)
// ==============================================================================
void VisualCanvas::renderOpenGL()
{
    // Record true rendered frame for the real-time FPS counter
    header.recordFrame();

    const double nowMs = juce::Time::getMillisecondCounterHiRes();
    const float timeSec = static_cast<float>((nowMs - startTime) * 0.001);

    // 1. Upload new image texture if user imported one
    if (newTexturePending.load(std::memory_order_acquire))
    {
        juce::Image imgToLoad;
        {
            std::lock_guard<std::mutex> lock(imageMutex);
            imgToLoad = pendingImage;
            pendingImage = {};
            newTexturePending.store(false, std::memory_order_release);
        }

        if (imgToLoad.isValid())
        {
            sourceTexture.loadImage(imgToLoad);
            hasImage.store(true);
        }
    }

    const auto bounds = getLocalBounds();
    const int canvasW = bounds.getWidth();
    const int canvasH = bounds.getHeight();
    constexpr int statusBarHeight = 28;
    const int viewportH = std::max(1, canvasH - statusBarHeight);

    if (canvasW <= 0 || viewportH <= 0)
        return;

    // Determine preview aspect ratio & letterbox viewport
    float targetAspect = 1.0f; // 1080x1080 by default (1:1)
    if (currentPreviewAspect == PreviewAspect::Portrait1080x1920)
        targetAspect = 1080.0f / 1920.0f; // 9:16 (~0.5625)
    else if (currentPreviewAspect == PreviewAspect::Landscape1920x1080)
        targetAspect = 1920.0f / 1080.0f; // 16:9 (~1.7778)

    const float canvasAspect = static_cast<float>(canvasW) / static_cast<float>(viewportH);

    int letterboxW = canvasW;
    int letterboxH = viewportH;
    int letterboxX = 0;
    int letterboxY = statusBarHeight;

    if (canvasAspect > targetAspect)
    {
        letterboxW = static_cast<int>(viewportH * targetAspect);
        letterboxX = (canvasW - letterboxW) / 2;
    }
    else
    {
        letterboxH = static_cast<int>(canvasW / targetAspect);
        letterboxY = statusBarHeight + (viewportH - letterboxH) / 2;
    }

    // Ensure Ping-Pong FBOs match target preview resolution
    const int targetFboW = std::clamp(letterboxW, 256, 1920);
    const int targetFboH = std::clamp(letterboxH, 256, 1920);

    if (fboPing == nullptr || fboPong == nullptr || fboWidth != targetFboW || fboHeight != targetFboH)
    {
        fboPing = std::make_unique<juce::OpenGLFrameBuffer>();
        fboPong = std::make_unique<juce::OpenGLFrameBuffer>();
        fboPing->initialise(openGLContext, targetFboW, targetFboH);
        fboPong->initialise(openGLContext, targetFboW, targetFboH);
        fboPing->clear(juce::Colour(0xff0a0d14));
        fboPong->clear(juce::Colour(0xff0a0d14));
        fboWidth = targetFboW;
        fboHeight = targetFboH;
    }

    // Query active visual parameters and audio snapshot
    VisualParameters params;
    {
        std::lock_guard<std::mutex> lock(paramMutex);
        params = parameters;
    }
    const auto audioSnap = analysis.getSnapshot();

    // Source image aspect ratio scaling (Fit vs Cover)
    const float imgW = static_cast<float>(hasImage.load() ? imageWidth : 512);
    const float imgH = static_cast<float>(hasImage.load() ? imageHeight : 512);
    const float imgAspect = imgW / imgH;

    float sx = 1.0f;
    float sy = 1.0f;
    if (currentScaleMode == ScaleMode::Fit)
    {
        if (imgAspect >= targetAspect)
            sy = targetAspect / imgAspect;
        else
            sx = imgAspect / targetAspect;
    }
    else // Cover mode
    {
        if (imgAspect >= targetAspect)
            sx = imgAspect / targetAspect;
        else
            sy = targetAspect / imgAspect;
    }

    struct QuadVertex
    {
        float x, y, u, v;
    };

    const QuadVertex fboQuad[4] = {
        { -1.0f, -1.0f, 0.0f, 0.0f },
        {  1.0f, -1.0f, 1.0f, 0.0f },
        { -1.0f,  1.0f, 0.0f, 1.0f },
        {  1.0f,  1.0f, 1.0f, 1.0f }
    };

    // ==========================================================================
    // PASS 1: Render Reactive Warp & Persistence into fboPing
    // ==========================================================================
    if (warpShader != nullptr && fboPing != nullptr && fboPong != nullptr)
    {
        fboPing->makeCurrentRenderingTarget();
        glViewport(0, 0, fboWidth, fboHeight);

        warpShader->use();

        // Texture Unit 0: Source Image / Procedural Pattern
        glActiveTexture(GL_TEXTURE0);
        if (hasImage.load())
            sourceTexture.bind();
        else
            defaultPatternTexture.bind();
        warpShader->setUniform("u_imageTexture", 0);

        // Texture Unit 1: Feedback from previous frame (fboPong)
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, fboPong->getTextureID());
        warpShader->setUniform("u_prevTexture", 1);

        // Audio Uniforms
        warpShader->setUniform("u_bass",   audioSnap.bands.bass);
        warpShader->setUniform("u_lowMid", audioSnap.bands.lowMid);
        warpShader->setUniform("u_mid",    audioSnap.bands.mid);
        warpShader->setUniform("u_high",   audioSnap.bands.high);
        warpShader->setUniform("u_beat",   audioSnap.bands.beat);

        // Parameters
        warpShader->setUniform("u_scene",     static_cast<int>(params.scene));
        warpShader->setUniform("u_react",     params.react);
        warpShader->setUniform("u_chaos",     params.chaos);
        warpShader->setUniform("u_trails",    params.trails);
        warpShader->setUniform("u_heat",      params.heat);
        warpShader->setUniform("u_decay",     params.decay);
        warpShader->setUniform("u_symmetry",  params.symmetry);
        warpShader->setUniform("u_tiles",     params.tiles);
        warpShader->setUniform("u_twist",     params.twist);
        warpShader->setUniform("u_slice",     params.slice);
        warpShader->setUniform("u_lens",      params.lens);
        warpShader->setUniform("u_lensStyle", static_cast<int>(params.lensStyle));

        warpShader->setUniform("u_time",        timeSec);
        warpShader->setUniform("u_resolution",  static_cast<float>(fboWidth), static_cast<float>(fboHeight));
        warpShader->setUniform("u_aspectScale", sx, sy);

        // Draw quad into FBO
        const GLint posA = openGLContext.extensions.glGetAttribLocation(warpShader->getProgramID(), "aPosition");
        const GLint texA = openGLContext.extensions.glGetAttribLocation(warpShader->getProgramID(), "aTexCoord");

        if (posA >= 0)
        {
            openGLContext.extensions.glEnableVertexAttribArray(static_cast<GLuint>(posA));
            openGLContext.extensions.glVertexAttribPointer(static_cast<GLuint>(posA), 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), &fboQuad[0].x);
        }
        if (texA >= 0)
        {
            openGLContext.extensions.glEnableVertexAttribArray(static_cast<GLuint>(texA));
            openGLContext.extensions.glVertexAttribPointer(static_cast<GLuint>(texA), 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), &fboQuad[0].u);
        }

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        if (posA >= 0) openGLContext.extensions.glDisableVertexAttribArray(static_cast<GLuint>(posA));
        if (texA >= 0) openGLContext.extensions.glDisableVertexAttribArray(static_cast<GLuint>(texA));

        fboPing->releaseAsRenderingTarget();
    }

    // ==========================================================================
    // PASS 2: Canvas Presentation, Palette & Vignette to Viewport
    // ==========================================================================
    if (presentShader != nullptr && fboPing != nullptr)
    {
        // Bind default screen framebuffer and viewport
        glViewport(0, 0, canvasW, canvasH);

        // Clear entire canvas background to sleek dark tone
        glClearColor(0.05f, 0.06f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Set letterbox viewport
        glViewport(letterboxX, letterboxY, letterboxW, letterboxH);

        presentShader->use();

        // Texture Unit 0: Result from Pass 1
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fboPing->getTextureID());
        presentShader->setUniform("u_frameTexture", 0);

        // Parameters
        presentShader->setUniform("u_brightness", params.brightness);
        presentShader->setUniform("u_hueDrift",   params.hueDrift);
        presentShader->setUniform("u_palette",    static_cast<int>(params.palette));

        const float cr = params.customColour.getFloatRed();
        const float cg = params.customColour.getFloatGreen();
        const float cb = params.customColour.getFloatBlue();
        presentShader->setUniform("u_customColour", cr, cg, cb);

        presentShader->setUniform("u_vignette", params.vignette);
        presentShader->setUniform("u_time",     timeSec);

        const GLint posA = openGLContext.extensions.glGetAttribLocation(presentShader->getProgramID(), "aPosition");
        const GLint texA = openGLContext.extensions.glGetAttribLocation(presentShader->getProgramID(), "aTexCoord");

        if (posA >= 0)
        {
            openGLContext.extensions.glEnableVertexAttribArray(static_cast<GLuint>(posA));
            openGLContext.extensions.glVertexAttribPointer(static_cast<GLuint>(posA), 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), &fboQuad[0].x);
        }
        if (texA >= 0)
        {
            openGLContext.extensions.glEnableVertexAttribArray(static_cast<GLuint>(texA));
            openGLContext.extensions.glVertexAttribPointer(static_cast<GLuint>(texA), 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), &fboQuad[0].u);
        }

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        if (posA >= 0) openGLContext.extensions.glDisableVertexAttribArray(static_cast<GLuint>(posA));
        if (texA >= 0) openGLContext.extensions.glDisableVertexAttribArray(static_cast<GLuint>(texA));
    }

    // Swap Ping-Pong Framebuffers for the next frame
    std::swap(fboPing, fboPong);
}

// ==============================================================================
// 2D Component Paint (UI Overlays & Bottom Status Bar)
// ==============================================================================
void VisualCanvas::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds();
    const int w = bounds.getWidth();
    constexpr int statusBarHeight = 28;

    // Viewport area above the bottom status bar
    const auto viewportArea = bounds.withTrimmedBottom(statusBarHeight);

    // Placeholder hint if no custom image loaded
    if (!hasImage.load())
    {
        // Drag over active image
        if (isDragOver)
        {
            g.setColour(juce::Colour(0x5000b4d8));
            g.fillRect(viewportArea);
            g.setColour(juce::Colour(0xff00b4d8));
            g.drawRect(viewportArea, 3);
            g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
            g.drawText("RELEASE TO IMPORT IMAGE", viewportArea, juce::Justification::centred, false);
        }
    }
    else if (isDragOver)
    {
        g.setColour(juce::Colour(0x5000b4d8));
        g.fillRect(viewportArea);
        g.setColour(juce::Colour(0xff00b4d8));
        g.drawRect(viewportArea, 3);
        g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
        g.drawText("RELEASE TO REPLACE IMAGE", viewportArea, juce::Justification::centred, false);
    }

    // Bottom Status Line (always present)
    const auto statusArea = juce::Rectangle<int>(0, bounds.getBottom() - statusBarHeight, w, statusBarHeight);

    // Dark sleek background
    g.setColour(juce::Colour(0xff10131a));
    g.fillRect(statusArea);

    // Top hairline divider
    g.setColour(juce::Colour(0xff232732));
    g.drawHorizontalLine(statusArea.getY(), 0.0f, static_cast<float>(w));

    // Status text
    if (isStatusError)
        g.setColour(juce::Colour(0xffff9966));
    else if (hasImage.load())
        g.setColour(juce::Colour(0xffd0d7e5));
    else
        g.setColour(juce::Colour(0xff7d8799));

    g.setFont(juce::FontOptions(11.0f, juce::Font::plain));
    g.drawText(currentStatusText,
               statusArea.reduced(12, 0).withTrimmedRight(260),
               juce::Justification::centredLeft,
               true);

    // Right-hand Scene & Mode Indicator Pill
    const auto modePillRect = statusArea.reduced(10, 4).removeFromRight(240);
    g.setColour(juce::Colour(0xff181c25));
    g.fillRoundedRectangle(modePillRect.toFloat(), 4.0f);

    g.setColour(juce::Colour(0xff2c3444));
    g.drawRoundedRectangle(modePillRect.toFloat(), 4.0f, 1.0f);

    g.setColour(juce::Colour(0xff00b4d8));
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    VisualParameters params = getParameters();
    juce::String pillText = "SCENE: " + getSceneName(params.scene).toUpperCase() + " | " +
                            (currentScaleMode == ScaleMode::Fit ? "FIT" : "COVER");
    g.drawText(pillText, modePillRect, juce::Justification::centred, false);
}

void VisualCanvas::resized()
{
}

void VisualCanvas::mouseDown(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    if (!hasImage.load())
    {
        openFileDialog();
    }
}

// ==============================================================================
// Image Import Callbacks
// ==============================================================================
void VisualCanvas::openFileDialog()
{
    const int chooserFlags = juce::FileBrowserComponent::openMode
                           | juce::FileBrowserComponent::canSelectFiles;

    activeFileChooser = std::make_unique<juce::FileChooser>(
        "Select an Image (JPG, PNG, WebP, BMP)",
        juce::File::getSpecialLocation(juce::File::userPicturesDirectory),
        "*.jpg;*.jpeg;*.png;*.webp;*.bmp"
    );

    activeFileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
    {
        const auto resultFile = fc.getResult();
        if (resultFile.existsAsFile())
        {
            isLoading = true;
            currentStatusText = "Loading " + resultFile.getFileName() + "...";
            repaint();

            ImageLoader::loadFileAsync(resultFile, [this](ImageImportResult res)
            {
                onImageLoaded(res);
            });
        }
    });
}

void VisualCanvas::pasteFromClipboard()
{
    isLoading = true;
    currentStatusText = "Reading clipboard...";
    repaint();

    ImageLoader::loadClipboardAsync([this](ImageImportResult res)
    {
        onImageLoaded(res);
    });
}

void VisualCanvas::setScaleMode(ScaleMode mode)
{
    currentScaleMode = mode;
    repaint();
}

void VisualCanvas::toggleScaleMode()
{
    setScaleMode(currentScaleMode == ScaleMode::Fit ? ScaleMode::Cover : ScaleMode::Fit);
}

void VisualCanvas::onImageLoaded(const ImageImportResult& result)
{
    isLoading = false;

    if (!result.success || !result.image.isValid())
    {
        isStatusError = true;
        currentStatusText = result.errorMessage.isNotEmpty() ? result.errorMessage : "Failed to load image.";
        repaint();
        return;
    }

    isStatusError = false;
    imageWidth = result.finalWidth;
    imageHeight = result.finalHeight;
    currentSourceName = result.sourceName;
    currentStatusText = result.statusMessage;

    // Handoff image to OpenGL thread
    {
        std::lock_guard<std::mutex> lock(imageMutex);
        pendingImage = result.image;
        newTexturePending.store(true, std::memory_order_release);
    }

    repaint();
}

// ==============================================================================
// Drag & Drop Implementation
// ==============================================================================
bool VisualCanvas::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& file : files)
    {
        const juce::String ext = file.fromLastOccurrenceOf(".", false, false);
        if (ImageLoader::isSupportedExtension(ext))
            return true;
    }
    return false;
}

void VisualCanvas::fileDragEnter(const juce::StringArray& /*files*/, int /*x*/, int /*y*/)
{
    isDragOver = true;
    repaint();
}

void VisualCanvas::fileDragMove(const juce::StringArray& /*files*/, int /*x*/, int /*y*/)
{
}

void VisualCanvas::fileDragExit(const juce::StringArray& /*files*/)
{
    isDragOver = false;
    repaint();
}

void VisualCanvas::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    isDragOver = false;

    for (const auto& fileStr : files)
    {
        const juce::File targetFile { fileStr };
        const juce::String ext = targetFile.getFileExtension().replace(".", "");

        if (ImageLoader::isSupportedExtension(ext) && targetFile.existsAsFile())
        {
            isLoading = true;
            currentStatusText = "Importing " + targetFile.getFileName() + "...";
            repaint();

            ImageLoader::loadFileAsync(targetFile, [this](ImageImportResult res)
            {
                onImageLoaded(res);
            });
            break;
        }
    }
}

} // namespace drinkfloyd
