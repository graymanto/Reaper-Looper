#include "MidiClip.h"
#include <algorithm>

namespace reaperlooper
{

void MidiClip::setLengthBeats (double beats) noexcept
{
    jassert (beats > 0.0);
    lengthBeats_ = juce::jmax (0.0, beats);
}

void MidiClip::addNote (const MidiNote& note)
{
    notes_.push_back (note);
    sortNotes();
}

void MidiClip::removeNoteAt (int index)
{
    if (index >= 0 && index < static_cast<int> (notes_.size()))
        notes_.erase (notes_.begin() + index);
}

void MidiClip::clearNotes()
{
    notes_.clear();
}

void MidiClip::sortNotes()
{
    std::sort (notes_.begin(), notes_.end(),
               [] (const MidiNote& a, const MidiNote& b) { return a.startBeat < b.startBeat; });
}

void MidiClip::collectMidi (juce::MidiBuffer& output, const ClipBlockContext& ctx) const
{
    jassert (ctx.localStartBeat >= 0.0 && ctx.localEndBeat <= lengthBeats_ + 1.0e-9);
    jassert (ctx.localEndBeat > ctx.localStartBeat);

    const double invBeatsPerSample = ctx.beatsPerSample > 0.0 ? (1.0 / ctx.beatsPerSample) : 0.0;

    const auto sampleFor = [&] (double localBeat) -> int
    {
        const int s = static_cast<int> (std::round ((localBeat - ctx.localStartBeat) * invBeatsPerSample));
        return ctx.sampleOffset + juce::jlimit (0, juce::jmax (0, ctx.numSamples - 1), s);
    };

    for (const auto& note : notes_)
    {
        if (note.startBeat >= ctx.localStartBeat && note.startBeat < ctx.localEndBeat)
        {
            output.addEvent (juce::MidiMessage::noteOn (note.channel, note.pitch,
                                                          static_cast<juce::uint8> (note.velocity)),
                              sampleFor (note.startBeat));
        }

        const double endBeat = note.endBeat();
        if (endBeat > ctx.localStartBeat && endBeat <= ctx.localEndBeat)
        {
            output.addEvent (juce::MidiMessage::noteOff (note.channel, note.pitch),
                              sampleFor (endBeat));
        }
    }
}

juce::ValueTree MidiClip::toValueTree() const
{
    juce::ValueTree tree = Clip::toValueTree();
    tree.setProperty ("lengthBeats", lengthBeats_, nullptr);

    for (const auto& n : notes_)
    {
        juce::ValueTree noteTree ("Note");
        noteTree.setProperty ("pitch", n.pitch, nullptr);
        noteTree.setProperty ("velocity", n.velocity, nullptr);
        noteTree.setProperty ("channel", n.channel, nullptr);
        noteTree.setProperty ("start", n.startBeat, nullptr);
        noteTree.setProperty ("length", n.lengthBeats, nullptr);
        tree.appendChild (noteTree, nullptr);
    }

    return tree;
}

void MidiClip::fromValueTree (const juce::ValueTree& tree)
{
    Clip::fromValueTree (tree);
    lengthBeats_ = static_cast<double> (tree.getProperty ("lengthBeats", lengthBeats_));

    notes_.clear();
    for (const auto& noteTree : tree)
    {
        if (! noteTree.hasType ("Note"))
            continue;

        MidiNote n;
        n.pitch = static_cast<int> (noteTree.getProperty ("pitch", 60));
        n.velocity = static_cast<int> (noteTree.getProperty ("velocity", 100));
        n.channel = static_cast<int> (noteTree.getProperty ("channel", 1));
        n.startBeat = static_cast<double> (noteTree.getProperty ("start", 0.0));
        n.lengthBeats = static_cast<double> (noteTree.getProperty ("length", 1.0));
        notes_.push_back (n);
    }
    sortNotes();
}

} // namespace reaperlooper
