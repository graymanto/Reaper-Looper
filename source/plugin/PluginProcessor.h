#pragma once

#include <JuceHeader.h>
#include "reaper/ReaperApi.h"
#include "core/session/Session.h"
#include "core/controller/GridViewport.h"
#include "LaunchpadController.h"

namespace reaperlooper
{

/**
 * A MIDI-effect VST3 that currently exists to prove, end-to-end, the
 * pieces built so far:
 *  - the REAPER API bridge (Phase 0, ReaperApi)
 *  - the looper engine (Phase 1, Session/Track) and REAPER adapter
 *    (Phase 2, TransportBridge/TrackRegistry/TrackRouter)
 *  - APC mini mk2 hardware control (Phase 3, LaunchpadController)
 *
 * `testSession_` is a small, hardcoded grid (not yet driven by any real
 * UI or routing - that's Phase 4+) that exists purely so pressing pads on
 * real hardware has something visible to react to: a couple of slots are
 * pre-populated as "occupied", and launching/stopping them is reflected
 * live in the pads' LEDs. Its per-track MIDI output is intentionally
 * discarded for now (see processBlock) - routing it to real REAPER tracks
 * is Phase 4+ work, once there's a UI to configure that routing through.
 *
 * Produces no audio.
 */
class ReaperLooperProcessor final : public juce::AudioProcessor
{
public:
    ReaperLooperProcessor();
    ~ReaperLooperProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return "ReaperLooper"; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    /** Gives the JUCE VST3 wrapper access to ReaperApi::setIHostApplication(). */
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &reaperApi_; }

    const ReaperApi& getReaperApi() const noexcept { return reaperApi_; }
    const LaunchpadController& getLaunchpadController() const noexcept { return launchpadController_; }
    LaunchpadController& getLaunchpadController() noexcept { return launchpadController_; }
    const Session& getTestSession() const noexcept { return testSession_; }
    const GridViewport& getViewport() const noexcept { return viewport_; }

private:
    void autoDetectApcMiniMk2();
    void populateTestSession();

    ReaperApi reaperApi_;
    Session testSession_;
    GridViewport viewport_;
    LaunchpadController launchpadController_;
    std::vector<juce::MidiBuffer> trackOutputs_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReaperLooperProcessor)
};

} // namespace reaperlooper
