#include "PluginEditor.h"
#include "reaper/TrackRegistry.h"

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

    const auto& api = processor_.getReaperApi();
    if (api.isConnected())
    {
        auto* proj = api.getHostProject();
        const auto tracks = TrackRegistry::listTracks (api.getFunctions(), proj);

        lines.add (juce::String (tracks.size()) + " track(s) in project:");

        for (size_t i = 0; i < tracks.size(); ++i)
        {
            const auto& t = tracks[i];
            const juce::String displayName = t.name.isNotEmpty()
                ? t.name
                : ("(unnamed track " + juce::String (i + 1) + ")");
            const juce::String colourNote = t.colourARGB != 0
                ? (" [colour " + juce::String::toHexString (static_cast<int> (t.colourARGB)) + "]")
                : juce::String();

            lines.add ("  " + juce::String (i + 1) + ". " + displayName + colourNote
                      + (t.track == api.getHostTrack() ? "  <- this plugin" : ""));
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
