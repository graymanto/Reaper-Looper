#include "PluginEditor.h"

namespace reaperlooper
{

namespace
{
    constexpr int kTimerHz = 2;
}

ReaperLooperEditor::ReaperLooperEditor (ReaperLooperProcessor& p)
    : juce::AudioProcessorEditor (&p), processor_ (p)
{
    statusLabel_.setJustificationType (juce::Justification::centredLeft);
    statusLabel_.setFont (juce::Font (juce::FontOptions (16.0f)));
    addAndMakeVisible (statusLabel_);

    trackListBox_.setMultiLine (true);
    trackListBox_.setReadOnly (true);
    trackListBox_.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::plain)));
    trackListBox_.setScrollbarsShown (true);
    addAndMakeVisible (trackListBox_);

    setResizable (true, true);
    setResizeLimits (420, 320, 2000, 1400);
    setSize (560, 420);

    statusLabel_.setText (buildStatusText(), juce::dontSendNotification);
    startTimerHz (kTimerHz);
}

ReaperLooperEditor::~ReaperLooperEditor()
{
    stopTimer();
}

void ReaperLooperEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xFF0D0F1A));
}

void ReaperLooperEditor::resized()
{
    auto bounds = getLocalBounds().reduced (12);
    statusLabel_.setBounds (bounds.removeFromTop (28));
    bounds.removeFromTop (8);
    trackListBox_.setBounds (bounds);
}

void ReaperLooperEditor::timerCallback()
{
    statusLabel_.setText (buildStatusText(), juce::dontSendNotification);

    juce::StringArray lines;

    if (processor_.getReaperApi().isConnected())
    {
        const int numTracks = ::GetNumTracks();
        lines.add (juce::String (numTracks) + " track(s) in project:");

        for (int i = 0; i < numTracks; ++i)
        {
            if (auto* track = ::GetTrack (nullptr, i))
            {
                char name[256] = {};
                ::GetSetMediaTrackInfo_String (track, "P_NAME", name, false);

                juce::String displayName = juce::String (name).isNotEmpty()
                    ? juce::String (name)
                    : ("(unnamed track " + juce::String (i + 1) + ")");

                lines.add ("  " + juce::String (i + 1) + ". " + displayName);
            }
        }
    }
    else
    {
        lines.add ("Not connected to REAPER.");
        lines.add ("Load this plugin on a track in REAPER to see live track info here.");
        lines.add ("(Standalone and non-REAPER hosts will never connect - that's expected.)");
    }

    trackListBox_.setText (lines.joinIntoString ("\n"), false);
}

juce::String ReaperLooperEditor::buildStatusText() const
{
    const auto& api = processor_.getReaperApi();

    if (api.isConnected())
        return "REAPER API: connected";

    if (api.numUnresolvedFunctions() > 0)
        return "REAPER API: partially connected (" + juce::String (api.numUnresolvedFunctions())
             + " function(s) unresolved - REAPER version too old?)";

    return "REAPER API: not connected";
}

} // namespace reaperlooper
