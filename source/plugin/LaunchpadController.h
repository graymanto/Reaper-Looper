#pragma once

#include <JuceHeader.h>
#include "core/controller/LaunchpadProfile.h"
#include "core/controller/GridViewport.h"
#include "core/session/Session.h"
#include <array>
#include <memory>

namespace reaperlooper
{

/**
 * Opens and manages a single grid-controller device (APC Mini Mk2 in
 * Phase 3; Launchpad Mini Mk3 mapping exists but is unverified - see
 * LaunchpadProfile).
 *
 * Ported from `sequenced`'s LaunchpadController, adapted for this
 * project's dynamic Session grid: every grid position is now reached
 * through a GridViewport (the hardware's fixed 8x8 physical surface onto
 * a Session grid that can be larger), and pad presses are translated into
 * Track::handlePadDown()/handlePadUp() (not a single "click" event) so
 * Gate and Repeat launch modes work correctly from real hardware.
 *
 * Track buttons (APC Mini Mk2 only) are hardwired to "stop this column" -
 * `sequenced`'s Solo/Mute/RecordArm track-button modes are not ported;
 * Track mute/arm read-write was already deferred in Phase 2 to the phase
 * that actually needs it (the grid UI), and a button mode selector needs
 * that UI to be meaningful.
 *
 * Responsibilities:
 *  - Receives pad/button presses via its own MidiInput (separate from any
 *    future musical-input chain) and translates them to Session/Track
 *    calls using the current GridViewport.
 *  - Sends LED feedback to its own MidiOutput, diff-cached so only changed
 *    pads/buttons are re-sent.
 *
 * Thread safety:
 *  - processBlock() is called on the AUDIO thread (matching how
 *    PluginProcessor::processBlock() drives everything else touching
 *    Session/Track - see Track.h's own thread-safety note).
 *  - sendLedUpdates()/setDeviceType()/set*DeviceId() are called on the
 *    MESSAGE thread.
 *  - The MidiMessageCollector is internally thread-safe (juce::SpinLock).
 */
class LaunchpadController : public juce::MidiInputCallback
{
public:
    LaunchpadController();
    ~LaunchpadController() override;

    //==============================================================================
    // Configuration (message thread)

    void setDeviceType (LaunchpadDeviceType type);
    void setInputDeviceId (const juce::String& deviceId);
    void setOutputDeviceId (const juce::String& deviceId);

    LaunchpadDeviceType getDeviceType() const noexcept { return profile_.type; }
    juce::String getInputDeviceId() const noexcept { return inputDeviceId_; }
    juce::String getOutputDeviceId() const noexcept { return outputDeviceId_; }
    bool isOutputConnected() const noexcept { return outputDevice_ != nullptr; }

    /** Call after sample rate is known (prepareToPlay). */
    void prepare (double sampleRate);

    //==============================================================================
    // Audio thread

    /**
     * Collects buffered pad/button presses and translates them into
     * Session/Track calls. `trackOutputs` must be sized to
     * session.getNumTracks() (matching Session::processBlock's own
     * convention) - Gate-mode immediate note-offs are written directly
     * into the relevant column's buffer, at sample offset 0 (an
     * acknowledged approximation: a Gate trigger landing mid-block isn't
     * placed at its exact input sample - see the project's README).
     */
    void processBlock (Session& session, const GridViewport& viewport, const HostTransportState& transport,
                        int numSamples, std::vector<juce::MidiBuffer>& trackOutputs);

    //==============================================================================
    // Message thread

    /**
     * Reads current cell/scene/track-button states from the session
     * (through the viewport) and sends LED update messages, only for
     * state that actually changed since the last call.
     */
    void sendLedUpdates (const Session& session, const GridViewport& viewport);

    //==============================================================================
    // juce::MidiInputCallback (MIDI thread - background)
    void handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage&) override;

private:
    void openInput();
    void closeInput();
    void openOutput();
    void closeOutput();

    /** Turns off every LED on the physical 8x8 surface and resets the diff caches. Safe to call before closeOutput(). */
    void clearAllLeds();

    void sendInit();
    void sendOutputMessage (const juce::MidiMessage& msg);

    //==============================================================================
    LaunchpadProfile profile_;
    juce::String inputDeviceId_;
    juce::String outputDeviceId_;

    std::unique_ptr<juce::MidiInput> inputDevice_;
    std::unique_ptr<juce::MidiOutput> outputDevice_;

    juce::MidiMessageCollector inputCollector_;

    // LED state caches, indexed by PHYSICAL [col][row]/[row] - not logical
    // Session coordinates. 0xFF = uninitialised (forces a send on first update).
    std::array<std::array<uint8_t, 8>, 8> lastLedState_;
    std::array<uint8_t, 8> lastSceneLedState_;
    std::array<uint8_t, 8> lastTrackButtonLedState_;

    juce::CriticalSection outputLock_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LaunchpadController)
};

} // namespace reaperlooper
