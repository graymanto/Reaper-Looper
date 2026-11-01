#pragma once

#include "ReaperFunctions.h"
#include "../core/engine/HostTransportState.h"

namespace reaperlooper::TransportBridge
{

/**
 * Builds a HostTransportState from REAPER's live transport and tempo map -
 * the thing the plan's Phase 1 engine was deliberately designed to take as
 * a plain value rather than reading REAPER (or any host) itself.
 *
 * Reads, in order:
 *  - GetPlayPosition2Ex: "position of next audio block being processed" -
 *    the audio-thread-relevant position, not the latency-compensated
 *    display position (GetPlayPositionEx) a UI would want instead.
 *  - GetPlayStateEx: &1 playing, &4 recording.
 *  - TimeMap2_timeToBeats: gives both the absolute bar index (`measures`)
 *    and the continuously-accumulating beat count (`fullbeats`) - NOT the
 *    same thing as TimeMap2_timeToQN, which can reset at certain tempo-map
 *    configurations. fullbeats is what HostTransportState::positionQuarterNotes
 *    needs.
 *  - TimeMap_GetMeasureInfo: the *actual* QN start/end of that specific
 *    bar, correct across tempo and time-signature changes - deliberately
 *    not reconstructed from a numerator/denominator formula (REAPER already
 *    did the work; re-deriving it would risk exactly the kind of
 *    denominator bug the plan flagged in `sequenced`'s MidiClock).
 *
 * `proj` should be ReaperApi::getHostProject() - the project this specific
 * plugin instance is loaded into, not necessarily whichever REAPER project
 * tab happens to be focused.
 *
 * Returns a stopped, 120bpm, bar-0 default if any required function
 * pointer is null (i.e. not connected to REAPER) - safe to call
 * unconditionally.
 */
HostTransportState read (const ReaperFunctions& f, ReaProject* proj);

/**
 * Drives REAPER's transport directly. Note: CSurf_On* has no project
 * parameter - it always affects whichever project tab is currently
 * focused in the REAPER UI, not necessarily `proj` above. This is a REAPER
 * platform limitation shared by every control surface and transport-driving
 * plugin, not something this adapter can route around.
 */
void play (const ReaperFunctions& f);
void stop (const ReaperFunctions& f);
void toggleRecord (const ReaperFunctions& f);

} // namespace reaperlooper::TransportBridge
