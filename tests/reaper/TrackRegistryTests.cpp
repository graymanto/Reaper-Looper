#include <catch2/catch_all.hpp>
#include "reaper/TrackRegistry.h"
#include "FakeReaper.h"

using namespace reaperlooper;

TEST_CASE ("listTracks returns every track with its name and colour", "[reaper][trackregistry]")
{
    FakeReaper::reset();
    FakeReaper::world().addTrack ("Drums");
    auto* bass = FakeReaper::world().addTrack ("Bass");
    bass->customColour = 0x1000000 | FakeReaper::colorToNative (255, 128, 64);

    const auto tracks = TrackRegistry::listTracks (FakeReaper::makeFunctions(), nullptr);

    REQUIRE (tracks.size() == 2);
    REQUIRE (tracks[0].name == "Drums");
    REQUIRE (tracks[0].colourARGB == 0); // no custom colour set
    REQUIRE (tracks[1].name == "Bass");
    REQUIRE (tracks[1].colourARGB == 0xFFFF8040); // 0xFF alpha + the RGB set above
}

TEST_CASE ("describeTrack handles a null track gracefully", "[reaper][trackregistry]")
{
    FakeReaper::reset();
    const auto info = TrackRegistry::describeTrack (FakeReaper::makeFunctions(), nullptr);

    REQUIRE (info.track == nullptr);
    REQUIRE (info.name.isEmpty());
    REQUIRE (info.colourARGB == 0);
}

TEST_CASE ("createTrack appends, names, and wraps the action in an undo block", "[reaper][trackregistry]")
{
    FakeReaper::reset();
    FakeReaper::world().addTrack ("Existing");

    auto* created = TrackRegistry::createTrack (FakeReaper::makeFunctions(), nullptr, "New Track");

    REQUIRE (created != nullptr);
    REQUIRE (FakeReaper::world().tracks.size() == 2);
    REQUIRE (FakeReaper::fromHandle (created)->name == "New Track");
    REQUIRE (FakeReaper::world().undoBeginCount == 1);
    REQUIRE (FakeReaper::world().undoEndCount == 1);
}

TEST_CASE ("TrackRegistry functions are safe no-ops when not connected to REAPER", "[reaper][trackregistry]")
{
    const ReaperFunctions empty; // every member null - "not connected"

    REQUIRE (TrackRegistry::listTracks (empty, nullptr).empty());
    REQUIRE (TrackRegistry::createTrack (empty, nullptr, "x") == nullptr);
    REQUIRE (TrackRegistry::describeTrack (empty, nullptr).track == nullptr);
}
