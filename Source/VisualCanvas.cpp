#include "VisualCanvas.h"
#include "HeaderBar.h"
#include "AudioAnalysisEngine.h"
#include <cmath>

// Bring JUCE's OpenGL function pointers and constants into scope
using namespace juce::gl;

namespace drinkfloyd
{

// ==============================================================================
// Common Fullscreen Quad Vertex Shader
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
// Pixel Melt & Scatter Vertex Shader
// ==============================================================================
static const char* meltScatterVertexShader = R"(
    attribute vec2 a_blockCoord;
    attribute vec2 a_corner;
    attribute vec2 a_cornerUV;
    attribute vec2 a_centerUV;

    varying vec2 v_texCoord;

    uniform float u_bass;
    uniform float u_lowMid;
    uniform float u_mid;
    uniform float u_high;
    uniform float u_beat;

    uniform float u_react;
    uniform float u_chaos;
    uniform float u_melt;
    uniform float u_scatter;
    uniform float u_decay;
    uniform float u_time;
    uniform vec2  u_aspectScale;
    uniform float u_blockSize;

    // High performance deterministic hash
    vec2 hash2(vec2 p)
    {
        vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
        p3 += dot(p3, p3.yzx + 33.33);
        return fract((p3.xx + p3.yz) * p3.zy) * 2.0 - 1.0;
    }

    float hash1(vec2 p)
    {
        vec3 p3 = fract(vec3(p.xyx) * 0.1031);
        p3 += dot(p3, p3.yzx + 33.33);
        return fract((p3.x + p3.y) * p3.z);
    }

    void main()
    {
        // Base rest position in NDC [-1.0, 1.0] from center UV, scaled by image aspect
        vec2 restCenter = (a_centerUV - vec2(0.5)) * 2.0 * u_aspectScale;

        // Block-specific stable random seeds
        vec2 rndDir = hash2(a_blockCoord);
        float rndVal1 = hash1(a_blockCoord * 1.618);
        float rndVal2 = hash1(a_blockCoord * 3.1415 + vec2(7.1, 13.3));

        // Audio force: driven by bass, beat transients, and mid punch
        float audioForce = (u_bass * 1.4 + u_beat * 1.2 + u_lowMid * 0.4) * u_react;

        // Activation threshold: staggered breakaway so blocks melt dynamically
        float threshold = rndVal1 * 0.35;
        float active = max(0.0, audioForce - threshold) * (1.0 + u_chaos * 0.8);
        active = clamp(active, 0.0, 1.0);

        // 1. Melt Downward: gravity pulls blocks down with slight horizontal sway
        float meltDist = active * (0.10 + 0.35 * rndVal2) * u_melt;
        vec2 meltOffset = vec2((rndVal1 - 0.5) * 0.25 * u_chaos * active, -meltDist);

        // 2. Scatter Outward: explodes radially in random stable direction
        float scatterDist = active * (0.05 + 0.22 * rndVal1) * u_scatter * (0.35 + 0.65 * u_chaos);
        vec2 scatterOffset = rndDir * scatterDist;

        // 3. Combined displacement
        vec2 disp = meltOffset + scatterOffset;

        // 4. Cap displacement to maintain visual coherence (shapes/faces stay clustered)
        float maxDisp = 0.45 * (u_react * 0.70 + 0.30);
        if (length(disp) > maxDisp)
            disp = normalize(disp) * maxDisp;

        // 5. Reform: When audio drops (silence / decay), disp -> 0 smoothly
        if (active > 0.001)
        {
            disp *= clamp(1.0 - u_decay * 0.20, 0.05, 1.0);
        }
        else
        {
            disp = vec2(0.0);
        }

        vec2 blockCenter = restCenter + disp;

        // Block scale & tumbling rotation
        float blockScale = 1.0 + active * 0.15;
        vec2 cornerOffset = a_corner * u_blockSize * blockScale * u_aspectScale;

        // Stable rotation angle (proportional to displacement / active, stable direction)
        float blockRot = active * (rndVal1 - 0.5) * 2.5 * (1.0 + u_mid * 1.5);
        float cosR = cos(blockRot);
        float sinR = sin(blockRot);
        vec2 rotatedCorner = vec2(cornerOffset.x * cosR - cornerOffset.y * sinR,
                                  cornerOffset.x * sinR + cornerOffset.y * cosR);

        vec2 finalPos = blockCenter + rotatedCorner;
        gl_Position = vec4(finalPos, 0.0, 1.0);

        // Dynamic UV: At rest (active=0), 100% full-resolution sharp original image.
        // As active increases, transitions smoothly to block center sampling.
        v_texCoord = mix(a_cornerUV, a_centerUV, clamp(active * 2.0, 0.0, 1.0));
    }
)";

