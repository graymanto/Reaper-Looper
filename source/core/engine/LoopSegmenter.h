#pragma once

#include "../model/Clip.h"
#include <vector>

namespace reaperlooper::LoopSegmenter
{

/**
 * Splits one audio block into one or more clip-local, non-wrapping segments
 * suitable for Clip::collectMidi(), handling (possibly multiple) loop-
 * boundary crossings within the block.
 *
 * `loopAnchorBeat` is the absolute project beat at which this clip's local
 * beat 0 last occurred (i.e. when it was launched, or - under legato - when
 * the clip it replaced was launched; see Track). Each clip's loop phase is
 * entirely independent of any other clip's: there is no session-wide master
 * loop (see the plan for why `sequenced`'s master-loop model doesn't fit a
 * live looper).
 *
 * When `looping` is false, at most one segment is returned, truncated at
 * `clipLengthBeats`; once the clip has played past its own length, an empty
 * vector is returned (the caller should then stop/deactivate the slot).
 */
std::vector<reaperlooper::ClipBlockContext> splitIntoLoopSegments (double absoluteBlockStartBeat,
                                                                    int blockNumSamples,
                                                                    double beatsPerSample,
                                                                    double loopAnchorBeat,
                                                                    double clipLengthBeats,
                                                                    bool looping);

} // namespace reaperlooper::LoopSegmenter
