#pragma once

#include "../model/MidiClip.h"
#include <array>
#include <memory>
#include <vector>

namespace reaperlooper
{

/**
 * A bounded rolling history of a track's live MIDI input, kept continuously
 * (regardless of record-arm state) so that Capture - "turn what was just
 * played into a clip, retroactively" - has something to pull from.
 *
 * Fed every block via feedBlock(), called from Track::processBlock on the
 * audio thread. Held notes (no note-off yet) are tracked in a fixed-size
 * array to avoid allocation in the hot path; completed notes are kept in a
 * plain growable buffer and pruned to `maxHistoryBeats` periodically
 * (an O(n) erase-from-front, acceptable at the low frequency pruning
 * actually happens at - revisit with a true ring buffer if profiling says
 * otherwise).
 *
 * Capture does not auto-detect a loop length (unlike Ableton's heuristic);
 * the caller supplies the number of bars explicitly. This keeps the Phase 1
 * implementation bounded in scope while still being genuinely usable.
 */
class RecordBuffer
{
public:
    explicit RecordBuffer (double maxHistoryBeats = 64.0);

    void setMaxHistoryBeats (double beats) noexcept { maxHistoryBeats_ = beats; }

    /** Feeds live input for one block. `input` events are sample-offset within the block; `blockStartBeat` is the absolute project beat at sample 0. */
    void feedBlock (const juce::MidiBuffer& input, double blockStartBeat, double beatsPerSample);

    /** Discards history older than `currentBeat - maxHistoryBeats_`. Called automatically by feedBlock, but exposed for tests. */
    void pruneOlderThan (double currentBeat);

    /**
     * Builds a new clip from the last `numBars` bars (each `beatsPerBar` long)
     * of captured history, ending at `currentBeat`. Notes are shifted so the
     * window start becomes clip-local beat 0. Any note still held at
     * `currentBeat` is truncated there (not extended past capture time).
     * Returns an empty (silent) clip if there is no history in range yet -
     * capture always succeeds, it just may capture nothing to play.
     */
    std::unique_ptr<MidiClip> captureLastBars (double currentBeat, int numBars, double beatsPerBar) const;

    /** For tests: completed notes currently retained (absolute beat positions, not window-relative). */
    const std::vector<MidiNote>& getCompletedNotesForTest() const noexcept { return completedNotes_; }

private:
    struct HeldNote
    {
        double startBeat { -1.0 }; ///< -1 = not currently held.
        int velocity { 0 };
    };

    static int heldIndex (int channel, int pitch) noexcept { return (channel - 1) * 128 + pitch; }

    double maxHistoryBeats_;
    std::array<HeldNote, 16 * 128> held_;
    std::vector<MidiNote> completedNotes_; ///< Absolute beat positions, sorted by startBeat.
};

} // namespace reaperlooper
