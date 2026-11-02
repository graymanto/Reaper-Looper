#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/ReaperLooperLookAndFeel.h"
#include "ui/transport/TransportBar.h"
#include "ui/grid/GridView.h"

namespace reaperlooper
{

/**
 * The real grid editor (Phase 4) - replaces the Phase 0-3 plain-text
 * status view with TransportBar + GridView. Every interaction is pushed
 * into the processor's UiEventQueue (or, for routing, calls TrackRouter
 * directly - that doesn't touch Session/Track, see UiEventQueue's doc
 * comment for why the two are handled differently); refresh() just reads.
 */
class ReaperLooperEditor final : public juce::AudioProcessorEditor,
                                  private juce::Timer
{
public:
    explicit ReaperLooperEditor (ReaperLooperProcessor&);
    ~ReaperLooperEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    juce::String buildReaperStatusText() const;
    juce::String buildHardwareStatusText() const;

    ReaperLooperProcessor& processor_;
    ReaperLooperLookAndFeel lookAndFeel_;

    TransportBar transportBar_;
    GridView gridView_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReaperLooperEditor)
};

} // namespace reaperlooper
