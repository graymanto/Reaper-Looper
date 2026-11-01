#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace reaperlooper
{

ReaperLooperProcessor::ReaperLooperProcessor()
    : AudioProcessor (BusesProperties()) // no audio buses - MIDI effect only
{
}

ReaperLooperProcessor::~ReaperLooperProcessor() = default;

void ReaperLooperProcessor::prepareToPlay (double, int) {}
void ReaperLooperProcessor::releaseResources() {}

bool ReaperLooperProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet().isDisabled()
        && layouts.getMainOutputChannelSet().isDisabled();
}

void ReaperLooperProcessor::processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&)
{
    // Phase 0: pass MIDI through unchanged. The looper engine (Session/Track/
    // Cell model from the plan) replaces this once Phase 1 begins.
}

juce::AudioProcessorEditor* ReaperLooperProcessor::createEditor()
{
    return new ReaperLooperEditor (*this);
}

void ReaperLooperProcessor::getStateInformation (juce::MemoryBlock&) {}
void ReaperLooperProcessor::setStateInformation (const void*, int) {}

} // namespace reaperlooper

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new reaperlooper::ReaperLooperProcessor();
}
