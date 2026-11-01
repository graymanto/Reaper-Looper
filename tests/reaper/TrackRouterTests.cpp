#include <catch2/catch_all.hpp>
#include "reaper/TrackRouter.h"
#include "FakeReaper.h"

using namespace reaperlooper;

TEST_CASE ("routeColumnToTrack creates a MIDI-only send filtered to the given channel", "[reaper][trackrouter]")
{
    FakeReaper::reset();
    auto* src = FakeReaper::world().addTrack ("Looper");
    auto* dest = FakeReaper::world().addTrack ("Synth");

    const int idx = TrackRouter::routeColumnToTrack (FakeReaper::makeFunctions(), nullptr,
                                                       FakeReaper::toHandle (src), FakeReaper::toHandle (dest), 3);

    REQUIRE (idx == 0);
    REQUIRE (src->sends.size() == 1);
    REQUIRE (src->sends[0].dest == FakeReaper::toHandle (dest));
    REQUIRE (src->sends[0].srcChan == Catch::Approx (-1.0)); // audio portion disabled
    REQUIRE (src->sends[0].midiFlags == 3);
    REQUIRE (FakeReaper::world().undoBeginCount == 1);
    REQUIRE (FakeReaper::world().undoEndCount == 1);
}

TEST_CASE ("routeColumnToTrack reuses an existing send instead of duplicating it", "[reaper][trackrouter]")
{
    FakeReaper::reset();
    auto* src = FakeReaper::world().addTrack ("Looper");
    auto* dest = FakeReaper::world().addTrack ("Synth");
    const auto f = FakeReaper::makeFunctions();

    TrackRouter::routeColumnToTrack (f, nullptr, FakeReaper::toHandle (src), FakeReaper::toHandle (dest), 3);
    const int secondIdx = TrackRouter::routeColumnToTrack (f, nullptr, FakeReaper::toHandle (src), FakeReaper::toHandle (dest), 5);

    REQUIRE (secondIdx == 0);
    REQUIRE (src->sends.size() == 1); // no duplicate
    REQUIRE (src->sends[0].midiFlags == 5); // updated, not left at the first call's value
}

TEST_CASE ("routeColumnToTrack clamps an out-of-range MIDI channel into 1-16", "[reaper][trackrouter]")
{
    FakeReaper::reset();
    auto* src = FakeReaper::world().addTrack();
    auto* dest = FakeReaper::world().addTrack();
    const auto f = FakeReaper::makeFunctions();

    TrackRouter::routeColumnToTrack (f, nullptr, FakeReaper::toHandle (src), FakeReaper::toHandle (dest), 0);
    REQUIRE (src->sends[0].midiFlags == 1);

    TrackRouter::routeColumnToTrack (f, nullptr, FakeReaper::toHandle (src), FakeReaper::toHandle (dest), 99);
    REQUIRE (src->sends[0].midiFlags == 16);
}

TEST_CASE ("unroute removes an existing send", "[reaper][trackrouter]")
{
    FakeReaper::reset();
    auto* src = FakeReaper::world().addTrack();
    auto* dest = FakeReaper::world().addTrack();
    const auto f = FakeReaper::makeFunctions();

    TrackRouter::routeColumnToTrack (f, nullptr, FakeReaper::toHandle (src), FakeReaper::toHandle (dest), 1);
    REQUIRE (src->sends.size() == 1);

    TrackRouter::unroute (f, nullptr, FakeReaper::toHandle (src), FakeReaper::toHandle (dest));
    REQUIRE (src->sends.empty());
    REQUIRE (FakeReaper::world().undoBeginCount == 2); // one for the route, one for the unroute
}

TEST_CASE ("unroute is a no-op when no matching send exists", "[reaper][trackrouter]")
{
    FakeReaper::reset();
    auto* src = FakeReaper::world().addTrack();
    auto* dest = FakeReaper::world().addTrack();
    const auto f = FakeReaper::makeFunctions();

    TrackRouter::unroute (f, nullptr, FakeReaper::toHandle (src), FakeReaper::toHandle (dest));

    REQUIRE (src->sends.empty());
    REQUIRE (FakeReaper::world().undoBeginCount == 0); // never touched - nothing to undo
}

TEST_CASE ("TrackRouter functions are safe no-ops when not connected to REAPER", "[reaper][trackrouter]")
{
    const ReaperFunctions empty;
    REQUIRE (TrackRouter::routeColumnToTrack (empty, nullptr, nullptr, nullptr, 1) == -1);
    REQUIRE (TrackRouter::findExistingSend (empty, nullptr, nullptr) == -1);
}
