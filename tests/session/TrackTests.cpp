#include <catch2/catch_all.hpp>
#include "core/session/Track.h"
#include "core/model/MidiClip.h"

using namespace reaperlooper;

namespace
{
    // sampleRate=1000, bpm=60 => beatsPerSample = 0.001 => 1000 samples == 1.0 beat.
    // Chosen purely for round numbers in assertions, not musical realism.
    constexpr double kSampleRate = 1000.0;
    constexpr double kBpm = 60.0;
    constexpr int kSamplesPerBeat = 1000;

    HostTransportState transportAt (double positionBeats, int barIndex = 0, double barStart = 0.0, double barLength = 4.0)
    {
        HostTransportState t;
        t.bpm = kBpm;
        t.isPlaying = true;
        t.positionQuarterNotes = positionBeats;
        t.barStartQuarterNotes = barStart;
        t.barLengthQuarterNotes = barLength;
        t.barIndex = barIndex;
        return t;
    }

    std::unique_ptr<MidiClip> makeClip (double lengthBeats, LaunchMode mode = LaunchMode::Toggle)
    {
        auto clip = std::make_unique<MidiClip>();
        clip->setLengthBeats (lengthBeats);
        clip->setLaunchMode (mode);
        return clip;
    }

    /** setClip() takes ownership as a base Clip*, so callers that need to add
        notes afterwards must do so through the MidiClip* before moving it in. */
    MidiClip* setMidiClip (Track& track, int row, std::unique_ptr<MidiClip> clip)
    {
        auto* raw = clip.get();
        track.setClip (row, std::move (clip));
        return raw;
    }

    bool hasNoteOnAt (const juce::MidiBuffer& buffer, int sample, int pitch)
    {
        for (const auto metadata : buffer)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn() && msg.getNoteNumber() == pitch && metadata.samplePosition == sample)
                return true;
        }
        return false;
    }

    bool hasAnyNoteOff (const juce::MidiBuffer& buffer)
    {
        for (const auto metadata : buffer)
            if (metadata.getMessage().isNoteOff() || metadata.getMessage().isAllNotesOff())
                return true;
        return false;
    }
}

TEST_CASE ("Toggle: launching an empty track resolves at the next block when quantize is None", "[track]")
{
    Track t;
    t.prepare (kSampleRate);
    t.setDefaultQuantize (LaunchQuantize::None);
    setMidiClip (t, 0, makeClip (4.0))->addNote ({ 60, 100, 1, 0.0, 1.0 });

    juce::MidiBuffer dummy;
    const auto transport = transportAt (0.0);
    t.handlePadDown (0, transport, dummy);
    REQUIRE (t.getQueuedSlot() == 0);

    juce::MidiBuffer output;
    t.processBlock (output, kSamplesPerBeat, transport);

    REQUIRE (t.getActiveSlot() == 0);
    REQUIRE (hasNoteOnAt (output, 0, 60));
}

TEST_CASE ("Toggle: pressing the active slot queues a stop", "[track]")
{
    Track t;
    t.prepare (kSampleRate);
    t.setDefaultQuantize (LaunchQuantize::None);
    t.setClip (0, makeClip (4.0));

    juce::MidiBuffer dummy;
    auto transport = transportAt (0.0);
    t.handlePadDown (0, transport, dummy);
    t.processBlock (dummy, kSamplesPerBeat, transport);
    REQUIRE (t.getActiveSlot() == 0);

    transport = transportAt (1.0);
    t.handlePadDown (0, transport, dummy);
    REQUIRE (t.isStopQueued());

    juce::MidiBuffer output;
    t.processBlock (output, kSamplesPerBeat, transport);
    REQUIRE (t.getActiveSlot() == -1);
    REQUIRE (hasAnyNoteOff (output));
}

TEST_CASE ("Toggle: re-pressing a still-queued slot cancels the launch", "[track]")
{
    Track t;
    t.prepare (kSampleRate);
    t.setDefaultQuantize (LaunchQuantize::OneBar); // deliberately not-yet-resolved
    t.setClip (0, makeClip (4.0));

    juce::MidiBuffer dummy;
    const auto transport = transportAt (1.0, 0, 0.0, 4.0); // mid-bar, so OneBar won't resolve immediately
    t.handlePadDown (0, transport, dummy);
    REQUIRE (t.getQueuedSlot() == 0);

    t.handlePadDown (0, transport, dummy);
    REQUIRE (t.getQueuedSlot() == -1);
    REQUIRE (t.getActiveSlot() == -1);
}

