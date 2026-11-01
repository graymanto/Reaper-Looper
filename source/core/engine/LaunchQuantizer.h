#pragma once

#include "HostTransportState.h"
#include "../model/ClipTypes.h"

namespace reaperlooper::LaunchQuantizer
{

/**
 * The next quantize boundary at or after `transport.positionQuarterNotes`,
 * in absolute project quarter notes.
 *
 * - None resolves immediately (returns the current position).
 * - Quarter/Half are note-value quantization (always exactly 1/2 quarter
 *   notes respectively, regardless of the prevailing time signature - a
 *   "quarter note" is an absolute duration, not "a quarter of a bar").
 * - OneBar/TwoBars/.../EightBars align to the host's actual bar grid via
 *   `barIndex`, so e.g. "4 bars" always lands on a bar index that's a
 *   multiple of 4 counted from project start, matching Ableton's behaviour,
 *   not just "4 bars from wherever you happened to press".
 */
double nextBoundary (const HostTransportState& transport, LaunchQuantize quantize);

/** How many bars a given quantize value spans (1 for None/Quarter/Half - they don't align to bars). */
int barsForQuantize (LaunchQuantize quantize) noexcept;

} // namespace reaperlooper::LaunchQuantizer
