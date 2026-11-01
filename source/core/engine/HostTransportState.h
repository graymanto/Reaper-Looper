#pragma once

namespace reaperlooper
{

/**
 * A snapshot of the host's transport/tempo-map, as seen "at" the current
 * processed block. Supplied by the caller (the REAPER adapter in
 * production - see Phase 2 of the plan; tests supply it directly) so that
 * the core engine never needs to know anything about REAPER, JUCE's
 * AudioPlayHead, or any other host-specific type.
 *
 * `barStartQuarterNotes`, `barLengthQuarterNotes` and `barIndex` are
 * expressed explicitly rather than re-derived from `bpm` so that
 * quantization stays correct across tempo and time-signature changes
 * without the core engine re-implementing tempo-map math (REAPER's own
 * TimeMap2_timeToQN/TimeMap_QNToTime family does this correctly; the
 * adapter calls those and hands over the result). This avoids `sequenced`'s
 * MidiClock bugs (constant-tempo-per-block assumption, time-signature
 * denominator ignored when computing a bar's length).
 */
struct HostTransportState
{
    double bpm { 120.0 };

    /** Current playhead position, in quarter notes (PPQ) from project start. */
    double positionQuarterNotes { 0.0 };

    bool isPlaying { false };
    bool isRecording { false };

    /** QN position at which the bar containing `positionQuarterNotes` began. */
    double barStartQuarterNotes { 0.0 };

    /** Length of that bar, in quarter notes (= numerator * 4/denominator). Always > 0. */
    double barLengthQuarterNotes { 4.0 };

    /** Absolute bar number since project start (bar 1 == index 0). Used for N-bar quantize alignment. */
    int barIndex { 0 };
};

} // namespace reaperlooper
