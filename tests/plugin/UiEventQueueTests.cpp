#include <catch2/catch_all.hpp>
#include "plugin/UiEventQueue.h"

using namespace reaperlooper;

TEST_CASE ("UiEventQueue drains events in push order and then is empty", "[plugin][uieventqueue]")
{
    UiEventQueue q;
    q.pushPadDown (2, 3);
    q.pushPadUp (2, 3);
    q.pushPadDownImmediate (4, 5);
    q.pushSceneLaunch (5);
    q.pushTrackStop (1);
    q.pushClearClip (0, 1);
    q.pushCreatePlaceholder (0, 2);
    q.pushAddTrack();
    q.pushAddScene();
    q.pushPageTracks (8);
    q.pushPageRows (-8);

    const auto events = q.drain();
    REQUIRE (events.size() == 11);

    REQUIRE (events[0].type == UiEventQueue::Type::PadDown);
    REQUIRE (events[0].track == 2);
    REQUIRE (events[0].row == 3);

    REQUIRE (events[1].type == UiEventQueue::Type::PadUp);

    REQUIRE (events[2].type == UiEventQueue::Type::PadDownImmediate);
    REQUIRE (events[2].track == 4);
    REQUIRE (events[2].row == 5);

    REQUIRE (events[3].type == UiEventQueue::Type::SceneLaunch);
    REQUIRE (events[3].row == 5);

    REQUIRE (events[4].type == UiEventQueue::Type::TrackStop);
    REQUIRE (events[4].track == 1);

    REQUIRE (events[5].type == UiEventQueue::Type::ClearClip);
    REQUIRE (events[5].track == 0);
    REQUIRE (events[5].row == 1);

    REQUIRE (events[6].type == UiEventQueue::Type::CreatePlaceholder);
    REQUIRE (events[6].row == 2);

    REQUIRE (events[7].type == UiEventQueue::Type::AddTrack);
    REQUIRE (events[8].type == UiEventQueue::Type::AddScene);

    REQUIRE (events[9].type == UiEventQueue::Type::PageTracks);
    REQUIRE (events[9].track == 8);

    REQUIRE (events[10].type == UiEventQueue::Type::PageRows);
    REQUIRE (events[10].row == -8);

    REQUIRE (q.drain().empty());
}

TEST_CASE ("UiEventQueue::drain on an empty queue returns an empty vector", "[plugin][uieventqueue]")
{
    UiEventQueue q;
    REQUIRE (q.drain().empty());
}
