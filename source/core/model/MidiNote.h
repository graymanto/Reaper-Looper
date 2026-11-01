#pragma once

#include <cmath>

namespace reaperlooper
{

/** A single recorded/drawn MIDI note, in clip-local beat coordinates. */
struct MidiNote
{
    int pitch { 60 };
    int velocity { 100 };
    int channel { 1 };            ///< 1-16

    double startBeat { 0.0 };     ///< Position within the clip, quarter-note units, >= 0.
    double lengthBeats { 1.0 };   ///< > 0. May exceed the clip length (notes do not wrap).

    double endBeat() const noexcept { return startBeat + lengthBeats; }

    bool operator== (const MidiNote& other) const noexcept
    {
        constexpr double epsilon = 1.0e-9;
        return pitch == other.pitch && velocity == other.velocity && channel == other.channel
            && std::abs (startBeat - other.startBeat) < epsilon
            && std::abs (lengthBeats - other.lengthBeats) < epsilon;
    }
};

} // namespace reaperlooper
