#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "reaper/TransportBridge.h"
#include "core/model/MidiClip.h"

namespace reaperlooper
{

ReaperLooperProcessor::ReaperLooperProcessor()
    : AudioProcessor (BusesProperties()) // no audio buses - MIDI effect only
{
    populateTestSession();
    launchpadController_.setDeviceType (LaunchpadDeviceType::ApcMiniMk2);
    autoDetectApcMiniMk2();
}

ReaperLooperProcessor::~ReaperLooperProcessor() = default;

void ReaperLooperProcessor::populateTestSession()
{
    // A small, hardcoded grid purely so real hardware has something
    // visible to react to before any UI/routing exists - see the class
    // doc comment. Not meant to represent a real song.
    for (int i = 0; i < 8; ++i)
        testSession_.addTrack();
    for (int i = 0; i < 8; ++i)
        testSession_.addScene();

    auto clipA = std::make_unique<MidiClip>();
    clipA->setLengthBeats (4.0);
    clipA->addNote ({ 60, 100, 1, 0.0, 1.0 });
    testSession_.getTrack (0).setClip (0, std::move (clipA));

    auto clipB = std::make_unique<MidiClip>();
    clipB->setLengthBeats (2.0);
    clipB->addNote ({ 64, 100, 1, 0.0, 0.5 });
    testSession_.getTrack (1).setClip (2, std::move (clipB));
}

void ReaperLooperProcessor::autoDetectApcMiniMk2()
{
    const auto nameHint = LaunchpadProfile::forDevice (LaunchpadDeviceType::ApcMiniMk2).deviceNameHint();

    for (const auto& dev : juce::MidiInput::getAvailableDevices())
        if (dev.name.containsIgnoreCase (nameHint))
            { launchpadController_.setInputDeviceId (dev.identifier); break; }

    for (const auto& dev : juce::MidiOutput::getAvailableDevices())
        if (dev.name.containsIgnoreCase (nameHint))
            { launchpadController_.setOutputDeviceId (dev.identifier); break; }
}

void ReaperLooperProcessor::prepareToPlay (double sampleRate, int)
{
    testSession_.prepare (sampleRate);
    launchpadController_.prepare (sampleRate);
}

void ReaperLooperProcessor::releaseResources() {}

bool ReaperLooperProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet().isDisabled()
        && layouts.getMainOutputChannelSet().isDisabled();
}

void ReaperLooperProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int numSamples = buffer.getNumSamples();
    const auto transport = TransportBridge::read (reaperApi_.getFunctions(), reaperApi_.getHostProject());

    // Pad/scene/track-button input -> Session/Track calls.
    launchpadController_.processBlock (testSession_, viewport_, transport, numSamples, trackOutputs_);

    // Advance playback state. The resulting per-track MIDI (trackOutputs_)
    // is intentionally discarded for now - there is no routing UI yet to
    // say where it should go (Phase 4+); what matters for Phase 3 is that
    // Session state changes correctly and is visible via the APC's LEDs.
    testSession_.processBlock (trackOutputs_, numSamples, transport, juce::MidiBuffer());
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
