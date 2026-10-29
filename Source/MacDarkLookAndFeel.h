#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

namespace drinkfloyd
{

class MacDarkLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MacDarkLookAndFeel();
    ~MacDarkLookAndFeel() override = default;

    // Sliders
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override;

    // Buttons
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    // ComboBoxes
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    juce::Font getComboBoxFont(juce::ComboBox& box) override;

    // Popup Menus
    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;

    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted, bool isTicked,
                           bool hasSubMenu, const juce::String& text,
                           const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override;

    // Scrollbars
    void drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar,
                       int x, int y, int width, int height,
                       bool isScrollbarVertical, int thumbStartPosition,
                       int thumbSize, bool isMouseOver, bool isMouseDown) override;

    int getDefaultScrollbarWidth() override;

    // Text Editor & Alert Window
    void drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override;
    void drawAlertBox(juce::Graphics& g, juce::AlertWindow& alert, const juce::Rectangle<int>& textArea, juce::TextLayout& textLayout) override;

    // Color Constants (macOS Dark Palette)
    static const juce::Colour ColWindowBg;
    static const juce::Colour ColHeaderBg;
    static const juce::Colour ColSidebarBg;
    static const juce::Colour ColCardBg;
    static const juce::Colour ColControlBg;
    static const juce::Colour ColBorderSubtle;
    static const juce::Colour ColBorderMedium;
    static const juce::Colour ColTextPrimary;
    static const juce::Colour ColTextSecondary;
    static const juce::Colour ColTextMuted;
    static const juce::Colour ColAccentIndigo;
    static const juce::Colour ColAccentViolet;
    static const juce::Colour ColAccentSky;
    static const juce::Colour ColAccentRose;
    static const juce::Colour ColAccentEmerald;
    static const juce::Colour ColAccentAmber;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacDarkLookAndFeel)
};

} // namespace drinkfloyd
