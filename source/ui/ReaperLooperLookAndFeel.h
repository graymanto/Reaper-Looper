#pragma once

#include <JuceHeader.h>
#include "Theme.h"

namespace reaperlooper
{

/**
 * A minimal LookAndFeel_V4 override applying the Theme (see Theme.h) to
 * every standard JUCE control. Install once on the top-level editor via
 * setLookAndFeel().
 *
 * Ported from `sequenced`'s SequencedLookAndFeel, minus the custom Geist
 * typeface loading (getTypefaceForFont() + the 18 BinaryData typeface
 * pointers) - this project doesn't bundle font binaries yet (see Theme.h);
 * everything else (the gradient-fill button motif, the double-ring toggle
 * glow, the thin accent scrollbar) is unchanged.
 */
class ReaperLooperLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ReaperLooperLookAndFeel();

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& bgColour,
                                bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;

    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int width, int height,
                        bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                        bool isMouseOver, bool isMouseDown) override;

    int getDefaultScrollbarWidth() override;

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
};

} // namespace reaperlooper
