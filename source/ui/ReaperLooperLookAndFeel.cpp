#include "ReaperLooperLookAndFeel.h"

namespace reaperlooper
{

ReaperLooperLookAndFeel::ReaperLooperLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, Theme::background);
    setColour (juce::TextButton::buttonColourId, Theme::btnNormal);
    setColour (juce::TextButton::buttonOnColourId, Theme::btnActive);
    setColour (juce::TextButton::textColourOffId, Theme::textPrimary);
    setColour (juce::TextButton::textColourOnId, Theme::textPrimary);
    setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, Theme::textPrimary);
    setColour (juce::ComboBox::backgroundColourId, Theme::btnNormal);
    setColour (juce::ComboBox::textColourId, Theme::textPrimary);
    setColour (juce::ComboBox::outlineColourId, Theme::cellBorder);
    setColour (juce::ComboBox::arrowColourId, Theme::textSecondary);
    setColour (juce::Slider::backgroundColourId, Theme::surface);
    setColour (juce::Slider::thumbColourId, Theme::accent);
    setColour (juce::Slider::trackColourId, Theme::elevated);
    setColour (juce::ScrollBar::thumbColourId, Theme::accent);
    setColour (juce::ScrollBar::trackColourId, juce::Colours::transparentBlack);
    setColour (juce::ToggleButton::textColourId, Theme::textPrimary);
    setColour (juce::ToggleButton::tickColourId, Theme::accent);
    setColour (juce::ToggleButton::tickDisabledColourId, Theme::textDim);
    setColour (juce::PopupMenu::backgroundColourId, Theme::surface);
    setColour (juce::PopupMenu::textColourId, Theme::textPrimary);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Theme::accentDim);
    setColour (juce::PopupMenu::highlightedTextColourId, Theme::textPrimary);
    setColour (juce::AlertWindow::backgroundColourId, Theme::surface);
    setColour (juce::AlertWindow::textColourId, Theme::textPrimary);
}

void ReaperLooperLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& btn,
                                                      const juce::Colour& bgColour,
                                                      bool highlighted, bool down)
{
    const auto bounds = btn.getLocalBounds().toFloat().reduced (0.5f);

    juce::Colour bg;
    if (down)
        bg = Theme::accent.darker (0.3f);
    else if (btn.getToggleState())
        bg = Theme::accent.withAlpha (0.75f);
    else if (highlighted)
        bg = Theme::btnHover;
    else
        bg = bgColour;

    g.setGradientFill (juce::ColourGradient (bg.brighter (0.06f), bounds.getX(), bounds.getY(),
                                              bg.darker (0.10f), bounds.getX(), bounds.getBottom(), false));
    g.fillRoundedRectangle (bounds, Theme::cornerSmall);

    if (btn.getToggleState())
    {
        g.setColour (Theme::accent.withAlpha (0.28f));
        g.drawRoundedRectangle (bounds.expanded (2.0f), Theme::cornerSmall + 2.0f, 3.0f);
        g.setColour (Theme::accent);
        g.drawRoundedRectangle (bounds, Theme::cornerSmall, 1.5f);
    }
    else
    {
        g.setColour (bg.brighter (0.15f));
        g.drawRoundedRectangle (bounds, Theme::cornerSmall, 0.5f);
    }
}

void ReaperLooperLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& btn, bool, bool)
{
    const float h = static_cast<float> (btn.getHeight());
    g.setColour (btn.getToggleState() ? juce::Colours::white : Theme::textPrimary);
    g.setFont (Theme::uiFont (h * 0.40f, "Medium"));
    g.drawFittedText (btn.getButtonText(), btn.getLocalBounds().reduced (2), juce::Justification::centred, 1);
}

void ReaperLooperLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    const auto bounds = juce::Rectangle<int> (0, 0, w, h).toFloat().reduced (0.5f);
    g.setGradientFill (juce::ColourGradient (Theme::glassHi.brighter (0.04f), bounds.getX(), bounds.getY(),
                                              Theme::glass, bounds.getX(), bounds.getBottom(), false));
    g.fillRoundedRectangle (bounds, Theme::cornerSmall);
    g.setColour (Theme::borderHi.withAlpha (0.72f));
    g.drawRoundedRectangle (bounds, Theme::cornerSmall, 0.8f);

    const float arrowH = static_cast<float> (h) * 0.28f;
    const float arrowW = arrowH * 1.1f;
    const float arrowX = static_cast<float> (w) - arrowW - static_cast<float> (h) * 0.3f;
    const float arrowY = (static_cast<float> (h) - arrowH) * 0.5f;

    juce::Path arrow;
    arrow.addTriangle (arrowX, arrowY, arrowX + arrowW, arrowY, arrowX + arrowW * 0.5f, arrowY + arrowH);
    g.setColour (Theme::textSecondary);
    g.fillPath (arrow);
}

void ReaperLooperLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar& scrollbar, int x, int y,
                                               int width, int height, bool isScrollbarVertical,
                                               int thumbStartPosition, int thumbSize, bool, bool)
{
    g.setColour (juce::Colours::transparentBlack);
    g.fillRect (x, y, width, height);

    if (thumbSize <= 0)
        return;

    const int minThumb = 20;
    const int ts = juce::jmax (thumbSize, minThumb);

    juce::Rectangle<int> thumb;
    if (isScrollbarVertical)
    {
        const int tx = x + (width - 4) / 2;
        thumb = { tx, y + thumbStartPosition, 4, ts };
    }
    else
    {
        const int ty = y + (height - 4) / 2;
        thumb = { x + thumbStartPosition, ty, ts, 4 };
    }

    g.setColour (scrollbar.isMouseOver() ? Theme::accent : Theme::accent.withAlpha (0.6f));
    g.fillRoundedRectangle (thumb.toFloat(), 2.0f);
}

int ReaperLooperLookAndFeel::getDefaultScrollbarWidth() { return 8; }

juce::Font ReaperLooperLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return Theme::uiFont (juce::jlimit (11.0f, 15.5f, static_cast<float> (buttonHeight) * 0.42f), "Medium");
}

juce::Font ReaperLooperLookAndFeel::getComboBoxFont (juce::ComboBox&) { return Theme::uiFont (13.0f, "Medium"); }
juce::Font ReaperLooperLookAndFeel::getPopupMenuFont() { return Theme::uiFont (14.0f, "Medium"); }

} // namespace reaperlooper
