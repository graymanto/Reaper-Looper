#include <catch2/catch_all.hpp>
#include "core/engine/LaunchQuantizer.h"

using namespace reaperlooper;

namespace
{
    HostTransportState makeTransport (double position, double barStart, double barLength, int barIndex)
    {
        HostTransportState t;
        t.positionQuarterNotes = position;
        t.barStartQuarterNotes = barStart;
        t.barLengthQuarterNotes = barLength;
        t.barIndex = barIndex;
        return t;
    }
}

TEST_CASE ("None resolves immediately", "[quantize]")
{
    const auto t = makeTransport (12.34, 12.0, 4.0, 3);
    REQUIRE (LaunchQuantizer::nextBoundary (t, LaunchQuantize::None) == Catch::Approx (12.34));
}

TEST_CASE ("Quarter rounds up to the next whole quarter note", "[quantize]")
{
    const auto t = makeTransport (12.25, 12.0, 4.0, 3);
    REQUIRE (LaunchQuantizer::nextBoundary (t, LaunchQuantize::Quarter) == Catch::Approx (13.0));
}

TEST_CASE ("Quarter is a no-op when already exactly on a quarter note", "[quantize]")
{
    const auto t = makeTransport (13.0, 12.0, 4.0, 3);
    REQUIRE (LaunchQuantizer::nextBoundary (t, LaunchQuantize::Quarter) == Catch::Approx (13.0));
}

TEST_CASE ("Half rounds up to the next half note regardless of time signature", "[quantize]")
{
    const auto t = makeTransport (12.5, 12.0, 4.0, 3);
    REQUIRE (LaunchQuantizer::nextBoundary (t, LaunchQuantize::Half) == Catch::Approx (14.0));
}

TEST_CASE ("OneBar resolves to the start of the next bar when mid-bar", "[quantize]")
{
    const auto t = makeTransport (13.5, 12.0, 4.0, 3);
    REQUIRE (LaunchQuantizer::nextBoundary (t, LaunchQuantize::OneBar) == Catch::Approx (16.0)); // bar 4 starts at 16
}

TEST_CASE ("OneBar resolves to the current bar start when already exactly there", "[quantize]")
{
    const auto t = makeTransport (12.0, 12.0, 4.0, 3);
    REQUIRE (LaunchQuantizer::nextBoundary (t, LaunchQuantize::OneBar) == Catch::Approx (12.0));
}

TEST_CASE ("FourBars aligns to the absolute bar grid, not to a fixed offset from the press", "[quantize]")
{
    // barIndex 5 (6th bar), mid-bar: the next bar index that's a multiple of
    // 4 is 8, not "5 + 4 = 9" - this is the behaviour that distinguishes
    // real bar-grid alignment from `sequenced`'s "beat 0 = press play" model.
    const auto t = makeTransport (22.0, 20.0, 4.0, 5);
    REQUIRE (LaunchQuantizer::nextBoundary (t, LaunchQuantize::FourBars) == Catch::Approx (32.0));
}

TEST_CASE ("FourBars at an already-aligned bar start resolves immediately", "[quantize]")
{
    const auto t = makeTransport (16.0, 16.0, 4.0, 4); // barIndex 4 is already a multiple of 4
    REQUIRE (LaunchQuantizer::nextBoundary (t, LaunchQuantize::FourBars) == Catch::Approx (16.0));
}

TEST_CASE ("Bar-based quantize works with a non-4/4 bar length", "[quantize]")
{
    // 6/8 time: a bar is 3 quarter notes long (6 * 4/8).
    const auto t = makeTransport (13.0, 12.0, 3.0, 4);
    REQUIRE (LaunchQuantizer::nextBoundary (t, LaunchQuantize::OneBar) == Catch::Approx (15.0));
}

TEST_CASE ("barsForQuantize reports bar counts for bar-based values and 0 otherwise", "[quantize]")
{
    REQUIRE (LaunchQuantizer::barsForQuantize (LaunchQuantize::OneBar) == 1);
    REQUIRE (LaunchQuantizer::barsForQuantize (LaunchQuantize::TwoBars) == 2);
    REQUIRE (LaunchQuantizer::barsForQuantize (LaunchQuantize::FourBars) == 4);
    REQUIRE (LaunchQuantizer::barsForQuantize (LaunchQuantize::EightBars) == 8);
    REQUIRE (LaunchQuantizer::barsForQuantize (LaunchQuantize::None) == 0);
    REQUIRE (LaunchQuantizer::barsForQuantize (LaunchQuantize::Quarter) == 0);
    REQUIRE (LaunchQuantizer::barsForQuantize (LaunchQuantize::Half) == 0);
}
