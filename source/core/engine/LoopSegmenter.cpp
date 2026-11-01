#include "LoopSegmenter.h"
#include <cmath>

namespace reaperlooper::LoopSegmenter
{

namespace
{
    constexpr double kEpsilon = 1.0e-9;

    double wrapIntoRange (double localStart, double clipLengthBeats)
    {
        // fmod can leave us a hair outside [0, length) due to floating point
        // error on huge absolute beat positions; nudge back in. Bounded to a
        // couple of iterations in practice.
        while (localStart >= clipLengthBeats)
            localStart -= clipLengthBeats;
        while (localStart < 0.0)
            localStart += clipLengthBeats;
        return localStart;
    }
}

std::vector<reaperlooper::ClipBlockContext> splitIntoLoopSegments (double absoluteBlockStartBeat,
                                                                    int blockNumSamples,
                                                                    double beatsPerSample,
                                                                    double loopAnchorBeat,
                                                                    double clipLengthBeats,
                                                                    bool looping)
{
    std::vector<reaperlooper::ClipBlockContext> segments;

    if (blockNumSamples <= 0 || clipLengthBeats <= 0.0 || beatsPerSample <= 0.0)
        return segments;

    double remainingBeats = static_cast<double> (blockNumSamples) * beatsPerSample;
    double absStart = absoluteBlockStartBeat;
    int samplesUsed = 0;

    while (remainingBeats > kEpsilon && samplesUsed < blockNumSamples)
    {
        const double sinceAnchor = absStart - loopAnchorBeat;
        double localStart;

        if (looping)
        {
            localStart = wrapIntoRange (std::fmod (sinceAnchor, clipLengthBeats), clipLengthBeats);
        }
        else
        {
            // Must NOT wrap here: fmod would make a one-shot clip that has
            // already finished look like it's back at the start of another
            // "loop" iteration. Treat anything at or past the end as done.
            if (sinceAnchor >= clipLengthBeats - kEpsilon)
                break;
            localStart = juce::jmax (0.0, sinceAnchor);
        }

        const double beatsLeftInLoop = clipLengthBeats - localStart;
        const bool isFinalSegment = beatsLeftInLoop >= remainingBeats - kEpsilon;

        const int segSamples = isFinalSegment
            ? (blockNumSamples - samplesUsed) // use every remaining sample - avoids a rounding gap at block end
            : juce::jlimit (1, blockNumSamples - samplesUsed,
                             static_cast<int> (std::round (beatsLeftInLoop / beatsPerSample)));

        reaperlooper::ClipBlockContext ctx;
        ctx.localStartBeat = localStart;
        ctx.localEndBeat = localStart + static_cast<double> (segSamples) * beatsPerSample;
        ctx.sampleOffset = samplesUsed;
        ctx.numSamples = segSamples;
        ctx.beatsPerSample = beatsPerSample;
        segments.push_back (ctx);

        samplesUsed += segSamples;
        const double beatsAdvanced = static_cast<double> (segSamples) * beatsPerSample;
        absStart += beatsAdvanced;
        remainingBeats -= beatsAdvanced;

        if (! looping)
            break; // a one-shot clip needs at most one segment.
    }

    return segments;
}

} // namespace reaperlooper::LoopSegmenter
