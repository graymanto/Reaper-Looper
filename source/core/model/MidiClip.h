#pragma once

#include "Clip.h"
#include "MidiNote.h"
#include <vector>

namespace reaperlooper
{

/**
 * A clip holding recorded/drawn MIDI notes.
 *
 * Notes are kept sorted by startBeat. collectMidi() fires a note-on when a
 * note's startBeat falls within [localStartBeat, localEndBeat), and a
 * note-off when its endBeat falls within (localStartBeat, localEndBeat] -
 * i.e. only within the single, already-non-wrapping segment it is given
 * (see ClipBlockContext). Track is responsible for segmenting across loop
 * boundaries and for emitting an all-notes-off when a clip stops or is
 * replaced, so MidiClip itself never needs to track "notes currently
 * sounding" across calls.
 *
 * Notes are not required to fit within the clip length - a note that runs
 * past getLengthBeats() simply never receives its note-off from this clip
 * (the caller's all-notes-off on stop covers it). This matches how most
 * grid-based loopers treat an overhanging note drawn right at a clip's end.
 */
class MidiClip final : public Clip
{
public:
    MidiClip() = default;

    //==============================================================================
    double getLengthBeats() const noexcept override { return lengthBeats_; }
    void setLengthBeats (double beats) noexcept;

    juce::String getTypeId() const override { return "Midi"; }

    //==============================================================================
    void addNote (const MidiNote& note);
    void removeNoteAt (int index);
    void clearNotes();
    const std::vector<MidiNote>& getNotes() const noexcept { return notes_; }

    //==============================================================================
    void collectMidi (juce::MidiBuffer& output, const ClipBlockContext& ctx) const override;

    //==============================================================================
    juce::ValueTree toValueTree() const override;
    void fromValueTree (const juce::ValueTree& tree) override;

private:
    void sortNotes();

    std::vector<MidiNote> notes_;
    double lengthBeats_ { 4.0 * 4.0 }; ///< Default: 4 bars at 4/4.
};

} // namespace reaperlooper
