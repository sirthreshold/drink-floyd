#include "MacDarkLookAndFeel.h"

namespace drinkfloyd
{

// macOS Dark Mode Palette Colors
const juce::Colour MacDarkLookAndFeel::ColWindowBg      = juce::Colour(0xff121217); // Obsidian base
const juce::Colour MacDarkLookAndFeel::ColHeaderBg      = juce::Colour(0xff181820); // Top bar
const juce::Colour MacDarkLookAndFeel::ColSidebarBg     = juce::Colour(0xff15151c); // Inspector panel
const juce::Colour MacDarkLookAndFeel::ColCardBg        = juce::Colour(0xff1d1d26); // Card box
const juce::Colour MacDarkLookAndFeel::ColControlBg     = juce::Colour(0xff171720); // Slider/input bg
const juce::Colour MacDarkLookAndFeel::ColBorderSubtle  = juce::Colour(0xff292938); // 1px border
const juce::Colour MacDarkLookAndFeel::ColBorderMedium  = juce::Colour(0xff39394d); // Stronger border
const juce::Colour MacDarkLookAndFeel::ColTextPrimary   = juce::Colour(0xfff4f4f6); // High contrast text
const juce::Colour MacDarkLookAndFeel::ColTextSecondary = juce::Colour(0xffa1a1b0); // Muted label text
const juce::Colour MacDarkLookAndFeel::ColTextMuted     = juce::Colour(0xff6b6b7d); // Dimmed readout
const juce::Colour MacDarkLookAndFeel::ColAccentIndigo  = juce::Colour(0xff6366f1); // Apple Indigo
const juce::Colour MacDarkLookAndFeel::ColAccentViolet  = juce::Colour(0xff8b5cf6); // Apple Purple
const juce::Colour MacDarkLookAndFeel::ColAccentSky     = juce::Colour(0xff0ea5e9); // Apple Sky Blue
const juce::Colour MacDarkLookAndFeel::ColAccentRose    = juce::Colour(0xfff43f5e); // Apple Rose/Red
const juce::Colour MacDarkLookAndFeel::ColAccentEmerald = juce::Colour(0xff10b981); // Apple Mint/Green
const juce::Colour MacDarkLookAndFeel::ColAccentAmber   = juce::Colour(0xfff59e0b); // Apple Amber/Gold

MacDarkLookAndFeel::MacDarkLookAndFeel()
{
    // Global theme overrides
    setColour(juce::ResizableWindow::backgroundColourId, ColWindowBg);
    setColour(juce::Label::textColourId, ColTextPrimary);

    // Text Editor Colors
    setColour(juce::TextEditor::backgroundColourId, ColControlBg);
    setColour(juce::TextEditor::textColourId, ColTextPrimary);
    setColour(juce::TextEditor::outlineColourId, ColBorderSubtle);
    setColour(juce::TextEditor::focusedOutlineColourId, ColAccentIndigo);

    // ComboBox Popup Colors
    setColour(juce::PopupMenu::backgroundColourId, ColHeaderBg);
    setColour(juce::PopupMenu::textColourId, ColTextPrimary);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, ColAccentIndigo);
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
}

