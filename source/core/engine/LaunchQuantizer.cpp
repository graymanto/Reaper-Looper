#include "LaunchQuantizer.h"
#include <cassert>
#include <cmath>

namespace reaperlooper::LaunchQuantizer
{

namespace
{
    constexpr double kEpsilon = 1.0e-9;

    /** Smallest multiple of `m` that is >= pos, treating near-exact matches as exact. */
    double ceilToMultiple (double pos, double m)
    {
        const double n = pos / m;
        const double rounded = std::round (n);
        if (std::abs (n - rounded) < kEpsilon)
            return rounded * m;
        return std::ceil (n) * m;
    }
}

int barsForQuantize (LaunchQuantize quantize) noexcept
{
    switch (quantize)
    {
        case LaunchQuantize::OneBar:     return 1;
        case LaunchQuantize::TwoBars:    return 2;
        case LaunchQuantize::FourBars:   return 4;
        case LaunchQuantize::EightBars:  return 8;
        case LaunchQuantize::None:
        case LaunchQuantize::Quarter:
        case LaunchQuantize::Half:
        default:
            return 0;
    }
}

double nextBoundary (const HostTransportState& transport, LaunchQuantize quantize)
{
    const double pos = transport.positionQuarterNotes;

    switch (quantize)
    {
        case LaunchQuantize::None:    return pos;
        case LaunchQuantize::Quarter: return ceilToMultiple (pos, 1.0);
        case LaunchQuantize::Half:    return ceilToMultiple (pos, 2.0);
        default: break;
    }

    // Bar-based: align to the host's actual bar grid (barIndex), approximating
    // future bars as the same length as the current one (transport.barLengthQuarterNotes).
    // This is exact when no tempo/time-signature change intervenes before the
    // boundary, and self-corrects on the next call otherwise - see header.
    const int N = barsForQuantize (quantize);
    assert (N > 0);

    const bool atBarStart = std::abs (pos - transport.barStartQuarterNotes) < kEpsilon;
    const int startIndex = atBarStart ? transport.barIndex : (transport.barIndex + 1);

    int rem = startIndex % N;
    if (rem < 0)
        rem += N;
    const int barsToAdd = (rem == 0) ? 0 : (N - rem);
    const int totalBarsAhead = (startIndex - transport.barIndex) + barsToAdd;

    return transport.barStartQuarterNotes + static_cast<double> (totalBarsAhead) * transport.barLengthQuarterNotes;
}

} // namespace reaperlooper::LaunchQuantizer
