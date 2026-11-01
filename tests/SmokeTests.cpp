#include <catch2/catch_all.hpp>

// Phase 0: proves the test harness itself builds and runs against
// ReaperLooperCore. Real engine tests (Session/Track/Cell, clock,
// quantization) land in Phase 1 - see /Users/grahamthomas/.claude/plans/linear-exploring-fox.md.

TEST_CASE ("test harness is wired up", "[smoke]")
{
    REQUIRE (1 + 1 == 2);
}
