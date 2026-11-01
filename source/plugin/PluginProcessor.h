#pragma once

#include <JuceHeader.h>
#include "reaper/ReaperApi.h"

namespace reaperlooper
{

/**
 * Phase 0 skeleton: a MIDI-effect VST3 whose only job right now is to prove
 * the REAPER API bridge end-to-end (see ReaperApi.h) before any looper
 * engine or UI work begins.
 *
 * Produces no audio and passes MIDI through unchanged; everything
 * interesting currently happens in ReaperApi + the editor's connection
 * status display.
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

private:
    ReaperApi reaperApi_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReaperLooperProcessor)
};

} // namespace reaperlooper
