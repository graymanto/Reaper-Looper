#include <catch2/catch_all.hpp>
#include "core/controller/GridViewport.h"

using namespace reaperlooper;

TEST_CASE ("GridViewport maps physical coordinates to logical ones via its origin", "[controller][viewport]")
{
    GridViewport v;
    v.originTrack = 3;
    v.originRow = 2;

    REQUIRE (v.logicalTrack (0) == 3);
    REQUIRE (v.logicalTrack (4) == 7);
    REQUIRE (v.logicalRow (0) == 2);
    REQUIRE (v.logicalRow (5) == 7);
}

TEST_CASE ("GridViewport defaults to an unscrolled 8x8 window", "[controller][viewport]")
{
    const GridViewport v;
    REQUIRE (v.originTrack == 0);
    REQUIRE (v.originRow == 0);
    REQUIRE (v.width == 8);
    REQUIRE (v.height == 8);
}

TEST_CASE ("GridViewport::clampTo keeps the origin within [0, total - size]", "[controller][viewport]")
{
    GridViewport v;
    v.originTrack = 100;
    v.originRow = -5;

    v.clampTo (20, 20); // grid is 20x20, viewport is 8x8 -> max origin is 12

    REQUIRE (v.originTrack == 12);
    REQUIRE (v.originRow == 0);
}

TEST_CASE ("GridViewport::clampTo pins the origin to 0 when the grid is smaller than the viewport", "[controller][viewport]")
{
    GridViewport v;
    v.originTrack = 3;
    v.originRow = 3;

    v.clampTo (4, 4); // grid smaller than the 8x8 viewport in both dimensions

    REQUIRE (v.originTrack == 0);
    REQUIRE (v.originRow == 0);
}

TEST_CASE ("GridViewport::clampTo is a no-op when already within range", "[controller][viewport]")
{
    GridViewport v;
    v.originTrack = 2;
    v.originRow = 1;

    v.clampTo (20, 20);

    REQUIRE (v.originTrack == 2);
    REQUIRE (v.originRow == 1);
}
