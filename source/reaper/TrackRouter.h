#pragma once

#include "ReaperFunctions.h"

namespace reaperlooper::TrackRouter
{

/**
 * Ensures a MIDI-only send exists from `sourceTrack` to `destTrack`,
 * filtered to `midiChannel` (1-16) - the "route a column onto a track"
 * gesture from the plan, implemented as one real REAPER send rather than
 * an internal routing table.
 *
 * Reuses an existing send between the two tracks if one is already there
 * (updating its channel filter) instead of creating a duplicate each time
 * the user re-points a column at the same track. The send's audio portion
 * is explicitly disabled (I_SRCCHAN = -1): this plugin produces no audio,
 * only MIDI, and a freshly created send otherwise defaults to carrying
 * audio too.
 *
 * Wrapped in a REAPER undo block. Returns the send index, or -1 on
 * failure / if `f` isn't connected / either track is null.
 */
int routeColumnToTrack (const ReaperFunctions& f, ReaProject* proj,
                        MediaTrack* sourceTrack, MediaTrack* destTrack, int midiChannel);

/** Removes the send created by routeColumnToTrack, if one exists. Wrapped in a REAPER undo block. */
void unroute (const ReaperFunctions& f, ReaProject* proj, MediaTrack* sourceTrack, MediaTrack* destTrack);

/** Index of an existing send from `sourceTrack` to `destTrack`, or -1 if none exists. Exposed for tests. */
int findExistingSend (const ReaperFunctions& f, MediaTrack* sourceTrack, MediaTrack* destTrack);

} // namespace reaperlooper::TrackRouter
