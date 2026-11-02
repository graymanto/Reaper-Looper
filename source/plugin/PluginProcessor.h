#pragma once

#include <JuceHeader.h>
#include "reaper/ReaperApi.h"
#include "core/session/Session.h"
#include "core/controller/GridViewport.h"
#include "LaunchpadController.h"
#include "UiEventQueue.h"

namespace reaperlooper
{

/**
 * The plugin. Owns the Session (the user's actual working grid - grown
 * and edited via the UI/hardware, not a fixed test fixture any more now
 * that Phase 4 gives it a real UI) and everything that feeds it:
 *  - LaunchpadController, for APC mini mk2 input/LEDs (Phase 3).
 *  - UiEventQueue, the equivalent path for mouse input from the grid UI
 *    (Phase 4) - see that class's doc comment for why mouse clicks can't
 *    just call into Session directly from the message thread.
 *  - ReaperApi/TransportBridge, for REAPER's live transport/tempo map.
 *
 * `viewport_` is the single shared GridViewport both the hardware
 * controller and the UI render through - panning it (via the UI's paging
 * buttons, queued like everything else) moves both in lockstep.
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
    LaunchpadController& getLaunchpadController() noexcept { return launchpadController_; }
    const LaunchpadController& getLaunchpadController() const noexcept { return launchpadController_; }
    const Session& getSession() const noexcept { return session_; }
    const GridViewport& getViewport() const noexcept { return viewport_; }
    UiEventQueue& getUiEventQueue() noexcept { return uiEventQueue_; }

    /** A thread-safe snapshot of the transport read during the most recent processBlock() - safe to poll from the UI thread. */
    HostTransportState getLastTransportState() const noexcept;

private:
    void autoDetectApcMiniMk2();
    void dispatchUiEvents (const HostTransportState& transport);

    ReaperApi reaperApi_;
    Session session_;
    GridViewport viewport_;
    LaunchpadController launchpadController_;
    UiEventQueue uiEventQueue_;
    std::vector<juce::MidiBuffer> trackOutputs_;

    mutable juce::SpinLock transportStateLock_;
    HostTransportState lastTransportState_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReaperLooperProcessor)
};

} // namespace reaperlooper