// ==============================================================================
// Pixel Melt & Scatter Fragment Shader (Samples Original Image Color)
// ==============================================================================
static const char* meltScatterFragmentShader = R"(
    varying vec2 v_texCoord;

    uniform sampler2D u_imageTexture;
    uniform vec2 u_aspectScale;

    void main()
    {
        if (v_texCoord.x < 0.0 || v_texCoord.x > 1.0 || v_texCoord.y < 0.0 || v_texCoord.y > 1.0)
        {
            discard;
        }

        // Sample each block's ORIGINAL image color
        gl_FragColor = texture2D(u_imageTexture, v_texCoord);
    }
)";

// ==============================================================================
// Trails Feedback Shader (Blends previous frame into FBO)
// ==============================================================================
static const char* trailsFeedbackFragmentShader = R"(
    varying vec2 vTexCoord;
    uniform sampler2D u_prevTexture;
    uniform float u_trails;
    uniform float u_decay;

    void main()
    {
        vec4 prev = texture2D(u_prevTexture, vTexCoord);
        // Melt streaks fade out gracefully governed by Trails and Decay
        float trailWeight = clamp(u_trails * (1.0 - u_decay * 0.50), 0.0, 0.92);
        gl_FragColor = prev * trailWeight;
    }
)";

// ==============================================================================
// Generic Warp Shader (Pass 1 for Smear, Ripple, Flow, Tiles)
// ==============================================================================
static const char* warpFragmentShader = R"(
    varying vec2 vTexCoord;

    uniform sampler2D u_imageTexture;
    uniform sampler2D u_prevTexture;

    uniform float u_bass;
    uniform float u_lowMid;
    uniform float u_mid;
    uniform float u_high;
    uniform float u_beat;

    uniform int   u_scene; // 1=Smear, 2=Ripple, 3=Flow, 4=Tiles
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
    uniform int   u_lensStyle;

    uniform float u_time;
    uniform vec2  u_resolution;
    uniform vec2  u_aspectScale;

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

        // Lens
        float r = length(p);
        if (u_lens > 0.001)
        {
            if (u_lensStyle == 0) // Plain
            {
                float dist = r * (1.0 + u_lens * (r * r * 2.8 - 0.2));
                if (r > 0.0001) p = (p / r) * dist;
            }
            else if (u_lensStyle == 1) // Ribs
            {
                p.x += sin(p.y * 32.0 + u_time * 2.5) * 0.035 * u_lens * (1.0 + u_mid * u_react * 2.5);
                float dist = r * (1.0 + u_lens * r * 0.85);
                if (r > 0.0001) p = (p / r) * dist;
            }
            else // Rings
            {
                float ring = sin(r * 38.0 - u_time * 4.5) * 0.032 * u_lens * (1.0 + u_bass * u_react * 2.5);
                float dist = r + ring;
                if (r > 0.0001) p = (p / r) * dist;
            }
            r = length(p);
        }

        // Symmetry
        if (u_symmetry > 1.05)
        {
            float numSegments = floor(u_symmetry);
            float angle = atan(p.y, p.x);
            float segAngle = 6.2831853 / numSegments;
            angle = mod(angle + segAngle * 0.5, segAngle) - segAngle * 0.5;
            angle = abs(angle);
            p = vec2(cos(angle), sin(angle)) * r;
        }

        // Twist
        if (abs(u_twist) > 0.001)
        {
            float twistAngle = u_twist * (1.0 - clamp(r * 1.5, 0.0, 1.0)) * 3.14159 * (1.0 + u_lowMid * u_react * 2.2);
            p = rotateVec(p, twistAngle);
        }

        // Tiles
        if (u_tiles > 1.01)
        {
            p = fract(p * u_tiles + vec2(0.5)) - vec2(0.5);
        }

        // Slice
        if (u_slice > 0.001)
        {
            float strip = floor((p.y + 0.5) * 16.0);
            float shift = sin(strip * 4.13 + u_time * 3.2) * u_slice * 0.15 * (1.0 + u_mid * u_react * 3.0);
            p.x += shift;
        }

        // Scene warps
        if (u_scene == 1) // SMEAR
        {
            float meltNoise = sin(p.x * 24.0 + u_time * 2.5) * u_chaos * 0.05;
            float meltSpeed = (0.03 + u_bass * u_react * 0.16 + u_beat * u_react * 0.08) * (1.0 + meltNoise);
            p.y -= meltSpeed;
            p.x += sin(p.y * 14.0 + u_time * 1.2) * (0.012 + u_lowMid * u_react * 0.05);
        }
        else if (u_scene == 2) // RIPPLE
        {
            float rippleFreq = 26.0 + u_mid * u_react * 16.0;
            float rippleAmp  = (0.035 + u_bass * u_react * 0.15 + u_beat * u_react * 0.12) * (1.0 + u_chaos * 0.85);
            float wave = sin(r * rippleFreq - u_time * 7.5 - u_beat * 5.0) * rippleAmp;
            vec2 dir = (r > 0.0001) ? normalize(p) : vec2(0.0, 1.0);
            p += dir * wave;
        }
        else if (u_scene == 3) // FLOW
        {
            float t = u_time * 1.6;
            float vx = sin(p.y * 7.5 + t + u_bass * u_react * 4.5) + cos(p.x * 5.0 - t * 0.8) * u_chaos;
            float vy = cos(p.x * 7.5 - t + u_lowMid * u_react * 4.5) + sin(p.y * 5.0 + t * 0.8) * u_chaos;
            p += vec2(vx, vy) * (0.035 + u_react * 0.09);
        }
        else if (u_scene == 4) // TILES
        {
            float gridSub = floor(1.0 + u_bass * u_react * 2.0);
            vec2 q = fract(p * gridSub + vec2(0.5)) - vec2(0.5);
            float rot = (u_mid * u_react * 1.6 + u_beat * 0.9) * sign(sin(floor((p.x + 0.5) * 4.0) + floor((p.y + 0.5) * 4.0)));
            p = mix(p, rotateVec(q, rot), 0.70);
        }

        vec2 warpUv = p + vec2(0.5);
        vec2 srcUv = (warpUv - vec2(0.5)) / u_aspectScale + vec2(0.5);

        vec4 currentCol;
        if (srcUv.x < 0.0 || srcUv.x > 1.0 || srcUv.y < 0.0 || srcUv.y > 1.0)
        {
            currentCol = vec4(0.04, 0.05, 0.07, 1.0);
        }
        else
        {
            currentCol = texture2D(u_imageTexture, srcUv);
        }

        vec4 prevCol = texture2D(u_prevTexture, uv);
        float trailWeight = clamp(u_trails * (1.0 - u_decay * 0.45), 0.0, 0.96);
        vec4 result = mix(currentCol, prevCol, trailWeight);

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

    uniform sampler2D u_frameTexture;

    uniform float u_brightness;
    uniform float u_hueDrift;
    uniform int   u_palette;
    uniform vec3  u_customColour;
    uniform float u_vignette;
    uniform float u_time;
    uniform int   u_isPixelMelt; // 1 if Pixel Melt & Scatter, 0 otherwise

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

        // Hue drift: applied only if non-zero (defaults to 0.0 for pure original colors)
        if (abs(u_hueDrift) > 0.001)
        {
            col.rgb = rotateHue(col.rgb, u_hueDrift * 6.2831853);
        }

        // Palette presets (0 = Original: strictly untouched true colors; 1-5 = stylized presets)
        if (u_palette > 0)
        {
            float lum = clamp(dot(col.rgb, vec3(0.299, 0.587, 0.114)), 0.0, 1.0);

            if (u_palette == 1) // Neon Ultraviolet
            {
                vec3 c0 = vec3(0.08, 0.02, 0.22);
                vec3 c1 = vec3(0.92, 0.05, 0.55);
                vec3 c2 = vec3(0.0, 0.95, 0.95);
                vec3 pal = (lum < 0.5) ? mix(c0, c1, lum * 2.0) : mix(c1, c2, (lum - 0.5) * 2.0);
                col.rgb = mix(col.rgb, pal, 0.75);
            }
            else if (u_palette == 2) // Solar Flare
            {
                vec3 c0 = vec3(0.05, 0.01, 0.01);
                vec3 c1 = vec3(0.85, 0.12, 0.05);
                vec3 c2 = vec3(1.0, 0.85, 0.2);
                vec3 pal = (lum < 0.5) ? mix(c0, c1, lum * 2.0) : mix(c1, c2, (lum - 0.5) * 2.0);
                col.rgb = mix(col.rgb, pal, 0.80);
            }
            else if (u_palette == 3) // Monochrome Ice
            {
                vec3 c0 = vec3(0.04, 0.08, 0.15);
                vec3 c1 = vec3(0.25, 0.65, 0.85);
                vec3 c2 = vec3(0.92, 0.96, 1.0);
                vec3 pal = (lum < 0.5) ? mix(c0, c1, lum * 2.0) : mix(c1, c2, (lum - 0.5) * 2.0);
                col.rgb = mix(col.rgb, pal, 0.85);
            }
            else if (u_palette == 4) // Dark Floyd
            {
                vec3 c0 = vec3(0.02, 0.04, 0.03);
                vec3 c1 = vec3(0.0, 0.85, 0.45);
                vec3 c2 = vec3(0.95, 0.85, 0.15);
                vec3 pal = (lum < 0.5) ? mix(c0, c1, lum * 2.0) : mix(c1, c2, (lum - 0.5) * 2.0);
                col.rgb = mix(col.rgb, pal, 0.75);
            }
            else if (u_palette == 5) // Custom
            {
                vec3 c0 = vec3(0.02, 0.02, 0.03);
                vec3 c1 = u_customColour;
                vec3 c2 = vec3(1.0, 1.0, 1.0);
                vec3 pal = (lum < 0.5) ? mix(c0, c1, lum * 2.0) : mix(c1, c2, (lum - 0.5) * 2.0);
                col.rgb = mix(col.rgb, pal, 0.80);
            }
        }

        col.rgb *= u_brightness;

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

