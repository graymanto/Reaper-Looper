#include <catch2/catch_all.hpp>
#include "core/model/MidiClip.h"

using namespace reaperlooper;

namespace
{
    ClipBlockContext makeCtx (double localStart, double localEnd, int sampleOffset, int numSamples, double beatsPerSample)
    {
        ClipBlockContext ctx;
        ctx.localStartBeat = localStart;
        ctx.localEndBeat = localEnd;
        ctx.sampleOffset = sampleOffset;
        ctx.numSamples = numSamples;
        ctx.beatsPerSample = beatsPerSample;
        return ctx;
    }
}

TEST_CASE ("MidiClip emits note-on and note-off within its segment", "[midiclip]")
{
    MidiClip clip;
    clip.setLengthBeats (4.0);
    clip.addNote ({ 60, 100, 1, 0.0, 1.0 }); // beat 0 to 1

    juce::MidiBuffer buffer;
    // beatsPerSample = 0.001 -> 1000 samples per beat; segment covers [0, 2) beats.
    clip.collectMidi (buffer, makeCtx (0.0, 2.0, 0, 2000, 0.001));

    int numOn = 0, numOff = 0;
    int onSample = -1, offSample = -1;
    for (const auto metadata : buffer)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn()) { ++numOn; onSample = metadata.samplePosition; }
        if (msg.isNoteOff()) { ++numOff; offSample = metadata.samplePosition; }
    }

    REQUIRE (numOn == 1);
    REQUIRE (numOff == 1);
    REQUIRE (onSample == 0);
    REQUIRE (offSample == 1000);
}

TEST_CASE ("MidiClip ignores notes outside the given segment", "[midiclip]")
{
    MidiClip clip;
    clip.setLengthBeats (4.0);
    clip.addNote ({ 60, 100, 1, 2.0, 1.0 }); // beat 2 to 3

    juce::MidiBuffer buffer;
    // Segment only covers [0, 1) - the note starts after this segment ends.
    clip.collectMidi (buffer, makeCtx (0.0, 1.0, 0, 1000, 0.001));

    REQUIRE (buffer.getNumEvents() == 0);
}

TEST_CASE ("MidiClip note-off fires for a note that started in a previous segment", "[midiclip]")
{
    MidiClip clip;
    clip.setLengthBeats (4.0);
    clip.addNote ({ 60, 100, 1, 0.0, 1.5 }); // ends at beat 1.5

    juce::MidiBuffer buffer;
    // Second-segment call covering [1.0, 2.0) - note-on already happened
    // earlier; only the note-off should appear here.
    clip.collectMidi (buffer, makeCtx (1.0, 2.0, 0, 1000, 0.001));

    REQUIRE (buffer.getNumEvents() == 1);
    for (const auto metadata : buffer)
        REQUIRE (metadata.getMessage().isNoteOff());
}

TEST_CASE ("MidiClip serialization round-trips notes and metadata", "[midiclip]")
{
    MidiClip clip;
    clip.setLengthBeats (8.0);
    clip.setLooping (false);
    clip.setLaunchMode (LaunchMode::Gate);
    clip.setName ("Test Clip");
    clip.addNote ({ 64, 90, 2, 0.5, 0.25 });

    auto tree = clip.toValueTree();

    MidiClip restored;
    restored.fromValueTree (tree);

    REQUIRE (restored.getLengthBeats() == Catch::Approx (8.0));
    REQUIRE (restored.isLooping() == false);
    REQUIRE (restored.getLaunchMode() == LaunchMode::Gate);
    REQUIRE (restored.getName() == "Test Clip");
    REQUIRE (restored.getNotes().size() == 1);
    REQUIRE (restored.getNotes()[0] == clip.getNotes()[0]);
}
