#pragma once

#include <JuceHeader.h>
#include "../Theme.h"
#include <functional>

namespace reaperlooper
{

/**
 * A single grid cell. Visually ported from `sequenced`'s ClipCell - the
 * circular badge, double-ring glow, and track-colour tinting are its
 * actual visual identity and are unchanged - but with a much smaller
 * interaction surface, scoped to what this phase actually needs:
 *
 *  - no drag-and-drop, variations, library, or context menus (those are
 *    tied to features - a clip library, processor racks, copy/paste -
 *    this project hasn't built; Phase 5 is where real cell editing lands)
 *  - no density-weighted radial spikes (Clip has no note-density query
 *    yet) - a plain evenly-spaced tick ring instead
 *  - double-click an empty cell creates a short placeholder clip (there
 *    is no editor yet - see Phase 5); right-click an occupied one deletes
 *    it. Both are deliberately blunt placeholders, not final UX.
 *
 * ClipCell never touches Session/Track directly (matching `sequenced`'s
 * design) - GridView pushes a plain State snapshot in via setState() each
 * refresh, and reports interactions back through Callbacks.
 */
class ClipCell final : public juce::Component
{
public:
    struct Callbacks
    {
        std::function<void (int track, int row)> onTrigger;           ///< Left-click a clip.
        std::function<void (int track, int row)> onTriggerImmediate;  ///< Shift+click: bypass quantize.
        std::function<void (int track, int row)> onRelease;           ///< Mouse-up over a clip (Gate mode release).
        std::function<void (int track, int row)> onCreatePlaceholder; ///< Double-click an empty cell.
        std::function<void (int track, int row)> onDelete;            ///< Right-click an occupied cell.
    };

    struct State
    {
        bool hasClip { false };
        bool isActive { false };
        bool isQueued { false };
        bool isStopQueued { false };
        double lengthBeats { 0.0 };
    };

    ClipCell (int logicalTrack, int logicalRow, Callbacks cbs);

    void setTrackIndex (int t) noexcept { track_ = t; }
    void setRowIndex (int r) noexcept { row_ = r; }

    /** `progressFraction` is 0-1, only meaningful when state.isActive. */
    void setState (const State& s, juce::Colour trackColour, double beatsPerBar, double progressFraction);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    void drawTickRing (juce::Graphics&, float cx, float cy, float radius) const;
    void drawProgressSweep (juce::Graphics&, float cx, float cy, float radius) const;
    void drawClipTypeIcon (juce::Graphics&, float cx, float cy, float size) const;

    int track_;
    int row_;
    Callbacks cbs_;
    State state_;
    juce::Colour trackColour_ { Theme::accent };
    double beatsPerBar_ { 4.0 };
    double progressFraction_ { 0.0 };
    bool hovered_ { false };
    bool pressed_ { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipCell)
};

} // namespace reaperlooper
