#include "PluginEditor.h"
#include "reaper/TrackRegistry.h"
#include "reaper/TrackRouter.h"
#include "reaper/TransportBridge.h"

namespace reaperlooper
{

namespace
{
    constexpr int kTimerHz = 15; // matches sequenced's "fast while there's anything to animate" rate; cheap enough to just always run it.
    constexpr int kTransportBarHeight = 40;
}

ReaperLooperEditor::ReaperLooperEditor (ReaperLooperProcessor& p)
    : juce::AudioProcessorEditor (&p), processor_ (p),
      transportBar_ ({
          [this] { TransportBridge::toggleRecord (processor_.getReaperApi().getFunctions()); },
          // Applies to new tracks only, not retroactively to existing ones -
          // matches Session::setGlobalDefaultQuantize()'s own documented
          // scope. Queued rather than called directly for the same reason
          // every other Session mutation is - see UiEventQueue's doc comment.
          [this] (LaunchQuantize q) { processor_.getUiEventQueue().pushSetGlobalQuantize (q); }
      }),
      gridView_ ({
          [this] (int t, int r) { processor_.getUiEventQueue().pushPadDown (t, r); },
          [this] (int t, int r) { processor_.getUiEventQueue().pushPadDownImmediate (t, r); },
          [this] (int t, int r) { processor_.getUiEventQueue().pushPadUp (t, r); },
          [this] (int t, int r) { processor_.getUiEventQueue().pushCreatePlaceholder (t, r); },
          [this] (int t, int r) { processor_.getUiEventQueue().pushClearClip (t, r); },
          [this] (int r) { processor_.getUiEventQueue().pushSceneLaunch (r); },
          [this] (int t) { processor_.getUiEventQueue().pushTrackStop (t); },
          [this] { processor_.getUiEventQueue().pushAddTrack(); },
          [this] { processor_.getUiEventQueue().pushAddScene(); },
          [this] (int t, MediaTrack* oldDest, MediaTrack* newDest)
          {
              const auto& api = processor_.getReaperApi();
              auto* hostTrack = api.getHostTrack();
              if (hostTrack == nullptr)
                  return;
              auto* proj = api.getHostProject();
              const auto& f = api.getFunctions();

              // Known Phase 4 limitation: if two columns are routed to the
              // SAME destination track, they share one REAPER send
              // (TrackRouter matches sends by destination track only, not
              // by channel) - the most recently set channel wins for that
              // shared send. Narrow enough to accept for now; see the README.
              if (oldDest != nullptr && oldDest != newDest)
                  TrackRouter::unroute (f, proj, hostTrack, oldDest);
              if (newDest != nullptr)
                  TrackRouter::routeColumnToTrack (f, proj, hostTrack, newDest, t + 1);
          },
          [this] (int delta) { processor_.getUiEventQueue().pushPageTracks (delta); },
          [this] (int delta) { processor_.getUiEventQueue().pushPageRows (delta); }
      })
{
    setLookAndFeel (&lookAndFeel_);

    addAndMakeVisible (transportBar_);
    addAndMakeVisible (gridView_);

    setResizable (true, true);
    setResizeLimits (720, 480, 2400, 1600);
    setSize (1100, 760);

    startTimerHz (kTimerHz);
    timerCallback(); // paint something sensible before the first tick
}

ReaperLooperEditor::~ReaperLooperEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ReaperLooperEditor::paint (juce::Graphics& g)
{
    g.fillAll (Theme::backgroundDeep);
}

void ReaperLooperEditor::resized()
{
    auto bounds = getLocalBounds();
    transportBar_.setBounds (bounds.removeFromTop (kTransportBarHeight));
    gridView_.setBounds (bounds.reduced (6));
}

void ReaperLooperEditor::timerCallback()
{
    const auto& api = processor_.getReaperApi();
    const auto transport = processor_.getLastTransportState();

    transportBar_.refresh (transport, buildReaperStatusText(), buildHardwareStatusText());

    // LED feedback for the hardware controller - message thread, matching
    // LaunchpadController's documented thread-safety contract (processBlock
    // on the audio thread only updates Session state).
    processor_.getLaunchpadController().sendLedUpdates (processor_.getSession(), processor_.getViewport());

    std::vector<TrackRegistry::TrackInfo> reaperTracks;
    if (api.isConnected())
        reaperTracks = TrackRegistry::listTracks (api.getFunctions(), api.getHostProject());

    gridView_.refresh (processor_.getSession(), processor_.getViewport(), transport.barLengthQuarterNotes,
                       reaperTracks, api.getHostTrack(), transport.positionQuarterNotes);
}

juce::String ReaperLooperEditor::buildReaperStatusText() const
{
    const auto& api = processor_.getReaperApi();
    if (api.isConnected())
        return "REAPER: connected";
    if (api.numUnresolvedFunctions() > 0)
        return "REAPER: partial (" + juce::String (api.numUnresolvedFunctions()) + " unresolved)";
    return "REAPER: not connected";
}

juce::String ReaperLooperEditor::buildHardwareStatusText() const
{
    const auto& controller = processor_.getLaunchpadController();
    if (controller.getDeviceType() == LaunchpadDeviceType::None)
        return "Controller: none";
    if (controller.getInputDeviceId().isEmpty() && controller.getOutputDeviceId().isEmpty())
        return "APC mini mk2: not found";
    return controller.isOutputConnected() ? "APC mini mk2: connected" : "APC mini mk2: input only";
}

} // namespace reaperlooper