TEST_CASE ("Trigger mode relaunches from the top even while already active", "[track]")
{
    Track t;
    t.prepare (kSampleRate);
    t.setDefaultQuantize (LaunchQuantize::None);
    setMidiClip (t, 0, makeClip (2.0, LaunchMode::Trigger))->addNote ({ 60, 100, 1, 0.0, 0.05 }); // a short note right at the top of the loop

    juce::MidiBuffer dummy;
    auto transport = transportAt (0.0);
    t.handlePadDown (0, transport, dummy);
    t.processBlock (dummy, kSamplesPerBeat, transport); // active slot now mid-way through its 2-beat loop

    // Advance without retriggering: the top-of-loop note should NOT fire again.
    transport = transportAt (1.0);
    juce::MidiBuffer noRetrigger;
    t.processBlock (noRetrigger, kSamplesPerBeat, transport);
    REQUIRE_FALSE (hasNoteOnAt (noRetrigger, 0, 60));

    // Re-press (Trigger mode): should relaunch from beat 0, firing the note again immediately.
    t.handlePadDown (0, transport, dummy);
    juce::MidiBuffer retriggered;
    t.processBlock (retriggered, kSamplesPerBeat, transport);
    REQUIRE (hasNoteOnAt (retriggered, 0, 60));
}

TEST_CASE ("Gate mode launches and stops immediately, bypassing quantize", "[track]")
{
    Track t;
    t.prepare (kSampleRate);
    t.setDefaultQuantize (LaunchQuantize::FourBars); // would never resolve within this test if it were used
    t.setClip (0, makeClip (4.0, LaunchMode::Gate));

    juce::MidiBuffer down;
    const auto transport = transportAt (1.3);
    t.handlePadDown (0, transport, down);
    REQUIRE (t.getActiveSlot() == 0); // resolved inline, no processBlock needed

    juce::MidiBuffer up;
    t.handlePadUp (0, transport, up);
    REQUIRE (t.getActiveSlot() == -1);
    REQUIRE (hasAnyNoteOff (up));
}

TEST_CASE ("Legato carries the old clip's loop phase into the new one", "[track]")
{
    Track t;
    t.prepare (kSampleRate);
    t.setDefaultQuantize (LaunchQuantize::None);
    t.setLegatoMode (true);

    t.setClip (0, makeClip (2.0)); // 2-beat loop
    // 3-beat loop - deliberately longer than the carried-over phase (1 beat),
    // so the carry shows up as an exact, unwrapped local position.
    setMidiClip (t, 1, makeClip (3.0))->addNote ({ 61, 100, 1, 1.0, 0.05 });

    juce::MidiBuffer dummy;
    auto transport = transportAt (0.0);
    t.handlePadDown (0, transport, dummy);
    t.processBlock (dummy, kSamplesPerBeat, transport); // slot 0 has now played 1.0 beat of its 2-beat loop

    transport = transportAt (1.0);
    t.handlePadDown (1, transport, dummy); // switch to slot 1 under legato

    juce::MidiBuffer output;
    t.processBlock (output, kSamplesPerBeat, transport);

    REQUIRE (t.getActiveSlot() == 1);
    REQUIRE (hasNoteOnAt (output, 0, 61)); // fires immediately: phase carried over (local beat 1.0), not reset to 0
}

TEST_CASE ("Without legato, switching clips always restarts at local beat 0", "[track]")
{
    Track t;
    t.prepare (kSampleRate);
    t.setDefaultQuantize (LaunchQuantize::None);
    t.setLegatoMode (false);

    t.setClip (0, makeClip (2.0));
    setMidiClip (t, 1, makeClip (1.0))->addNote ({ 62, 100, 1, 0.0, 0.05 }); // fires only if phase resets to 0

    juce::MidiBuffer dummy;
    auto transport = transportAt (0.0);
    t.handlePadDown (0, transport, dummy);
    t.processBlock (dummy, kSamplesPerBeat, transport);

    transport = transportAt (1.0);
    t.handlePadDown (1, transport, dummy);

    juce::MidiBuffer output;
    t.processBlock (output, kSamplesPerBeat, transport);

    REQUIRE (hasNoteOnAt (output, 0, 62));
}

