#pragma once

#include "../model/Clip.h"
#include "../model/ClipTypes.h"
#include "../engine/HostTransportState.h"
#include "../engine/RecordBuffer.h"
#include <memory>
#include <vector>

namespace reaperlooper
{

/**
 * One column of the grid: a dynamically-sized set of clip slots ("cells",
 * rows) plus the launch state machine and settings that apply to it.
 *
 * Unlike `sequenced`'s Track, this does NOT own a fixed-size array, a MIDI
 * channel assignment, or volume/pan - track lifecycle and routing belong to
 * REAPER (see the plan's Phase 2 adapter layer); this class is purely the
 * musical grid-column model, and knows nothing about REAPER or JUCE's
 * plugin types.
 *
 * Launch resolution is host-bar-grid-relative (Ableton/Logic-style), driven
 * entirely by the HostTransportState snapshot passed into processBlock() -
 * this class never maintains its own clock or "beat 0 = when play was
 * pressed" anchor the way `sequenced`'s Session/Track did.
 *
 * At most one pending launch/stop boundary is tracked at a time (matching
 * Ableton: a second pad press changes what's queued rather than adding a
 * second pending item), and processBlock() resolves at most one boundary
 * per block, splitting the block at the exact sample the boundary falls on.
 * A Repeat-mode retrigger whose interval is shorter than one audio block
 * will have its second-and-later retriggers resolve on the following
 * block(s) rather than within the same one - a realistic limit given
 * typical quantize values and block sizes.
 *
 * Thread safety: processBlock(), handlePadDown()/Up(), handleSceneLaunchForRow(),
 * and feedLiveInput() are all meant to be called from the audio thread (the
 * same convention `sequenced`'s LaunchpadController/Session use - hardware
 * pad presses are collected into a MidiBuffer and consumed during
 * processBlock, not applied directly from a MIDI callback thread). Grid
 * editing methods (setClip, clearClip, ensureRowCount, the settings
 * setters) are intended for the message thread, called only while the
 * audio thread is not concurrently processing this Track (e.g. via REAPER's
 * own suspend-processing-around-edits convention, handled by the adapter
 * layer in a later phase) - there is no internal locking here.
 */
class Track
{
public:
    Track() = default;

    void prepare (double sampleRate) noexcept { sampleRate_ = sampleRate; }

    //==============================================================================
    // Grid management

    int getNumRows() const noexcept { return static_cast<int> (cells_.size()); }
    void ensureRowCount (int rows);

    Clip* getClip (int row) noexcept;
    const Clip* getClip (int row) const noexcept;
    bool hasClip (int row) const noexcept { return getClip (row) != nullptr; }

    /** Grows the grid if necessary. */
    void setClip (int row, std::unique_ptr<Clip> clip);

    /**
     * Note: does not emit note-offs for a clip that is currently playing -
     * this method has no MidiBuffer to write into. Callers clearing an
     * active slot from the audio thread should stop it first (queueStop /
     * handlePadDown) and only clear once it is no longer active.
     */
    void clearClip (int row);

    //==============================================================================
    // Settings

    juce::String getName() const { return name_; }
    void setName (juce::String n) { name_ = std::move (n); }

    juce::uint32 getColourARGB() const noexcept { return colourARGB_; }
    void setColourARGB (juce::uint32 c) noexcept { colourARGB_ = c; }

    bool isMuted() const noexcept { return muted_; }
    void setMuted (bool m) noexcept { muted_ = m; }

    bool isArmed() const noexcept { return armed_; }
    void setArmed (bool a) noexcept { armed_ = a; }

    LaunchQuantize getDefaultQuantize() const noexcept { return defaultQuantize_; }
    void setDefaultQuantize (LaunchQuantize q) noexcept { defaultQuantize_ = q; }

    /** Ableton-style Legato: a clip that replaces a different, currently-playing clip inherits its loop phase instead of restarting at 0. */
    bool isLegatoMode() const noexcept { return legatoMode_; }
    void setLegatoMode (bool l) noexcept { legatoMode_ = l; }

    //==============================================================================
    // Launch state (for UI/hardware LED feedback)

    int getActiveSlot() const noexcept { return activeSlot_; }
    int getQueuedSlot() const noexcept { return queuedSlot_; }
    bool isStopQueued() const noexcept { return stopQueued_; }

    //==============================================================================
    // Interaction. `immediateOutput` receives any notes-off that must happen
    // right now (Gate mode); queued transitions instead emit their notes-off
    // during the processBlock() call in which they resolve.

    void handlePadDown (int row, const HostTransportState& transport, juce::MidiBuffer& immediateOutput);
    void handlePadUp (int row, const HostTransportState& transport, juce::MidiBuffer& immediateOutput);

    /** Ableton-style scene launch: launches this track's clip at `row` (restarting it even if already playing), or stops the track if `row` has no clip. */
    void handleSceneLaunchForRow (int row, const HostTransportState& transport);

    void queueStopAll (const HostTransportState& transport);

    //==============================================================================
    // Capture: continuously-kept live-input history, independent of record-arm.

    void feedLiveInput (const juce::MidiBuffer& input, double blockStartBeat, double beatsPerSample);

    /** Creates a new clip from the last `numBars` bars of live input and places it at `row`. Always succeeds (may capture silence). */
    bool captureIntoSlot (int row, const HostTransportState& transport, int numBars);

    //==============================================================================
    void processBlock (juce::MidiBuffer& output, int numSamples, const HostTransportState& transport);

    //==============================================================================
    juce::ValueTree toValueTree() const;
    void fromValueTree (const juce::ValueTree& tree);

private:
    LaunchQuantize effectiveQuantize (const Clip* clip) const noexcept;
    void queueLaunch (int row, const HostTransportState& transport, bool bypassQuantize);
    void queueStop (const HostTransportState& transport, bool bypassQuantize);

    void resolveQueuedBoundaryNow (const HostTransportState& transport, double resolvedBeat,
                                    juce::MidiBuffer& output, int sampleOffset);
    void resolveLaunchNow (int row, double resolvedBeat, const HostTransportState& transport,
                            juce::MidiBuffer* output, int sampleOffset);
    void resolveStopNow (juce::MidiBuffer* output, int sampleOffset);

    void emitAllNotesOff (juce::MidiBuffer& output, int sampleOffset) const;
    void collectActiveClip (juce::MidiBuffer& output, int sampleOffset, int numSamplesForSegment,
                             double segmentStartBeat, double beatsPerSample);

    void checkFollowAction (const HostTransportState& transport, juce::MidiBuffer& output);
    int resolveFollowTarget (const FollowAction& fa) const;

    std::vector<std::unique_ptr<Clip>> cells_;

    juce::String name_;
    juce::uint32 colourARGB_ { 0 };
    bool muted_ { false };
    bool armed_ { false };
    LaunchQuantize defaultQuantize_ { LaunchQuantize::OneBar };
    bool legatoMode_ { false };

    int activeSlot_ { -1 };
    int queuedSlot_ { -1 };
    bool stopQueued_ { false };
    bool hasQueuedBoundary_ { false };
    double queuedBoundaryBeat_ { 0.0 };

    double activeClipLoopAnchorBeat_ { 0.0 };
    int followTargetBarIndex_ { -1 }; ///< -1 = disarmed (no clip active, or its follow action is None).
    bool repeating_ { false };

    double sampleRate_ { 44100.0 };
    RecordBuffer recordBuffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Track)
};

} // namespace reaperlooper
