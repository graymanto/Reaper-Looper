#pragma once

#include <JuceHeader.h>
#include "../Theme.h"
#include "../../core/engine/HostTransportState.h"
#include "../../core/model/ClipTypes.h"
#include <functional>

namespace reaperlooper
{

/**
 * A minimal transport strip - deliberately not `sequenced`'s TransportBar
 * (BPM drag/tap-tempo, rewind/play/stop, project save/load). None of that
 * makes sense here: REAPER owns the transport. Matches the one thing
 * `sequenced` itself does when running as a plugin rather than standalone -
 * hide the controls a host already provides, keep only what a host
 * doesn't: a live readout, a Record toggle (useful for starting a REAPER
 * recording from the grid without reaching for the host transport), and
 * the session-wide default launch quantize.
 */
class TransportBar final : public juce::Component
{
public:
    struct Callbacks
    {
        std::function<void()> onToggleRecord;
        std::function<void (LaunchQuantize)> onGlobalQuantizeChanged;
    };

    explicit TransportBar (Callbacks cbs);

    /** `reaperStatus`/`hardwareStatus` are short, already-formatted strings (e.g. "REAPER: connected"). */
    void refresh (const HostTransportState& transport, const juce::String& reaperStatus, const juce::String& hardwareStatus);

    void resized() override;
    void paint (juce::Graphics&) override;

private:
    Callbacks cbs_;

    juce::Label positionLabel_;
    juce::Label bpmLabel_;
    juce::TextButton recordButton_ { "REC" };
    juce::ComboBox quantizeCombo_;
    juce::Label statusLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransportBar)
};

} // namespace reaperlooper
