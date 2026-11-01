#pragma once

#include "Track.h"
#include <memory>
#include <vector>

namespace reaperlooper
{

/**
 * The top-level grid: a dynamically-sized set of Tracks (columns), each
 * with the same number of rows (scenes span every track at a given row
 * index, so row count is a session-wide property, not a per-track one -
 * unlike `sequenced`'s fixed 8x8, both dimensions grow via addTrack()/
 * addScene(); see the plan's "grid scalability" decision).
 *
 * Session knows nothing about REAPER, hardware controllers, or UI. A
 * hardware controller's fixed physical surface (e.g. an 8x8 APC mini mk2)
 * is a later-phase viewport concept layered on top of this - the session
 * itself has no notion of "visible window".
 *
 * Known Phase 1 limitation: tracks and scenes can be added and grown, but
 * not removed or reordered - see addTrack()/addScene(). Shrinking/removal
 * is deferred; it needs careful handling of a Track's in-flight
 * active/queued slot indices and is not required for the engine to be
 * otherwise complete and testable.
 */
class Session
{
public:
    Session() = default;

    void prepare (double sampleRate);

    //==============================================================================
    // Grid management

    int getNumTracks() const noexcept { return static_cast<int> (tracks_.size()); }
    int getNumRows() const noexcept { return static_cast<int> (sceneNames_.size()); }

    /** Appends one new, empty track (grown to the current row count) and returns its index. */
    int addTrack();

    /** Appends one new scene row to every track and returns its index. */
    int addScene (const juce::String& name = {});

    Track& getTrack (int index) noexcept { return *tracks_[static_cast<size_t> (index)]; }
    const Track& getTrack (int index) const noexcept { return *tracks_[static_cast<size_t> (index)]; }

    juce::String getSceneName (int row) const;
    void setSceneName (int row, const juce::String& name);

    LaunchQuantize getGlobalDefaultQuantize() const noexcept { return globalDefaultQuantize_; }
    /** Applied to new tracks created by addTrack(); does not retroactively change existing tracks. */
    void setGlobalDefaultQuantize (LaunchQuantize q) noexcept { globalDefaultQuantize_ = q; }

    //==============================================================================
    /** Launches row `row` in every track (Ableton-style scene launch). */
    void handleSceneLaunch (int row, const HostTransportState& transport);

    /** Queues a stop in every track. */
    void stopAll (const HostTransportState& transport);

    //==============================================================================
    /**
     * `trackOutputs` is resized to getNumTracks() if necessary; trackOutputs[i]
     * receives track i's MIDI for this block. `liveInput` is fed to every
     * track's capture history (see Track::feedLiveInput) - it is NOT grid-
     * trigger input; pad presses are delivered via Track::handlePadDown/Up
     * directly (a hardware controller, in a later phase, translates note
     * on/off into those calls rather than routing them through here).
     */
    void processBlock (std::vector<juce::MidiBuffer>& trackOutputs, int numSamples,
                        const HostTransportState& transport, const juce::MidiBuffer& liveInput);

    //==============================================================================
    juce::ValueTree toValueTree() const;
    void fromValueTree (const juce::ValueTree& tree);

private:
    std::vector<std::unique_ptr<Track>> tracks_;
    std::vector<juce::String> sceneNames_;
    double sampleRate_ { 44100.0 };
    LaunchQuantize globalDefaultQuantize_ { LaunchQuantize::OneBar };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Session)
};

} // namespace reaperlooper