TEST_CASE ("Follow action switches to another slot after the configured number of bars", "[track]")
{
    Track t;
    t.prepare (kSampleRate);
    t.setDefaultQuantize (LaunchQuantize::None);

    t.setClip (0, makeClip (4.0));
    t.setClip (1, makeClip (4.0));

    FollowAction fa;
    fa.type = FollowActionType::Next;
    fa.afterBars = 2;
    fa.chance = 1.0f;
    t.getClip (0)->setFollowAction (fa);

    juce::MidiBuffer dummy;
    auto transport = transportAt (0.0, /*barIndex*/ 0, 0.0, 4.0);
    t.handlePadDown (0, transport, dummy);
    t.processBlock (dummy, kSamplesPerBeat, transport); // arms followTargetBarIndex_ = 0 + 2 = 2

    transport = transportAt (4.0, /*barIndex*/ 1, 4.0, 4.0);
    t.processBlock (dummy, kSamplesPerBeat, transport);
    REQUIRE (t.getActiveSlot() == 0); // not yet (barIndex 1 < target 2)

    transport = transportAt (8.0, /*barIndex*/ 2, 8.0, 4.0);
    juce::MidiBuffer output;
    t.processBlock (output, kSamplesPerBeat, transport);
    REQUIRE (t.getActiveSlot() == 1); // fired: barIndex reached the target
}

TEST_CASE ("Capture turns recent live input into a new clip", "[track][capture]")
{
    Track t;
    t.prepare (kSampleRate);

    // Feed a note from beat 0 to beat 1 (channel 1, pitch 60), one block at a time.
    juce::MidiBuffer in1;
    in1.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
    t.feedLiveInput (in1, 0.0, 0.001);

    juce::MidiBuffer in2;
    in2.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
    t.feedLiveInput (in2, 1.0, 0.001);

    const auto transport = transportAt (4.0, 1, 0.0, 4.0); // "now" = end of a 4-beat (1 bar) window
    REQUIRE (t.captureIntoSlot (2, transport, 1));

    auto* clip = t.getClip (2);
    REQUIRE (clip != nullptr);
    REQUIRE (clip->getLengthBeats() == Catch::Approx (4.0));

    auto* midiClip = dynamic_cast<MidiClip*> (clip);
    REQUIRE (midiClip != nullptr);
    REQUIRE (midiClip->getNotes().size() == 1);
    REQUIRE (midiClip->getNotes()[0].pitch == 60);
    REQUIRE (midiClip->getNotes()[0].startBeat == Catch::Approx (0.0)); // window starts at beat 0
}

TEST_CASE ("Track serialization round-trips settings and clip slots", "[track]")
{
    Track t;
    t.setName ("Drums");
    t.setColourARGB (0xFFAABBCC);
    t.setMuted (true);
    t.setArmed (true);
    t.setDefaultQuantize (LaunchQuantize::TwoBars);
    t.setLegatoMode (true);
    t.setClip (3, makeClip (8.0));
    t.getClip (3)->setName ("Verse");

    auto tree = t.toValueTree();

    Track restored;
    restored.fromValueTree (tree);

    REQUIRE (restored.getName() == "Drums");
    REQUIRE (restored.getColourARGB() == 0xFFAABBCC);
    REQUIRE (restored.isMuted());
    REQUIRE (restored.isArmed());
    REQUIRE (restored.getDefaultQuantize() == LaunchQuantize::TwoBars);
    REQUIRE (restored.isLegatoMode());
    REQUIRE (restored.getNumRows() >= 4);
    REQUIRE (restored.hasClip (3));
    REQUIRE (restored.getClip (3)->getName() == "Verse");
    REQUIRE (restored.getClip (3)->getLengthBeats() == Catch::Approx (8.0));
}
