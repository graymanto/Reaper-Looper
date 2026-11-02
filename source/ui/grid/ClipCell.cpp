#include "ClipCell.h"
#include <cmath>

namespace reaperlooper
{

ClipCell::ClipCell (int logicalTrack, int logicalRow, Callbacks cbs)
    : track_ (logicalTrack), row_ (logicalRow), cbs_ (std::move (cbs))
{
    setRepaintsOnMouseActivity (false); // handled manually - avoids a repaint on every pixel of mouse movement
}

void ClipCell::setState (const State& s, juce::Colour trackColour, double beatsPerBar, double progressFraction)
{
    state_ = s;
    trackColour_ = trackColour;
    beatsPerBar_ = beatsPerBar > 0.0 ? beatsPerBar : 4.0;
    progressFraction_ = progressFraction;
    repaint();
}

void ClipCell::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    const bool pulse = (juce::Time::getMillisecondCounter() % 500) < 250;

    juce::Colour bg;
    if (state_.hasClip && state_.isActive && state_.isStopQueued)
        bg = trackColour_.withAlpha (pulse ? 0.18f : 0.28f);
    else if (state_.hasClip && state_.isActive)
        bg = trackColour_.withAlpha (0.20f);
    else if (state_.hasClip && state_.isQueued)
        bg = trackColour_.withAlpha (pulse ? 0.12f : 0.20f);
    else if (state_.hasClip)
        bg = Theme::glassHi.withAlpha (0.76f);
    else
        bg = Theme::backgroundDeep.withAlpha (0.82f);

    if (hovered_) bg = bg.brighter (0.15f);
    if (pressed_) bg = bg.darker (0.25f);

    if (state_.hasClip)
        g.setGradientFill (juce::ColourGradient (bg.brighter (0.10f), bounds.getX(), bounds.getY(),
                                                  bg.darker (0.12f), bounds.getX(), bounds.getBottom(), false));
    else
        g.setColour (bg);
    g.fillRoundedRectangle (bounds, Theme::cornerSmall);

    g.setColour (trackColour_.withAlpha (state_.hasClip ? 0.13f : 0.025f));
    g.fillRoundedRectangle (bounds, Theme::cornerSmall);

    const float borderWidth = state_.isActive ? 1.5f : 1.0f;
    juce::Colour borderColour = Theme::borderHi.withAlpha (0.52f);
    if (state_.isActive) borderColour = trackColour_.withAlpha (0.80f);
    else if (state_.isQueued) borderColour = trackColour_.withAlpha (0.50f);
    g.setColour (borderColour);
    g.drawRoundedRectangle (bounds, Theme::cornerSmall, borderWidth);

    if (state_.hasClip)
    {
        const float strokeW = state_.isActive ? 2.4f : 1.4f;
        const float alpha = state_.isActive ? 0.30f : (state_.isQueued ? 0.20f : 0.13f);
        g.setColour (trackColour_.withAlpha (alpha));
        g.drawRoundedRectangle (bounds.expanded (1.5f), Theme::cornerSmall + 1.5f, strokeW);
    }

    if (! state_.hasClip)
    {
        if (hovered_)
        {
            g.setColour (Theme::textDim.withAlpha (0.78f));
            g.setFont (Theme::monoFont (bounds.getHeight() * 0.46f));
            g.drawText ("+", bounds.getSmallestIntegerContainer(), juce::Justification::centred);
        }
        return;
    }

    //==============================================================================
    // The circular badge.

    const float bw = bounds.getWidth();
    const float bh = bounds.getHeight();
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float radius = juce::jmin (bw, bh) * 0.34f;

    if (state_.isActive)
    {
        const std::pair<float, float> bloomLayers[] = { { 1.80f, 0.04f }, { 1.45f, 0.07f }, { 1.20f, 0.13f } };
        for (const auto& [mult, alpha] : bloomLayers)
        {
            g.setColour (trackColour_.withAlpha (alpha));
            g.fillEllipse (juce::Rectangle<float> (radius * mult * 2.0f, radius * mult * 2.0f).withCentre ({ cx, cy }));
        }
    }

    drawTickRing (g, cx, cy, radius);