// ==============================================================================
// SLIDERS: Sleek macOS Capsule Track + Silver Ring Thumb
// ==============================================================================
void MacDarkLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                         juce::Slider::SliderStyle /*style*/, juce::Slider& slider)
{
    const float trackH = 4.5f;
    const float trackY = static_cast<float>(y) + (static_cast<float>(height) - trackH) * 0.5f;
    const float trackX = static_cast<float>(x);
    const float trackW = static_cast<float>(width);

    // Track Background Capsule
    juce::Rectangle<float> trackRect(trackX, trackY, trackW, trackH);
    g.setColour(ColControlBg);
    g.fillRoundedRectangle(trackRect, 2.25f);

    g.setColour(ColBorderSubtle);
    g.drawRoundedRectangle(trackRect, 2.25f, 1.0f);

    // Filled Track Capsule (Progress)
    const float filledW = std::clamp(sliderPos - trackX, 0.0f, trackW);
    if (filledW > 1.0f)
    {
        juce::Rectangle<float> fillRect(trackX, trackY, filledW, trackH);

        // Apple Indigo to Sky subtle gradient
        juce::ColourGradient grad(ColAccentIndigo, trackX, trackY,
                                  ColAccentSky, trackX + trackW, trackY, false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(fillRect, 2.25f);
    }

    // Circular macOS Thumb Knob
    const float thumbDiameter = 14.0f;
    const float thumbX = sliderPos - thumbDiameter * 0.5f;
    const float thumbY = static_cast<float>(y) + (static_cast<float>(height) - thumbDiameter) * 0.5f;
    juce::Rectangle<float> thumbRect(thumbX, thumbY, thumbDiameter, thumbDiameter);

    // Soft drop shadow
    g.setColour(juce::Colour(0x44000000));
    g.fillEllipse(thumbRect.translated(0.0f, 1.5f));

    // Outer Silver / Platinum Ring
    const bool isHovered = slider.isMouseOverOrDragging();
    g.setColour(isHovered ? juce::Colours::white : juce::Colour(0xffe2e2ea));
    g.fillEllipse(thumbRect);

    // Inner Dark Core with subtle highlight
    juce::Rectangle<float> innerCore = thumbRect.reduced(3.5f);
    g.setColour(isHovered ? ColAccentIndigo : juce::Colour(0xff2b2b38));
    g.fillEllipse(innerCore);
}

// ==============================================================================
// BUTTONS: macOS Sonoma Rounded Glass / Matte Buttons
// ==============================================================================
void MacDarkLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                             const juce::Colour& backgroundColour,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    const float cornerRadius = 6.0f;

    juce::Colour baseColor = backgroundColour;

    // Check if this is a default grey button
    if (baseColor == juce::Colour(0xff161922) || baseColor == juce::Colour(0xff1a1e28) ||
        baseColor == juce::Colour(0xff222a38) || baseColor == juce::Colour(0xff152230))
    {
        baseColor = ColCardBg;
    }

    if (shouldDrawButtonAsDown)
        baseColor = baseColor.darker(0.20f);
    else if (shouldDrawButtonAsHighlighted)
        baseColor = baseColor.brighter(0.18f);

    // Button Body
    g.setColour(baseColor);
    g.fillRoundedRectangle(bounds, cornerRadius);

    // Subtle 1px Outline
    juce::Colour outlineCol = ColBorderSubtle;
    if (shouldDrawButtonAsHighlighted)
        outlineCol = ColBorderMedium;
    if (button.getToggleState())
        outlineCol = ColAccentIndigo;

    g.setColour(outlineCol);
    g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);

    // Top edge gloss highlight
    if (!shouldDrawButtonAsDown && bounds.getHeight() > 14.0f)
    {
        g.setColour(juce::Colour(0x18ffffff));
        g.drawHorizontalLine(static_cast<int>(bounds.getY() + 1.0f), bounds.getX() + 3.0f, bounds.getRight() - 3.0f);
    }
}

void MacDarkLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                       bool /*shouldDrawButtonAsHighlighted*/,
                                       bool shouldDrawButtonAsDown)
{
    juce::Font font(juce::FontOptions("Segoe UI Variable", 11.5f, juce::Font::bold));
    g.setFont(font);

    juce::Colour textCol = button.findColour(button.getToggleState() ? juce::TextButton::textColourOnId
                                                                    : juce::TextButton::textColourOffId);

    if (textCol == juce::Colour(0xff8a93a5) || textCol == juce::Colour(0xffd5dbe5))
        textCol = ColTextPrimary;

    if (shouldDrawButtonAsDown)
        textCol = textCol.withAlpha(0.70f);

    g.setColour(textCol);
    g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, true);
}

// ==============================================================================
// COMBOBOXES: macOS Rounded Dropdown with Crisp Chevron
// ==============================================================================
void MacDarkLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                     int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                     juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<float>(0.5f, 0.5f, static_cast<float>(width) - 1.0f, static_cast<float>(height) - 1.0f);
    const float cornerRadius = 6.0f;

    // Body Fill
    g.setColour(isButtonDown ? ColControlBg.darker(0.1f) : ColControlBg);
    g.fillRoundedRectangle(bounds, cornerRadius);

    // Border
    const bool isHover = box.isMouseOver();
    g.setColour(isHover ? ColBorderMedium : ColBorderSubtle);
    g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);

    // Minimalist Apple-Style Chevron '⌄'
    const float arrowCenterX = static_cast<float>(width) - 14.0f;
    const float arrowCenterY = static_cast<float>(height) * 0.5f - 1.0f;
    const float arrowSize = 3.5f;

    juce::Path arrow;
    arrow.startNewSubPath(arrowCenterX - arrowSize, arrowCenterY - 1.5f);
    arrow.lineTo(arrowCenterX, arrowCenterY + arrowSize - 1.5f);
    arrow.lineTo(arrowCenterX + arrowSize, arrowCenterY - 1.5f);

    g.setColour(ColTextSecondary);
    g.strokePath(arrow, juce::PathStrokeType(1.4f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
}

juce::Font MacDarkLookAndFeel::getComboBoxFont(juce::ComboBox& /*box*/)
{
    return juce::Font(juce::FontOptions("Segoe UI Variable", 12.0f, juce::Font::plain));
}

// ==============================================================================
// POPUP MENU: macOS Dark Frosted Glass Menu
// ==============================================================================
void MacDarkLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    const auto bounds = juce::Rectangle<float>(0.5f, 0.5f, static_cast<float>(width) - 1.0f, static_cast<float>(height) - 1.0f);
    g.setColour(ColHeaderBg);
    g.fillRoundedRectangle(bounds, 8.0f);

    g.setColour(ColBorderMedium);
    g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
}

void MacDarkLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                          bool isSeparator, bool isActive, bool isHighlighted, bool isTicked,
                                          bool hasSubMenu, const juce::String& text,
                                          const juce::String& shortcutKeyText,
                                          const juce::Drawable* icon, const juce::Colour* textColour)
{
    if (isSeparator)
    {
        auto r = area.reduced(8, 0);
        g.setColour(ColBorderSubtle);
        g.drawHorizontalLine(r.getCentreY(), static_cast<float>(r.getX()), static_cast<float>(r.getRight()));
        return;
    }

    auto itemArea = area.reduced(4, 2);

    if (isHighlighted && isActive)
    {
        g.setColour(ColAccentIndigo);
        g.fillRoundedRectangle(itemArea.toFloat(), 5.0f);
    }

    g.setFont(juce::FontOptions("Segoe UI Variable", 12.0f, juce::Font::plain));

    juce::Colour itemTextColour = ColTextPrimary;
    if (textColour != nullptr)
        itemTextColour = *textColour;
    else if (!isActive)
        itemTextColour = ColTextMuted;
    else if (isHighlighted)
        itemTextColour = juce::Colours::white;

    g.setColour(itemTextColour);

    auto textRect = itemArea.reduced(8, 0);
    if (isTicked)
    {
        // Apple-style checkmark
        juce::Path tick;
        tick.startNewSubPath(textRect.getX() - 6.0f, textRect.getCentreY() - 1.0f);
        tick.lineTo(textRect.getX() - 3.0f, textRect.getCentreY() + 2.5f);
        tick.lineTo(textRect.getX() + 2.5f, textRect.getCentreY() - 3.5f);
        g.strokePath(tick, juce::PathStrokeType(1.5f));
    }

    g.drawText(text, textRect, juce::Justification::centredLeft, true);
    juce::ignoreUnused(shortcutKeyText, icon, hasSubMenu);
}

// ==============================================================================
// SCROLLBAR: Minimalist macOS Capsule Scrollbar
// ==============================================================================
int MacDarkLookAndFeel::getDefaultScrollbarWidth()
{
    return 6;
}

void MacDarkLookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar& /*scrollbar*/,
                                      int x, int y, int width, int height,
                                      bool isScrollbarVertical, int thumbStartPosition,
                                      int thumbSize, bool isMouseOver, bool isMouseDown)
{
    if (thumbSize <= 0)
        return;

    juce::Rectangle<float> thumbBounds;
    if (isScrollbarVertical)
    {
        thumbBounds = juce::Rectangle<float>(static_cast<float>(x) + 1.0f,
                                            static_cast<float>(thumbStartPosition),
                                            static_cast<float>(width) - 2.0f,
                                            static_cast<float>(thumbSize));
    }
    else
    {
        thumbBounds = juce::Rectangle<float>(static_cast<float>(thumbStartPosition),
                                            static_cast<float>(y) + 1.0f,
                                            static_cast<float>(thumbSize),
                                            static_cast<float>(height) - 2.0f);
    }

    float alpha = isMouseDown ? 0.85f : (isMouseOver ? 0.65f : 0.35f);
    g.setColour(juce::Colour(0xff8a8a9e).withAlpha(alpha));
    g.fillRoundedRectangle(thumbBounds, 2.5f);
}

// ==============================================================================
// TEXT EDITOR OUTLINE
// ==============================================================================
void MacDarkLookAndFeel::drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor)
{
    const auto r = juce::Rectangle<float>(0.5f, 0.5f, static_cast<float>(width) - 1.0f, static_cast<float>(height) - 1.0f);
    if (textEditor.hasKeyboardFocus(true))
    {
        g.setColour(ColAccentIndigo);
        g.drawRoundedRectangle(r, 6.0f, 1.5f);
    }
    else
    {
        g.setColour(ColBorderSubtle);
        g.drawRoundedRectangle(r, 6.0f, 1.0f);
    }
}

// ==============================================================================
// ALERT WINDOW: Sleek macOS Modal Sheet
// ==============================================================================
void MacDarkLookAndFeel::drawAlertBox(juce::Graphics& g, juce::AlertWindow& alert,
                                     const juce::Rectangle<int>& textArea, juce::TextLayout& textLayout)
{
    auto bounds = alert.getLocalBounds().toFloat();

    // Dark sleek sheet
    g.setColour(ColHeaderBg);
    g.fillRoundedRectangle(bounds, 12.0f);

    g.setColour(ColBorderMedium);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 12.0f, 1.0f);

    // Title / Text Layout
    textLayout.draw(g, textArea.toFloat());
}

} // namespace drinkfloyd
