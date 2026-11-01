#pragma once

#include "ReaperFunctions.h"
#include <juce_core/juce_core.h>
#include <vector>

namespace reaperlooper::TrackRegistry
{

/** A track's identity as read from REAPER, for grid chrome (column headers). */
struct TrackInfo
{
    MediaTrack* track { nullptr };
    juce::String name;
    juce::uint32 colourARGB { 0 }; ///< 0 = no custom colour set (REAPER default track colour).
};

/** Every track in `proj`, in track-list order. Empty if `f` isn't connected. */
std::vector<TrackInfo> listTracks (const ReaperFunctions& f, ReaProject* proj);

/** Re-reads name/colour for one already-known track, e.g. after the user renames/recolours it in REAPER. */
TrackInfo describeTrack (const ReaperFunctions& f, MediaTrack* track);

/**
 * Creates a new track at the end of the track list, names it, and wraps
 * the action in a REAPER undo block. Returns nullptr on failure or if `f`
 * isn't connected.
 */
MediaTrack* createTrack (const ReaperFunctions& f, ReaProject* proj, const juce::String& name);

} // namespace reaperlooper::TrackRegistry