void VisualCanvas::setFullscreen(bool fs)
{
    isFullscreen = fs;
    repaint();
}

void VisualCanvas::mouseDoubleClick(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    if (onToggleFullscreen)
        onToggleFullscreen();
}

// ==============================================================================
// OpenGL Context Management & Block Mesh
// ==============================================================================
void VisualCanvas::newOpenGLContextCreated()
{
    initShaders();
    initBlockMesh();

    // Create default procedural Floyd test image pattern
    juce::Image defaultImg(juce::Image::ARGB, 512, 512, true);
    {
        juce::Graphics g(defaultImg);
        g.fillAll(juce::Colour(0xff0d1117));

        juce::ColourGradient grad(juce::Colour(0xff00b4d8), 256.0f, 256.0f,
                                  juce::Colour(0xffff007f), 480.0f, 480.0f, true);
        grad.addColour(0.3, juce::Colour(0xff7928ca));
        grad.addColour(0.6, juce::Colour(0xffff9900));
        grad.addColour(0.9, juce::Colour(0xff3ddc84));
        g.setGradientFill(grad);
        g.fillEllipse(64.0f, 64.0f, 384.0f, 384.0f);

        g.setColour(juce::Colour(0xff0a0d14));
        g.fillEllipse(128.0f, 128.0f, 256.0f, 256.0f);

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
    meltScatterShader.reset();
    trailsFeedbackShader.reset();
    presentShader.reset();
    releaseFBOs();
    releaseBlockMesh();
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

void VisualCanvas::initBlockMesh()
{
    releaseBlockMesh();

    struct BlockVertex
    {
        float blockCoordX, blockCoordY;
        float cornerX, cornerY;
        float cornerU, cornerV; // Exact continuous corner UV (sharp original image at rest)
        float centerU, centerV; // Block center UV (for discrete block sampling during scatter)
    };

    const int N = gridResolution;
    blockCount = N * N;
    std::vector<BlockVertex> vertices;
    vertices.reserve(blockCount * 4);
    std::vector<GLuint> indices;
    indices.reserve(blockCount * 6);

    const float corners[4][2] = {
        { -0.5f, -0.5f },
        {  0.5f, -0.5f },
        {  0.5f,  0.5f },
        { -0.5f,  0.5f }
    };

    GLuint baseVertex = 0;
    for (int y = 0; y < N; ++y)
    {
        for (int x = 0; x < N; ++x)
        {
            const float centerU = (static_cast<float>(x) + 0.5f) / static_cast<float>(N);
            const float centerV = (static_cast<float>(y) + 0.5f) / static_cast<float>(N);

            for (int c = 0; c < 4; ++c)
            {
                BlockVertex vert;
                vert.blockCoordX = static_cast<float>(x);
                vert.blockCoordY = static_cast<float>(y);
                vert.cornerX = corners[c][0];
                vert.cornerY = corners[c][1];
                vert.cornerU = (static_cast<float>(x) + 0.5f + corners[c][0]) / static_cast<float>(N);
                vert.cornerV = (static_cast<float>(y) + 0.5f + corners[c][1]) / static_cast<float>(N);
                vert.centerU = centerU;
                vert.centerV = centerV;
                vertices.push_back(vert);
            }

            indices.push_back(baseVertex + 0);
            indices.push_back(baseVertex + 1);
            indices.push_back(baseVertex + 2);
            indices.push_back(baseVertex + 0);
            indices.push_back(baseVertex + 2);
            indices.push_back(baseVertex + 3);

            baseVertex += 4;
        }
    }

    openGLContext.extensions.glGenBuffers(1, &blockVbo);
    openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, blockVbo);
    openGLContext.extensions.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(BlockVertex)), vertices.data(), GL_STATIC_DRAW);

    openGLContext.extensions.glGenBuffers(1, &blockIbo);
    openGLContext.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, blockIbo);
    openGLContext.extensions.glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(GLuint)), indices.data(), GL_STATIC_DRAW);

    openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
    openGLContext.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void VisualCanvas::releaseBlockMesh()
{
    if (blockVbo != 0)
    {
        openGLContext.extensions.glDeleteBuffers(1, &blockVbo);
        blockVbo = 0;
    }
    if (blockIbo != 0)
    {
        openGLContext.extensions.glDeleteBuffers(1, &blockIbo);
        blockIbo = 0;
    }
    blockCount = 0;
}

