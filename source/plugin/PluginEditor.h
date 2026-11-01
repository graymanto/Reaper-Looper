#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

namespace reaperlooper
{

/**
 * Phase 0 skeleton editor. No looper UI yet - this exists purely to make
 * the REAPER API connection visible: whether IReaperHostApplication was
 * found, and (if so) the live track count/names read back through the
 * resolved function pointers. Phase 4 replaces this with the real grid.
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
    juce::String buildStatusText() const;

    ReaperLooperProcessor& processor_;
    juce::Label statusLabel_;
    juce::TextEditor trackListBox_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReaperLooperEditor)
};

} // namespace reaperlooper
