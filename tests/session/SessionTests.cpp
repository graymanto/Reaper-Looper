#include <catch2/catch_all.hpp>
#include "core/session/Session.h"
#include "core/session/ProjectSerializer.h"
#include "core/model/MidiClip.h"

using namespace reaperlooper;

namespace
{
    HostTransportState transportAt (double positionBeats, int barIndex = 0, double barStart = 0.0, double barLength = 4.0)
    {
        HostTransportState t;
        t.bpm = 60.0;
        t.isPlaying = true;
        t.positionQuarterNotes = positionBeats;
        t.barStartQuarterNotes = barStart;
        t.barLengthQuarterNotes = barLength;
        t.barIndex = barIndex;
        return t;
    }
}

TEST_CASE ("Session grid grows dynamically, unlike a fixed 8x8", "[session]")
{
    Session s;
    s.prepare (1000.0);

    REQUIRE (s.getNumTracks() == 0);
    REQUIRE (s.getNumRows() == 0);

    for (int i = 0; i < 12; ++i)
        s.addTrack();
    for (int i = 0; i < 20; ++i)
        s.addScene();

    REQUIRE (s.getNumTracks() == 12);
    REQUIRE (s.getNumRows() == 20);

    // Every track must have the full row count, including ones added before
    // the later addScene() calls.
    for (int t = 0; t < s.getNumTracks(); ++t)
        REQUIRE (s.getTrack (t).getNumRows() == 20);
}

TEST_CASE ("addTrack applies the session's global default quantize to new tracks", "[session]")
{
    Session s;
    s.setGlobalDefaultQuantize (LaunchQuantize::EightBars);
    const int idx = s.addTrack();
    REQUIRE (s.getTrack (idx).getDefaultQuantize() == LaunchQuantize::EightBars);
}

TEST_CASE ("Scene launch triggers the right clip in every track, and stops empty ones", "[session]")
{
    Session s;
    s.prepare (1000.0);
    s.addTrack();
    s.addTrack();
    s.addScene ("Intro");
    s.addScene ("Verse");

    auto clipA = std::make_unique<MidiClip>();
    clipA->setLengthBeats (4.0);
    s.getTrack (0).setClip (1, std::move (clipA)); // track 0 has a clip in row 1 ("Verse") only

    // Track 1 has no clip in row 1 at all.

    const auto transport = transportAt (0.0);
    s.handleSceneLaunch (1, transport);

    std::vector<juce::MidiBuffer> outputs;
    s.processBlock (outputs, 1000, transport, juce::MidiBuffer());

    REQUIRE (s.getTrack (0).getActiveSlot() == 1);
    REQUIRE (s.getTrack (1).getActiveSlot() == -1); // nothing to launch - stays stopped
}

TEST_CASE ("stopAll queues a stop in every track", "[session]")
{
    Session s;
    s.prepare (1000.0);
    s.addTrack();
    s.addScene();
    s.getTrack (0).setDefaultQuantize (LaunchQuantize::None); // so the queued stop below resolves within one short test block

    auto clip = std::make_unique<MidiClip>();
    clip->setLengthBeats (4.0);
    s.getTrack (0).setClip (0, std::move (clip));

    auto transport = transportAt (0.0);
    s.handleSceneLaunch (0, transport);
    std::vector<juce::MidiBuffer> outputs;
    s.processBlock (outputs, 1000, transport, juce::MidiBuffer());
    REQUIRE (s.getTrack (0).getActiveSlot() == 0);

    transport = transportAt (1.0);
    s.stopAll (transport);
    s.processBlock (outputs, 1000, transport, juce::MidiBuffer());
    REQUIRE (s.getTrack (0).getActiveSlot() == -1);
}

TEST_CASE ("processBlock sizes its output buffers to the number of tracks", "[session]")
{
    Session s;
    s.prepare (1000.0);
    s.addTrack();
    s.addTrack();
    s.addTrack();

    std::vector<juce::MidiBuffer> outputs;
    s.processBlock (outputs, 512, transportAt (0.0), juce::MidiBuffer());
    REQUIRE (outputs.size() == 3);
}

TEST_CASE ("ProjectSerializer round-trips a full session", "[session][serialization]")
{
    Session s;
    s.prepare (1000.0);
    s.addTrack();
    s.addTrack();
    s.addScene ("Intro");
    s.addScene ("Drop");
    s.setGlobalDefaultQuantize (LaunchQuantize::TwoBars);
    s.getTrack (0).setName ("Lead");

    auto clip = std::make_unique<MidiClip>();
    clip->setLengthBeats (4.0);
    clip->addNote ({ 60, 100, 1, 0.0, 1.0 });
    s.getTrack (1).setClip (1, std::move (clip));

    const auto xml = ProjectSerializer::sessionToXml (s);

    Session restored;
    juce::String error;
    REQUIRE (ProjectSerializer::sessionFromXml (restored, xml, &error));
    REQUIRE (error.isEmpty());

    REQUIRE (restored.getNumTracks() == 2);
    REQUIRE (restored.getNumRows() == 2);
    REQUIRE (restored.getSceneName (0) == "Intro");
    REQUIRE (restored.getSceneName (1) == "Drop");
    REQUIRE (restored.getGlobalDefaultQuantize() == LaunchQuantize::TwoBars);
    REQUIRE (restored.getTrack (0).getName() == "Lead");
    REQUIRE (restored.getTrack (1).hasClip (1));
    REQUIRE (restored.getTrack (1).getClip (1)->getLengthBeats() == Catch::Approx (4.0));
}

TEST_CASE ("ProjectSerializer rejects a tree with the wrong version", "[session][serialization]")
{
    juce::ValueTree bad ("Session");
    bad.setProperty ("version", 999, nullptr);

    Session s;
    juce::String error;
    REQUIRE_FALSE (ProjectSerializer::sessionFromValueTree (s, bad, &error));
    REQUIRE (error.isNotEmpty());
}
