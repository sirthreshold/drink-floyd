#include "VisualCanvas.h"
#include "HeaderBar.h"

namespace drinkfloyd
{

VisualCanvas::VisualCanvas(HeaderBar& headerBar)
    : header(headerBar)
{
    // Run at 60 FPS repaint timer for smooth real-time preview animation
    startTimerHz(60);
}

VisualCanvas::~VisualCanvas()
{
    stopTimer();
}

void VisualCanvas::timerCallback()
{
    header.recordFrame();
    repaint();
}

void VisualCanvas::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds();
    const float w = static_cast<float>(bounds.getWidth());
    const float h = static_cast<float>(bounds.getHeight());

    // Deep charcoal background
    g.setColour(juce::Colour(0xff0a0c10));
    g.fillRect(bounds);

    // Subtle background grid
    g.setColour(juce::Colour(0x0a, 0xff, 0xff, 0xff));
    constexpr int gridSize = 40;
    for (int x = 0; x < bounds.getWidth(); x += gridSize)
        g.drawVerticalLine(x, 0.0f, h);
    for (int y = 0; y < bounds.getHeight(); y += gridSize)
        g.drawHorizontalLine(y, 0.0f, w);

    // Aspect Ratio Guide Box (Square 1:1 preview area by default)
    const float canvasDim = juce::jmin(w * 0.82f, h * 0.82f);
    const juce::Rectangle<float> viewArea((w - canvasDim) * 0.5f,
                                          (h - canvasDim) * 0.5f,
                                          canvasDim, canvasDim);

    // Viewport border & subtle shadow
    g.setColour(juce::Colour(0xff12151d));
    g.fillRect(viewArea);

    g.setColour(juce::Colour(0xff1f2533));
    g.drawRect(viewArea, 1.5f);

    // Center Crosshairs
    const float cx = viewArea.getCentreX();
    const float cy = viewArea.getCentreY();
    g.setColour(juce::Colour(0x30, 0x00, 0xb4, 0xd8));
    g.drawLine(cx - 20.0f, cy, cx + 20.0f, cy, 1.0f);
    g.drawLine(cx, cy - 20.0f, cx, cy + 20.0f, 1.0f);

    // Canvas Status text
    g.setColour(juce::Colour(0xff8a93a5));
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText("DRINK FLOYD VISUAL ENGINE",
               viewArea.toNearestInt(),
               juce::Justification::centred,
               false);

    g.setColour(juce::Colour(0xff525b6c));
    g.setFont(juce::FontOptions(11.5f, juce::Font::plain));
    auto subTextRect = viewArea.toNearestInt();
    subTextRect.translate(0, 24);
    g.drawText("CANVAS READY | PHASE 0 STANDALONE",
               subTextRect,
               juce::Justification::centred,
               false);

    // Resolution info badge inside canvas preview
    juce::Rectangle<float> resBadge(viewArea.getX() + 10.0f, viewArea.getY() + 10.0f, 100.0f, 20.0f);
    g.setColour(juce::Colour(0x60, 0x1a, 0x1e, 0x27));
    g.fillRoundedRectangle(resBadge, 4.0f);
    g.setColour(juce::Colour(0xff00b4d8));
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText("1:1 (1080x1080)", resBadge.toNearestInt(), juce::Justification::centred, false);
}

void VisualCanvas::resized()
{
}

} // namespace drinkfloyd
