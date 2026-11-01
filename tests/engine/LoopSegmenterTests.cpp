#include <catch2/catch_all.hpp>
#include "core/engine/LoopSegmenter.h"

using namespace reaperlooper;

TEST_CASE ("a block entirely within one loop iteration returns a single segment", "[loopsegmenter]")
{
    // beatsPerSample = 0.001 -> 1000 samples = 1.0 beat. Clip is 4 beats long,
    // well longer than the block, and the block starts right at the anchor.
    const auto segments = LoopSegmenter::splitIntoLoopSegments (0.0, 1000, 0.001, 0.0, 4.0, true);

    REQUIRE (segments.size() == 1);
    REQUIRE (segments[0].localStartBeat == Catch::Approx (0.0));
    REQUIRE (segments[0].localEndBeat == Catch::Approx (1.0));
    REQUIRE (segments[0].sampleOffset == 0);
    REQUIRE (segments[0].numSamples == 1000);
}

TEST_CASE ("a block that crosses one loop boundary splits into two segments", "[loopsegmenter]")
{
    // Clip is 0.6 beats long; the block covers 1.0 beat, so it wraps once.
    const auto segments = LoopSegmenter::splitIntoLoopSegments (0.0, 1000, 0.001, 0.0, 0.6, true);

    REQUIRE (segments.size() == 2);

    REQUIRE (segments[0].localStartBeat == Catch::Approx (0.0));
    REQUIRE (segments[0].localEndBeat == Catch::Approx (0.6));
    REQUIRE (segments[0].sampleOffset == 0);
    REQUIRE (segments[0].numSamples == 600);

    REQUIRE (segments[1].localStartBeat == Catch::Approx (0.0));
    REQUIRE (segments[1].localEndBeat == Catch::Approx (0.4));
    REQUIRE (segments[1].sampleOffset == 600);
    REQUIRE (segments[1].numSamples == 400);

    int totalSamples = 0;
    for (const auto& s : segments)
        totalSamples += s.numSamples;
    REQUIRE (totalSamples == 1000);
}

TEST_CASE ("a block starting mid-loop resumes from the correct local phase", "[loopsegmenter]")
{
    // Anchor at beat 10; block starts at beat 10.3, clip is 1.0 beat long ->
    // local phase should start at 0.3, not 0.
    const auto segments = LoopSegmenter::splitIntoLoopSegments (10.3, 500, 0.001, 10.0, 1.0, true);

    REQUIRE (segments.size() == 1);
    REQUIRE (segments[0].localStartBeat == Catch::Approx (0.3));
    REQUIRE (segments[0].localEndBeat == Catch::Approx (0.8));
}

TEST_CASE ("a non-looping clip returns one segment truncated at its own end", "[loopsegmenter]")
{
    // Clip is 0.6 beats, not looping. Block covers 1.0 beat starting at the
    // anchor - only the first 600 samples should be covered.
    const auto segments = LoopSegmenter::splitIntoLoopSegments (0.0, 1000, 0.001, 0.0, 0.6, false);

    REQUIRE (segments.size() == 1);
    REQUIRE (segments[0].sampleOffset == 0);
    REQUIRE (segments[0].numSamples == 600);
    REQUIRE (segments[0].localEndBeat == Catch::Approx (0.6));
}

TEST_CASE ("a non-looping clip played past its own end returns no segments", "[loopsegmenter]")
{
    // This is the regression case: naive fmod-based wrapping would otherwise
    // make a finished one-shot clip look like it's back at local beat ~0.2.
    const auto segments = LoopSegmenter::splitIntoLoopSegments (5.0, 500, 0.001, 0.0, 0.6, false);
    REQUIRE (segments.empty());
}

TEST_CASE ("zero or negative block length returns no segments", "[loopsegmenter]")
{
    REQUIRE (LoopSegmenter::splitIntoLoopSegments (0.0, 0, 0.001, 0.0, 4.0, true).empty());
    REQUIRE (LoopSegmenter::splitIntoLoopSegments (0.0, 100, 0.001, 0.0, 0.0, true).empty());
}
