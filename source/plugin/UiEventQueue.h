#pragma once

#include <juce_core/juce_core.h>
#include "../core/model/ClipTypes.h"
#include <vector>

namespace reaperlooper
{

/**
 * A thread-safe queue of grid interactions originating from the mouse
 * (message thread), drained once per block on the audio thread and
 * translated into the exact same Session/Track calls the hardware path
 * (LaunchpadController) uses - mouse clicks and APC pad presses are fully
 * equivalent inputs to the engine, by design.
 *
 * This queue carries more than pad presses: Session::addTrack()/addScene()
 * and Track::setClip()/clearClip() all mutate containers (`tracks_`,
 * `cells_`) that Session::processBlock() / Track::processBlock() read and
 * write on the audio thread with no internal locking (by design - see
 * their own doc comments). Calling them directly from a UI click handler
 * would race with the audio thread. Routing every grid mutation through
 * here and applying it from inside processBlock() (same thread, naturally
 * serialized with everything else touching Session) avoids that
 * entirely - there is deliberately no "safe to call live" grid-mutation
 * API at all.
 *
 * Deliberately a simple CriticalSection-protected vector rather than a
 * lock-free structure: grid clicks happen at human speed (a handful of
 * events per second at most), so contention is a non-issue and a lock is
 * far simpler to get right than a lock-free queue would be here.
 */
class UiEventQueue
{
public:
    enum class Type
    {
        PadDown,
        PadUp,
        PadDownImmediate, ///< Shift+click: bypass quantize (Track::handlePadDownImmediate).
        SceneLaunch,
        TrackStop,
        ClearClip,         ///< Delete whatever's in (track, row).
        CreatePlaceholder, ///< Create a short placeholder clip at (track, row) - see Phase 5.
        AddTrack,
        AddScene,
        PageTracks,       ///< Pan the shared GridViewport by `track` columns (field reused to carry the delta).
        PageRows,         ///< Pan the shared GridViewport by `row` rows (field reused to carry the delta).
        SetGlobalQuantize ///< `track` field reused to carry static_cast<int>(LaunchQuantize).
    };

    struct Event
    {
        Type type;
        int track { -1 }; ///< Logical track index, or (PageTracks) the pan delta. Unused for SceneLaunch/AddTrack/AddScene/PageRows.
        int row { -1 };   ///< Logical row index, or (PageRows) the pan delta. Unused for TrackStop/AddTrack/AddScene/PageTracks.
    };

    //==============================================================================
    // Message thread

    void pushPadDown (int track, int row) { push ({ Type::PadDown, track, row }); }
    void pushPadUp (int track, int row) { push ({ Type::PadUp, track, row }); }
    void pushPadDownImmediate (int track, int row) { push ({ Type::PadDownImmediate, track, row }); }
    void pushSceneLaunch (int row) { push ({ Type::SceneLaunch, -1, row }); }
    void pushTrackStop (int track) { push ({ Type::TrackStop, track, -1 }); }
    void pushClearClip (int track, int row) { push ({ Type::ClearClip, track, row }); }
    void pushCreatePlaceholder (int track, int row) { push ({ Type::CreatePlaceholder, track, row }); }
    void pushAddTrack() { push ({ Type::AddTrack, -1, -1 }); }
    void pushAddScene() { push ({ Type::AddScene, -1, -1 }); }
    void pushPageTracks (int delta) { push ({ Type::PageTracks, delta, -1 }); }
    void pushPageRows (int delta) { push ({ Type::PageRows, -1, delta }); }
    void pushSetGlobalQuantize (LaunchQuantize q) { push ({ Type::SetGlobalQuantize, static_cast<int> (q), -1 }); }

    //==============================================================================
    // Audio thread

    /** Moves out every pending event (clearing the queue) for the caller to process, in push order. */
    std::vector<Event> drain()
    {
        const juce::ScopedLock sl (lock_);
        return std::move (pending_);
    }

private:
    void push (Event e)
    {
        const juce::ScopedLock sl (lock_);
        pending_.push_back (e);
    }

    juce::CriticalSection lock_;
    std::vector<Event> pending_;
};

} // namespace reaperlooper
