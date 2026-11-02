#pragma once

#include <juce_graphics/juce_graphics.h>

/**
 * Visual language, ported from `sequenced`'s Theme.h (colours, geometry
 * and the track-colour palette are unchanged - this IS the product's
 * identity, and there was no reason to redesign it). Two deliberate
 * differences from the original:
 *
 *  - Font family names are JUCE's own default sans/mono rather than
 *    "Geist"/"Geist Mono" - this project doesn't bundle the Geist font
 *    binaries (sequenced does, via juce_add_binary_data). uiFont()/
 *    monoFont() keep the same call shape, so embedding real fonts later
 *    is a one-line change confined to this file.
 *  - `trackColours` MUST stay in the same order as LaunchpadProfile's
 *    `kApcColourVelocity`/`kLpColourRGB` (Red, Orange, Yellow, Lime, Cyan,
 *    Blue, Violet, Pink) - a column's on-screen colour and its hardware
 *    pad colour are the same conceptual palette index, by design.
 */
namespace reaperlooper::Theme
{
    //==============================================================================
    // Background layers
    inline const juce::Colour background     { 0xFF0D0F1A };
    inline const juce::Colour backgroundDeep { 0xFF070912 };
    inline const juce::Colour surface        { 0xFF161829 };
    inline const juce::Colour elevated       { 0xFF1E2235 };
    inline const juce::Colour glass          { 0xD9111622 };
    inline const juce::Colour glassHi        { 0xEE151B2A };
    inline const juce::Colour cellBorder     { 0xFF252840 };
    inline const juce::Colour borderHi       { 0xFF303850 };

    //==============================================================================
    // Accent
    inline const juce::Colour accent     { 0xFF7B6EF6 };
    inline const juce::Colour accentGlow { 0x407B6EF6 };
    inline const juce::Colour accentDim  { 0x157B6EF6 };

    //==============================================================================
    // Buttons
    inline const juce::Colour btnNormal { 0xFF1E2235 };
    inline const juce::Colour btnHover  { 0xFF2A2D48 };
    inline const juce::Colour btnActive { 0xFF7B6EF6 };
    inline const juce::Colour btnRecord { 0xFFCC2200 };

    //==============================================================================
    // Text
    inline const juce::Colour textPrimary   { 0xFFE8E8F0 };
    inline const juce::Colour textSecondary { 0xFFAAABBF };
    inline const juce::Colour textDim       { 0xFF6B6D8A };

    //==============================================================================
    // Status
    inline const juce::Colour playGreen { 0xFF4CAF50 };
    inline const juce::Colour recordRed { 0xFFFF4444 };

    //==============================================================================
    // Track colours (8 columns) - same order as the APC mini mk2's palette.
    inline const juce::Colour trackColours[8] = {
        juce::Colour (0xFFFF4444), // 0 Red
        juce::Colour (0xFFFF8C00), // 1 Orange
        juce::Colour (0xFFFFD700), // 2 Yellow
        juce::Colour (0xFF44DD44), // 3 Lime
        juce::Colour (0xFF00CCCC), // 4 Cyan
        juce::Colour (0xFF4488FF), // 5 Blue
        juce::Colour (0xFFAA44FF), // 6 Violet
        juce::Colour (0xFFFF44AA), // 7 Pink
    };

    //==============================================================================
    // Geometry
    inline constexpr float cornerSmall = 6.0f;
    inline constexpr float cornerCard  = 10.0f;
    inline constexpr float cornerLarge = 14.0f;
    inline constexpr float corner      = cornerSmall; // alias

    /** A comfortable minimum size for a clickable control (points). No touchscreen target on this project (desktop-only VST3), but small mis-clicks are still worth avoiding. */
    inline constexpr int minClickTarget = 28;

    inline int clickSize (int computed) noexcept
    {
        return computed > minClickTarget ? computed : minClickTarget;
    }

    //==============================================================================
    // Fonts
    inline juce::Font uiFont (float height, const juce::String& style = {})
    {
        auto options = juce::FontOptions().withName (juce::Font::getDefaultSansSerifFontName()).withHeight (height);
        if (style.isNotEmpty())
            options = options.withStyle (style);
        return juce::Font (options);
    }

    inline juce::Font monoFont (float height, const juce::String& style = {})
    {
        auto options = juce::FontOptions().withName (juce::Font::getDefaultMonospacedFontName()).withHeight (height);
        if (style.isNotEmpty())
            options = options.withStyle (style);
        return juce::Font (options);
    }
}
