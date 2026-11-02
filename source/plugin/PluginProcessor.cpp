#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "reaper/TransportBridge.h"
#include "core/model/MidiClip.h"

namespace reaperlooper
{

ReaperLooperProcessor::ReaperLooperProcessor()
    : AudioProcessor (BusesProperties()) // no audio buses - MIDI effect only
{
    // A reasonable starting grid, matching the APC mini mk2's 8x8 physical
    // surface - fully growable from here via the UI's "+ Track"/"+ Scene".
    for (int i = 0; i < 8; ++i)
        session_.addTrack();
    for (int i = 0; i < 8; ++i)
        session_.addScene();

    launchpadController_.setDeviceType (LaunchpadDeviceType::ApcMiniMk2);
    autoDetectApcMiniMk2();
}

ReaperLooperProcessor::~ReaperLooperProcessor() = default;

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
    session_.prepare (sampleRate);
    launchpadController_.prepare (sampleRate);
}

void ReaperLooperProcessor::releaseResources() {}

bool ReaperLooperProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet().isDisabled()
        && layouts.getMainOutputChannelSet().isDisabled();
}

void ReaperLooperProcessor::dispatchUiEvents (const HostTransportState& transport)
{
    // All of these mutate Session/Track state (or the shared viewport) that
    // processBlock() itself reads/writes with no internal locking by
    // design - applying them here, inline in the audio thread's own call,
    // is what makes that safe. See UiEventQueue's doc comment.
    for (const auto& e : uiEventQueue_.drain())
    {
        const bool trackInRange = e.track >= 0 && e.track < session_.getNumTracks();
        juce::MidiBuffer* trackOutput = trackInRange ? &trackOutputs_[static_cast<size_t> (e.track)] : nullptr;

        switch (e.type)
        {
            case UiEventQueue::Type::PadDown:
                if (trackOutput != nullptr) session_.getTrack (e.track).handlePadDown (e.row, transport, *trackOutput);
                break;
            case UiEventQueue::Type::PadUp:
                if (trackOutput != nullptr) session_.getTrack (e.track).handlePadUp (e.row, transport, *trackOutput);
                break;
            case UiEventQueue::Type::PadDownImmediate:
                if (trackOutput != nullptr) session_.getTrack (e.track).handlePadDownImmediate (e.row, transport, *trackOutput);
                break;
            case UiEventQueue::Type::SceneLaunch:
                session_.handleSceneLaunch (e.row, transport);
                break;
            case UiEventQueue::Type::TrackStop:
                if (trackInRange) session_.getTrack (e.track).queueStopAll (transport);
                break;
            case UiEventQueue::Type::ClearClip:
                if (trackInRange) session_.getTrack (e.track).clearClip (e.row);
                break;
            case UiEventQueue::Type::CreatePlaceholder:
                if (trackInRange)
                {
                    auto clip = std::make_unique<MidiClip>();
                    clip->setLengthBeats (4.0);
                    clip->addNote ({ 60, 100, 1, 0.0, 1.0 });
                    session_.getTrack (e.track).setClip (e.row, std::move (clip));
                }
                break;
            case UiEventQueue::Type::AddTrack:
                session_.addTrack();
                viewport_.clampTo (session_.getNumTracks(), session_.getNumRows());
                break;
            case UiEventQueue::Type::AddScene:
                session_.addScene();
                viewport_.clampTo (session_.getNumTracks(), session_.getNumRows());
                break;
            case UiEventQueue::Type::PageTracks:
                viewport_.originTrack += e.track;
                viewport_.clampTo (session_.getNumTracks(), session_.getNumRows());
                break;
            case UiEventQueue::Type::PageRows:
                viewport_.originRow += e.row;
                viewport_.clampTo (session_.getNumTracks(), session_.getNumRows());
                break;
            case UiEventQueue::Type::SetGlobalQuantize:
                session_.setGlobalDefaultQuantize (static_cast<LaunchQuantize> (e.track));
                break;
        }
    }
}

void ReaperLooperProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int numSamples = buffer.getNumSamples();
    const auto transport = TransportBridge::read (reaperApi_.getFunctions(), reaperApi_.getHostProject());

    {
        const juce::SpinLock::ScopedLockType sl (transportStateLock_);
        lastTransportState_ = transport;
    }

    dispatchUiEvents (transport);

    // Pad/scene/track-button input from real hardware -> Session/Track calls.
    launchpadController_.processBlock (session_, viewport_, transport, numSamples, trackOutputs_);

    // Advance playback state. The resulting per-track MIDI (trackOutputs_)
    // is intentionally discarded for now - routing it to real REAPER
    // tracks happens once a column is actually routed (Phase 2's
    // TrackRouter); consuming it here is Phase 6/7 territory (audio
    // clips, Record Performance) once there's somewhere real for it to go
    // beyond a REAPER send REAPER itself already carries the audio for.
    session_.processBlock (trackOutputs_, numSamples, transport, juce::MidiBuffer());
}

HostTransportState ReaperLooperProcessor::getLastTransportState() const noexcept
{
    const juce::SpinLock::ScopedLockType sl (transportStateLock_);
    return lastTransportState_;
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