    if (state_.isActive)
        g.setGradientFill (juce::ColourGradient (trackColour_.withAlpha (0.22f), cx, cy,
                                                  Theme::surface, cx + radius, cy, true));
    else
        g.setColour (Theme::surface);
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre ({ cx, cy }));

    g.setColour (state_.isActive ? trackColour_ : trackColour_.withAlpha (0.55f));
    g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre ({ cx, cy }), state_.isActive ? 2.0f : 1.5f);

    if (state_.isActive)
        drawProgressSweep (g, cx, cy, radius);

    drawClipTypeIcon (g, cx, cy, radius * 0.9f);

    //==============================================================================
    // Badges

    if (state_.isQueued || state_.isStopQueued)
    {
        const float badgeSize = bh * 0.18f;
        const juce::Rectangle<float> badgeArea (bounds.getX() + 4.0f, bounds.getY() + 4.0f, badgeSize, badgeSize);
        juce::Path p;
        if (state_.isStopQueued)
            p.addRectangle (badgeArea);
        else
            p.addTriangle (badgeArea.getX(), badgeArea.getY(),
                           badgeArea.getRight(), badgeArea.getCentreY(),
                           badgeArea.getX(), badgeArea.getBottom());
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.fillPath (p);
    }

    g.setColour (juce::Colours::white.withAlpha (0.38f));
    g.setFont (Theme::uiFont (juce::jmin (bw, bh) * 0.19f));
    const double bars = state_.lengthBeats / beatsPerBar_;
    const juce::String lenText = std::abs (bars - std::round (bars)) < 1.0e-6
        ? juce::String (static_cast<int> (std::round (bars))) + " bar" + (std::abs (bars - 1.0) < 1.0e-6 ? "" : "s")
        : juce::String (bars, 1) + " bars";
    g.drawText (lenText, bounds.reduced (4.0f).getSmallestIntegerContainer(), juce::Justification::bottomRight);
}

void ClipCell::drawTickRing (juce::Graphics& g, float cx, float cy, float radius) const
{
    constexpr int numTicks = 40;
    const float innerR = radius * 0.95f;
    const float outerR = radius * 1.12f;

    g.setColour (trackColour_.withAlpha (state_.isActive ? 0.55f : 0.25f));
    for (int i = 0; i < numTicks; ++i)
    {
        const float angle = (static_cast<float> (i) / static_cast<float> (numTicks)) * juce::MathConstants<float>::twoPi
                           - juce::MathConstants<float>::halfPi;
        const float s = std::sin (angle);
        const float c = std::cos (angle);
        g.drawLine (cx + innerR * c, cy + innerR * s, cx + outerR * c, cy + outerR * s, 1.2f);
    }
}

void ClipCell::drawProgressSweep (juce::Graphics& g, float cx, float cy, float radius) const
{
    const float arcR = radius * 0.82f;
    const float startAngle = -juce::MathConstants<float>::halfPi;
    const float endAngle = startAngle + static_cast<float> (progressFraction_) * juce::MathConstants<float>::twoPi;

    juce::Path arc;
    arc.addCentredArc (cx, cy, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colours::white.withAlpha (0.70f));
    g.strokePath (arc, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float headX = cx + arcR * std::cos (endAngle);
    const float headY = cy + arcR * std::sin (endAngle);
    const float headR = juce::jmax (2.0f, radius * 0.10f);
    g.setColour (juce::Colours::white.withAlpha (0.95f));
    g.fillEllipse (juce::Rectangle<float> (headR * 2.0f, headR * 2.0f).withCentre ({ headX, headY }));
}

void ClipCell::drawClipTypeIcon (juce::Graphics& g, float cx, float cy, float size) const
{
    // Every clip is a MidiClip for now (Phase 6 adds audio clips) - a
    // simple staggered note-bar glyph rather than per-type icon dispatch.
    g.setColour (juce::Colours::white.withAlpha (0.70f));
    constexpr float widths[4] = { 0.6f, 0.9f, 0.45f, 0.75f };
    const float barH = size * 0.10f;
    const float gap = size * 0.06f;
    float y = cy - (barH * 4.0f + gap * 3.0f) * 0.5f;
    for (const float wFrac : widths)
    {
        g.fillRoundedRectangle (cx - size * 0.5f, y, size * wFrac, barH, barH * 0.5f);
        y += barH + gap;
    }
}

void ClipCell::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
        return; // handled on mouseUp (delete)

    pressed_ = true;
    repaint();

    if (! state_.hasClip)
        return;

    if (e.mods.isShiftDown() && cbs_.onTriggerImmediate)
        cbs_.onTriggerImmediate (track_, row_);
    else if (cbs_.onTrigger)
        cbs_.onTrigger (track_, row_);
}

void ClipCell::mouseUp (const juce::MouseEvent& e)
{
    pressed_ = false;
    repaint();

    if (e.mods.isRightButtonDown())
    {
        if (state_.hasClip && cbs_.onDelete)
            cbs_.onDelete (track_, row_);
        return;
    }

    if (state_.hasClip && cbs_.onRelease)
        cbs_.onRelease (track_, row_);
}

void ClipCell::mouseDoubleClick (const juce::MouseEvent&)
{
    if (! state_.hasClip && cbs_.onCreatePlaceholder)
        cbs_.onCreatePlaceholder (track_, row_);
}

void ClipCell::mouseEnter (const juce::MouseEvent&) { hovered_ = true; repaint(); }
void ClipCell::mouseExit (const juce::MouseEvent&) { hovered_ = false; repaint(); }

} // namespace reaperlooper
