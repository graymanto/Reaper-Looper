#include "RecordBuffer.h"
#include <algorithm>

namespace reaperlooper
{

RecordBuffer::RecordBuffer (double maxHistoryBeats)
    : maxHistoryBeats_ (maxHistoryBeats)
{
    completedNotes_.reserve (2048);
}

void RecordBuffer::feedBlock (const juce::MidiBuffer& input, double blockStartBeat, double beatsPerSample)
{
    for (const auto metadata : input)
    {
        const auto msg = metadata.getMessage();
        const double absoluteBeat = blockStartBeat + static_cast<double> (metadata.samplePosition) * beatsPerSample;

        if (msg.isNoteOn())
        {
            auto& h = held_[static_cast<size_t> (heldIndex (msg.getChannel(), msg.getNoteNumber()))];
            // A retrigger without an intervening note-off drops the earlier
            // hanging note rather than closing it off - rare in practice and
            // an acceptable simplification for Phase 1 capture.
            h.startBeat = absoluteBeat;
            h.velocity = msg.getVelocity();
        }
        else if (msg.isNoteOff())
        {
            auto& h = held_[static_cast<size_t> (heldIndex (msg.getChannel(), msg.getNoteNumber()))];
            if (h.startBeat >= 0.0)
            {
                MidiNote note;
                note.pitch = msg.getNoteNumber();
                note.channel = msg.getChannel();
                note.velocity = h.velocity;
                note.startBeat = h.startBeat;
                note.lengthBeats = juce::jmax (1.0e-6, absoluteBeat - h.startBeat);
                completedNotes_.push_back (note);
                h.startBeat = -1.0;
            }
        }
    }

    pruneOlderThan (blockStartBeat);
}

void RecordBuffer::pruneOlderThan (double currentBeat)
{
    const double cutoff = currentBeat - maxHistoryBeats_;
    completedNotes_.erase (
        std::remove_if (completedNotes_.begin(), completedNotes_.end(),
                         [cutoff] (const MidiNote& n) { return n.endBeat() < cutoff; }),
        completedNotes_.end());
}

std::unique_ptr<MidiClip> RecordBuffer::captureLastBars (double currentBeat, int numBars, double beatsPerBar) const
{
    auto clip = std::make_unique<MidiClip>();
    const double windowLength = static_cast<double> (juce::jmax (1, numBars)) * beatsPerBar;
    const double windowStart = currentBeat - windowLength;

    clip->setLengthBeats (windowLength);

    for (const auto& n : completedNotes_)
    {
        if (n.endBeat() <= windowStart || n.startBeat >= currentBeat)
            continue;

        MidiNote shifted = n;
        shifted.startBeat = juce::jmax (0.0, n.startBeat - windowStart);
        const double clippedEnd = juce::jmin (n.endBeat(), currentBeat) - windowStart;
        shifted.lengthBeats = juce::jmax (1.0e-6, clippedEnd - shifted.startBeat);
        clip->addNote (shifted);
    }

    // Notes still sounding right now: truncate at the capture instant.
    for (int i = 0; i < static_cast<int> (held_.size()); ++i)
    {
        const auto& h = held_[static_cast<size_t> (i)];
        if (h.startBeat < 0.0 || h.startBeat >= currentBeat || h.startBeat < windowStart)
            continue;

        MidiNote note;
        note.channel = (i / 128) + 1;
        note.pitch = i % 128;
        note.velocity = h.velocity;
        note.startBeat = juce::jmax (0.0, h.startBeat - windowStart);
        note.lengthBeats = juce::jmax (1.0e-6, currentBeat - windowStart - note.startBeat);
        clip->addNote (note);
    }

    return clip;
}

} // namespace reaperlooper
