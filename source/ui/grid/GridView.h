#pragma once

#include <JuceHeader.h>
#include "ClipCell.h"
#include "../../core/session/Session.h"
#include "../../core/controller/GridViewport.h"
#include "../../reaper/TrackRegistry.h"
#include <functional>
#include <memory>
#include <vector>

namespace reaperlooper
{

/**
 * The clip grid: track headers (name/colour, a REAPER-track routing
 * picker, and a stop button), a scene launch rail, the viewport-sized
 * grid of ClipCells, and simple paging/add controls for growing past the
 * viewport.
 *
 * GridView never touches Session/Track directly outside refresh() (which
 * only *reads* it) - every interaction is reported back through Callbacks
 * for the owner (PluginEditor) to route into UiEventQueue. This keeps the
 * same decoupling `sequenced`'s GridView has, and - more importantly here -
 * keeps every Session-mutating action off the message thread (see
 * UiEventQueue's doc comment for why that matters).
 *
 * Visually ported from `sequenced`'s GridView: the glass-gradient headers,
 * the coloured scene-rail edge, the stop-button pill. Simplified for this
 * phase: no collapsible per-track volume/pan row, no long-press rename
 * menu (plain click-to-cycle colour instead), fixed pixel header
 * sizing rather than fully responsive proportional sizing.
 */
class GridView final : public juce::Component
{
public:
    struct Callbacks
    {
        std::function<void (int track, int row)> onCellTrigger;
        std::function<void (int track, int row)> onCellTriggerImmediate;
        std::function<void (int track, int row)> onCellRelease;
        std::function<void (int track, int row)> onCellCreatePlaceholder;
        std::function<void (int track, int row)> onCellDelete;
        std::function<void (int row)> onSceneLaunch;
        std::function<void (int track)> onTrackStop;
        std::function<void()> onAddTrack;
        std::function<void()> onAddScene;
        /** Either MediaTrack* may be nullptr ("no routing"). Both old and new are reported since removing a routing needs to know what it's removing. */
        std::function<void (int track, MediaTrack* oldDest, MediaTrack* newDest)> onRouteColumn;
        /**
         * Requests panning the viewport by `delta` columns/rows. GridView
         * does not own the viewport (see the class doc comment) - the
         * owner applies this to its single shared GridViewport (clamped
         * via GridViewport::clampTo) so the UI and any hardware controller
         * stay pointed at the same logical window.
         */
        std::function<void (int delta)> onPageTracksBy;
        std::function<void (int delta)> onPageRowsBy;
    };

    explicit GridView (Callbacks cbs);

    /**
     * Pulls fresh display state from `session` (through `viewport`) and
     * the current REAPER track list, and repaints. Called periodically by
     * the editor's timer - message thread, read-only.
     */
    void refresh (const Session& session, const GridViewport& viewport, double beatsPerBar,
                  const std::vector<TrackRegistry::TrackInfo>& reaperTracks, MediaTrack* hostTrack,
                  double positionQuarterNotes);

    void resized() override;
    void paint (juce::Graphics&) override;

private:
    juce::Colour colourForColumn (const Session& session, int logicalTrack) const;

    Callbacks cbs_;
    GridViewport viewport_; ///< A mirror of whatever was last passed to refresh() - GridView does not own this (see the class doc comment).
    int lastTotalTracks_ { 0 }; ///< From the most recent refresh() - enables/disables the paging buttons.
    int lastTotalRows_ { 0 };
    std::vector<TrackRegistry::TrackInfo> lastReaperTracks_; ///< So a routing combo's onChange (fired later, by the user) can map its selected id back to a MediaTrack*.

    std::vector<std::vector<std::unique_ptr<ClipCell>>> cells_; // [col][row], sized to viewport_.width x viewport_.height

    struct ColumnHeader
    {
        std::unique_ptr<juce::Label> nameLabel;
        std::unique_ptr<juce::ComboBox> routingCombo;
        std::unique_ptr<juce::TextButton> stopButton;
        MediaTrack* currentDest { nullptr };
    };
    std::vector<ColumnHeader> headers_; // one per physical column

    std::vector<std::unique_ptr<juce::TextButton>> sceneButtons_;

    juce::TextButton addTrackButton_ { "+ Track" };
    juce::TextButton addSceneButton_ { "+ Scene" };
    juce::TextButton pageTracksLeft_ { "<" };
    juce::TextButton pageTracksRight_ { ">" };
    juce::TextButton pageRowsUp_ { "^" };
    juce::TextButton pageRowsDown_ { "v" };
    juce::Label trackRangeLabel_;
    juce::Label rowRangeLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GridView)
};

} // namespace reaperlooper
