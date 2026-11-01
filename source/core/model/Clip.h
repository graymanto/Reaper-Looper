#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include <optional>
#include "ClipTypes.h"

namespace reaperlooper
{

/**
 * Explicit timing context for one contiguous, non-wrapping segment of a
 * clip's own loop.
 *
 * A clip never sees its *absolute* project position and never needs to
 * handle loop wraparound itself: the caller (Track, via LoopSegmenter) is
 * responsible for splitting a block at each loop-boundary crossing and
 * calling collectMidi() once per segment, each with a local range entirely
 * within [0, clip length]. This is a deliberate departure from `sequenced`'s
 * Clip::collectMidi contract, which left the clip to reconstruct its own
 * sample offsets from an implicit global beat range; see the project plan
 * for why.
 */
struct ClipBlockContext
{
    double localStartBeat { 0.0 };  ///< Clip-local position at the start of this segment. In [0, length).
    double localEndBeat { 0.0 };    ///< Clip-local position at the end of this segment (exclusive), > localStartBeat. May equal the clip length exactly.
    int    sampleOffset { 0 };      ///< Offset into the destination MidiBuffer where this segment begins.
    int    numSamples { 0 };        ///< Length of this segment, in samples.
    double beatsPerSample { 0.0 };  ///< For placing notes at sub-segment sample accuracy.
};

/**
 * Abstract base for everything that can occupy a grid cell.
 *
 * Each clip has its own, independent loop length and phase - there is no
 * session-wide "master loop" (see the plan: `sequenced`'s master-loop model,
 * where a shorter clip goes silent until the longest clip anywhere wraps,
 * is explicitly wrong for a live looper). Track owns the per-slot loop
 * anchor (the absolute beat at which this clip's local beat 0 last
 * occurred) and is responsible for calling collectMidi() with correctly
 * segmented, clip-local ranges.
 */
class Clip
{
public:
    virtual ~Clip() = default;

    /** This clip's own loop length, in quarter-note beats. Always > 0. */
    virtual double getLengthBeats() const noexcept = 0;

    /** A short, stable identifier used for serialization dispatch (ClipFactory). */
    virtual juce::String getTypeId() const = 0;

    //==============================================================================
    // Metadata common to every clip type. Content production (collectMidi,
    // and in a later phase an audio equivalent) is subclass-specific.

    bool isLooping() const noexcept { return looping_; }
    void setLooping (bool shouldLoop) noexcept { looping_ = shouldLoop; }

    LaunchMode getLaunchMode() const noexcept { return launchMode_; }
    void setLaunchMode (LaunchMode mode) noexcept { launchMode_ = mode; }

    /** nullopt = use the track/session default quantize value. */
    std::optional<LaunchQuantize> getQuantizeOverride() const noexcept { return quantizeOverride_; }
    void setQuantizeOverride (std::optional<LaunchQuantize> q) noexcept { quantizeOverride_ = q; }

    const FollowAction& getFollowAction() const noexcept { return followAction_; }
    void setFollowAction (const FollowAction& fa) noexcept { followAction_ = fa; }

    juce::String getName() const { return name_; }
    void setName (const juce::String& n) { name_ = n; }

    //==============================================================================
    /**
     * Appends MIDI events for one non-wrapping loop segment into `output`.
     * `ctx.localStartBeat`/`localEndBeat` are guaranteed to lie within
     * [0, getLengthBeats()].
     */
    virtual void collectMidi (juce::MidiBuffer& output, const ClipBlockContext& ctx) const = 0;

    //==============================================================================
    virtual juce::ValueTree toValueTree() const;
    virtual void fromValueTree (const juce::ValueTree& tree);

protected:
    bool looping_ { true };
    LaunchMode launchMode_ { LaunchMode::Toggle };
    std::optional<LaunchQuantize> quantizeOverride_;
    FollowAction followAction_;
    juce::String name_;
};

} // namespace reaperlooper