void VisualCanvas::initShaders()
{
    // 1. Generic Warp Shader (Smear, Ripple, Flow, Tiles)
    warpShader = std::make_unique<juce::OpenGLShaderProgram>(openGLContext);
    if (!warpShader->addVertexShader(commonVertexShader) ||
        !warpShader->addFragmentShader(warpFragmentShader) ||
        !warpShader->link())
    {
        DBG("Warp shader error: " + warpShader->getLastError());
    }

    // 2. Pixel Melt & Scatter Shader
    meltScatterShader = std::make_unique<juce::OpenGLShaderProgram>(openGLContext);
    if (!meltScatterShader->addVertexShader(meltScatterVertexShader) ||
        !meltScatterShader->addFragmentShader(meltScatterFragmentShader) ||
        !meltScatterShader->link())
    {
        DBG("Melt & Scatter shader error: " + meltScatterShader->getLastError());
    }

    // 3. Trails Feedback Shader
    trailsFeedbackShader = std::make_unique<juce::OpenGLShaderProgram>(openGLContext);
    if (!trailsFeedbackShader->addVertexShader(commonVertexShader) ||
        !trailsFeedbackShader->addFragmentShader(trailsFeedbackFragmentShader) ||
        !trailsFeedbackShader->link())
    {
        DBG("Trails shader error: " + trailsFeedbackShader->getLastError());
    }

    // 4. Presentation Shader
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

    // Query physical OpenGL pixel dimensions set by JUCE before renderOpenGL()
    GLint glVp[4] = { 0, 0, 0, 0 };
    glGetIntegerv(GL_VIEWPORT, glVp);
    const int physicalCanvasW = glVp[2];
    const int physicalCanvasH = glVp[3];

    if (physicalCanvasW <= 0 || physicalCanvasH <= 0)
        return;

    const auto bounds = getLocalBounds();
    const double renderScale = openGLContext.getRenderingScale();
    const double scale = (renderScale > 0.05) ? renderScale :
                         ((bounds.getWidth() > 0) ? (static_cast<double>(physicalCanvasW) / bounds.getWidth()) : 1.0);

    const int physicalStatusBarHeight = isFullscreen ? 0 : static_cast<int>(std::round(28.0 * scale));
    const int availableH = std::max(1, physicalCanvasH - physicalStatusBarHeight);

    // Determine preview aspect ratio:
    // In Fullscreen: Visualizer automatically adjusts to the landscape display aspect ratio (e.g. 16:9)
    // to fill the entire physical screen without distortion.
    // In Windowed: Respects user-chosen aspect button: 1080x1080 (1:1), 1080x1920 (9:16), 1920x1080 (16:9)
    float targetAspect = 1.0f;
    if (isFullscreen)
    {
        targetAspect = static_cast<float>(physicalCanvasW) / static_cast<float>(physicalCanvasH);
    }
    else
    {
        if (currentPreviewAspect == PreviewAspect::Portrait1080x1920)
            targetAspect = 1080.0f / 1920.0f;
        else if (currentPreviewAspect == PreviewAspect::Landscape1920x1080)
            targetAspect = 1920.0f / 1080.0f;
        else
            targetAspect = 1.0f; // Square 1080x1080
    }

    int letterboxW = physicalCanvasW;
    int letterboxH = availableH;
    int letterboxX = 0;
    int letterboxY = physicalStatusBarHeight;

    if (isFullscreen)
    {
        // Fullscreen visualizer adjusts to landscape display, covering 100% of the screen
        letterboxW = physicalCanvasW;
        letterboxH = physicalCanvasH;
        letterboxX = 0;
        letterboxY = 0;
    }
    else
    {
        // Windowed mode: FIT SCALING
        // Symmetrically centered both horizontally and vertically within available canvas area with equal margins.
        // Never anchored to bottom, top, or a corner.
        const float availAspect = static_cast<float>(physicalCanvasW) / static_cast<float>(availableH);

        if (availAspect > targetAspect)
        {
            // Canvas is wider than target aspect ratio (equal margins left and right)
            letterboxW = static_cast<int>(std::round(availableH * targetAspect));
            letterboxH = availableH;
            const int marginX = (physicalCanvasW - letterboxW) / 2;
            letterboxX = marginX;
            letterboxY = physicalStatusBarHeight; // Fills from status bar to top of canvas (0 top/bottom gap)
        }
        else
        {
            // Canvas is taller than target aspect ratio (equal margins top and bottom)
            letterboxW = physicalCanvasW;
            letterboxH = static_cast<int>(std::round(physicalCanvasW / targetAspect));
            const int marginY = (availableH - letterboxH) / 2;
            letterboxX = 0;
            letterboxY = physicalStatusBarHeight + marginY; // Exactly equal top and bottom margins!
        }
    }

    // Ensure Ping-Pong FBOs match target preview resolution
    int targetFboW = 1080;
    int targetFboH = 1080;
    if (isFullscreen || currentPreviewAspect == PreviewAspect::Landscape1920x1080)
    {
        targetFboW = 1920;
        targetFboH = 1080;
    }
    else if (currentPreviewAspect == PreviewAspect::Portrait1080x1920)
    {
        targetFboW = 1080;
        targetFboH = 1920;
    }

    if (fboPing == nullptr || fboPong == nullptr || fboWidth != targetFboW || fboHeight != targetFboH)
    {
        fboPing = std::make_unique<juce::OpenGLFrameBuffer>();
        fboPong = std::make_unique<juce::OpenGLFrameBuffer>();
        fboPing->initialise(openGLContext, targetFboW, targetFboH);
        fboPong->initialise(openGLContext, targetFboW, targetFboH);
        fboPing->clear(juce::Colours::black);
        fboPong->clear(juce::Colours::black);
        fboWidth = targetFboW;
        fboHeight = targetFboH;
    }

    // Query active visual parameters and audio snapshot
    VisualParameters params;
    {
        std::lock_guard<std::mutex> lock(paramMutex);
        params = parameters;
    }

    // Dynamic Pixel Size mesh update: live adjustment from Pixel Size slider (3 - 12 px)
    const int desiredGridRes = std::clamp(static_cast<int>(std::round(1080.0f / params.pixelSize)), 90, 360);
    if (desiredGridRes != gridResolution || blockVbo == 0)
    {
        gridResolution = desiredGridRes;
        currentPixelSize = params.pixelSize;
        initBlockMesh();
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
    // PASS 1: Render Active Scene into fboPing
    // ==========================================================================
    fboPing->makeCurrentRenderingTarget();
    glViewport(0, 0, fboWidth, fboHeight);

    if (params.scene == SceneType::PixelMeltScatter)
    {
        // 1a. Trails Feedback Pass: Blend previous frame dimmed by trails & decay
        if (trailsFeedbackShader != nullptr)
        {
            trailsFeedbackShader->use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, fboPong->getTextureID());
            trailsFeedbackShader->setUniform("u_prevTexture", 0);
            trailsFeedbackShader->setUniform("u_trails", params.trails);
            trailsFeedbackShader->setUniform("u_decay",  params.decay);

            const GLint posA = openGLContext.extensions.glGetAttribLocation(trailsFeedbackShader->getProgramID(), "aPosition");
            const GLint texA = openGLContext.extensions.glGetAttribLocation(trailsFeedbackShader->getProgramID(), "aTexCoord");

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

        // 1b. Render Pixel Melt & Scatter Blocks on top
        if (meltScatterShader != nullptr && blockVbo != 0 && blockIbo != 0)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            meltScatterShader->use();

            glActiveTexture(GL_TEXTURE0);
            if (hasImage.load())
                sourceTexture.bind();
            else
                defaultPatternTexture.bind();
            meltScatterShader->setUniform("u_imageTexture", 0);

            meltScatterShader->setUniform("u_bass",        audioSnap.bands.bass);
            meltScatterShader->setUniform("u_lowMid",      audioSnap.bands.lowMid);
            meltScatterShader->setUniform("u_mid",         audioSnap.bands.mid);
            meltScatterShader->setUniform("u_high",        audioSnap.bands.high);
            meltScatterShader->setUniform("u_beat",        audioSnap.bands.beat);

            meltScatterShader->setUniform("u_react",       params.react);
            meltScatterShader->setUniform("u_chaos",       params.chaos);
            meltScatterShader->setUniform("u_melt",        params.melt);
            meltScatterShader->setUniform("u_scatter",     params.scatter);
            meltScatterShader->setUniform("u_decay",       params.decay);
            meltScatterShader->setUniform("u_time",        timeSec);
            meltScatterShader->setUniform("u_aspectScale", sx, sy);

            const float blockSize = 2.0f / static_cast<float>(gridResolution);
            meltScatterShader->setUniform("u_blockSize",   blockSize);

            openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, blockVbo);
            openGLContext.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, blockIbo);

            const GLint blockCoordA = openGLContext.extensions.glGetAttribLocation(meltScatterShader->getProgramID(), "a_blockCoord");
            const GLint cornerA     = openGLContext.extensions.glGetAttribLocation(meltScatterShader->getProgramID(), "a_corner");
            const GLint cornerUvA   = openGLContext.extensions.glGetAttribLocation(meltScatterShader->getProgramID(), "a_cornerUV");
            const GLint centerUvA   = openGLContext.extensions.glGetAttribLocation(meltScatterShader->getProgramID(), "a_centerUV");

            constexpr int stride = 8 * sizeof(float);

            if (blockCoordA >= 0)
            {
                openGLContext.extensions.glEnableVertexAttribArray(static_cast<GLuint>(blockCoordA));
                openGLContext.extensions.glVertexAttribPointer(static_cast<GLuint>(blockCoordA), 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
            }
            if (cornerA >= 0)
            {
                openGLContext.extensions.glEnableVertexAttribArray(static_cast<GLuint>(cornerA));
                openGLContext.extensions.glVertexAttribPointer(static_cast<GLuint>(cornerA), 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
            }
            if (cornerUvA >= 0)
            {
                openGLContext.extensions.glEnableVertexAttribArray(static_cast<GLuint>(cornerUvA));
                openGLContext.extensions.glVertexAttribPointer(static_cast<GLuint>(cornerUvA), 2, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(float)));
            }
            if (centerUvA >= 0)
            {
                openGLContext.extensions.glEnableVertexAttribArray(static_cast<GLuint>(centerUvA));
                openGLContext.extensions.glVertexAttribPointer(static_cast<GLuint>(centerUvA), 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
            }

            glDrawElements(GL_TRIANGLES, blockCount * 6, GL_UNSIGNED_INT, 0);

            if (blockCoordA >= 0) openGLContext.extensions.glDisableVertexAttribArray(static_cast<GLuint>(blockCoordA));
            if (cornerA >= 0)     openGLContext.extensions.glDisableVertexAttribArray(static_cast<GLuint>(cornerA));
            if (cornerUvA >= 0)   openGLContext.extensions.glDisableVertexAttribArray(static_cast<GLuint>(cornerUvA));
            if (centerUvA >= 0)   openGLContext.extensions.glDisableVertexAttribArray(static_cast<GLuint>(centerUvA));

            openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
            openGLContext.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

            glDisable(GL_BLEND);
        }
    }
    else // Generic Warp Shader (Smear, Ripple, Flow, Tiles)
    {
        if (warpShader != nullptr)
        {
            warpShader->use();

            glActiveTexture(GL_TEXTURE0);
            if (hasImage.load())
                sourceTexture.bind();
            else
                defaultPatternTexture.bind();
            warpShader->setUniform("u_imageTexture", 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, fboPong->getTextureID());
            warpShader->setUniform("u_prevTexture", 1);

            warpShader->setUniform("u_bass",   audioSnap.bands.bass);
            warpShader->setUniform("u_lowMid", audioSnap.bands.lowMid);
            warpShader->setUniform("u_mid",    audioSnap.bands.mid);
            warpShader->setUniform("u_high",   audioSnap.bands.high);
            warpShader->setUniform("u_beat",   audioSnap.bands.beat);

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
        }
    }

    fboPing->releaseAsRenderingTarget();

    // ==========================================================================
    // PASS 2: Canvas Presentation to Centered Viewport
    // ==========================================================================
    if (presentShader != nullptr && fboPing != nullptr)
    {
        // 1. Clear outer screen background with dark letterbox bars
        glViewport(0, 0, physicalCanvasW, physicalCanvasH);
        glClearColor(0.04f, 0.05f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 2. Set centered letterbox viewport with equal margins
        glViewport(letterboxX, letterboxY, letterboxW, letterboxH);

        presentShader->use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fboPing->getTextureID());
        presentShader->setUniform("u_frameTexture", 0);

        presentShader->setUniform("u_brightness", params.brightness);
        presentShader->setUniform("u_hueDrift",   params.hueDrift);
        presentShader->setUniform("u_palette",    static_cast<int>(params.palette));

        const float cr = params.customColour.getFloatRed();
        const float cg = params.customColour.getFloatGreen();
        const float cb = params.customColour.getFloatBlue();
        presentShader->setUniform("u_customColour", cr, cg, cb);

        presentShader->setUniform("u_vignette",    params.vignette);
        presentShader->setUniform("u_time",        timeSec);
        presentShader->setUniform("u_isPixelMelt", (params.scene == SceneType::PixelMeltScatter) ? 1 : 0);

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

    // Swap Ping-Pong Framebuffers for next frame feedback
    std::swap(fboPing, fboPong);
}

// ==============================================================================
// 2D Component Paint (UI Overlays & Bottom Status Bar)
// ==============================================================================
void VisualCanvas::paint(juce::Graphics& g)
{
    if (isFullscreen)
        return; // In fullscreen: pure visual canvas, no UI overlays

    const auto bounds = getLocalBounds();
    const int w = bounds.getWidth();
    constexpr int statusBarHeight = 28;
    const auto viewportArea = bounds.withTrimmedBottom(statusBarHeight);

    if (isDragOver)
    {
        g.setColour(juce::Colour(0x5000b4d8));
        g.fillRect(viewportArea);
        g.setColour(juce::Colour(0xff00b4d8));
        g.drawRect(viewportArea, 3);
        g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
        g.drawText(hasImage.load() ? "RELEASE TO REPLACE IMAGE" : "RELEASE TO IMPORT IMAGE",
                   viewportArea, juce::Justification::centred, false);
    }

    // Bottom Status Line
    const auto statusArea = juce::Rectangle<int>(0, bounds.getBottom() - statusBarHeight, w, statusBarHeight);

    g.setColour(juce::Colour(0xff10131a));
    g.fillRect(statusArea);

    g.setColour(juce::Colour(0xff232732));
    g.drawHorizontalLine(statusArea.getY(), 0.0f, static_cast<float>(w));

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
