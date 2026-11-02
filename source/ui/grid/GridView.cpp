#include "GridView.h"
#include <cmath>

namespace reaperlooper
{

namespace
{
    constexpr int kSceneRailWidth = 120;
    constexpr int kHeaderHeight = 64;
    constexpr int kTopStripHeight = 32;
}

GridView::GridView (Callbacks cbs) : cbs_ (std::move (cbs))
{
    addTrackButton_.onClick = [this] { if (cbs_.onAddTrack) cbs_.onAddTrack(); };
    addSceneButton_.onClick = [this] { if (cbs_.onAddScene) cbs_.onAddScene(); };
    pageTracksLeft_.onClick = [this] { if (cbs_.onPageTracksBy) cbs_.onPageTracksBy (-viewport_.width); };
    pageTracksRight_.onClick = [this] { if (cbs_.onPageTracksBy) cbs_.onPageTracksBy (viewport_.width); };
    pageRowsUp_.onClick = [this] { if (cbs_.onPageRowsBy) cbs_.onPageRowsBy (-viewport_.height); };
    pageRowsDown_.onClick = [this] { if (cbs_.onPageRowsBy) cbs_.onPageRowsBy (viewport_.height); };

    for (auto* b : { &addTrackButton_, &addSceneButton_, &pageTracksLeft_, &pageTracksRight_, &pageRowsUp_, &pageRowsDown_ })
        addAndMakeVisible (*b);

    for (auto* label : { &trackRangeLabel_, &rowRangeLabel_ })
    {
        label->setJustificationType (juce::Justification::centred);
        label->setFont (Theme::monoFont (11.0f));
        label->setColour (juce::Label::textColourId, Theme::textSecondary);
        addAndMakeVisible (*label);
    }

    headers_.resize (static_cast<size_t> (viewport_.width));
    for (int c = 0; c < viewport_.width; ++c)
    {
        auto& h = headers_[static_cast<size_t> (c)];

        h.nameLabel = std::make_unique<juce::Label>();
        h.nameLabel->setJustificationType (juce::Justification::centred);
        h.nameLabel->setFont (Theme::uiFont (12.0f, "Medium"));
        h.nameLabel->setColour (juce::Label::textColourId, Theme::textPrimary);
        addAndMakeVisible (*h.nameLabel);

        h.routingCombo = std::make_unique<juce::ComboBox> ("routing" + juce::String (c));
        h.routingCombo->setTextWhenNothingSelected ("No routing");
        h.routingCombo->onChange = [this, c]
        {
            auto& hdr = headers_[static_cast<size_t> (c)];
            const int id = hdr.routingCombo->getSelectedId(); // 1 = "No routing", i+2 = lastReaperTracks_[i]
            MediaTrack* newDest = (id >= 2 && id - 2 < static_cast<int> (lastReaperTracks_.size()))
                ? lastReaperTracks_[static_cast<size_t> (id - 2)].track : nullptr;
            MediaTrack* oldDest = hdr.currentDest;
            hdr.currentDest = newDest;
            if (cbs_.onRouteColumn)
                cbs_.onRouteColumn (viewport_.logicalTrack (c), oldDest, newDest);
        };
        addAndMakeVisible (*h.routingCombo);

        h.stopButton = std::make_unique<juce::TextButton> ("STOP");
        h.stopButton->onClick = [this, c] { if (cbs_.onTrackStop) cbs_.onTrackStop (viewport_.logicalTrack (c)); };
        addAndMakeVisible (*h.stopButton);
    }

    sceneButtons_.reserve (static_cast<size_t> (viewport_.height));
    for (int r = 0; r < viewport_.height; ++r)
    {
        auto btn = std::make_unique<juce::TextButton>();
        btn->onClick = [this, r] { if (cbs_.onSceneLaunch) cbs_.onSceneLaunch (viewport_.logicalRow (r)); };
        addAndMakeVisible (*btn);
        sceneButtons_.push_back (std::move (btn));
    }

    cells_.resize (static_cast<size_t> (viewport_.width));
    for (int c = 0; c < viewport_.width; ++c)
    {
        auto& column = cells_[static_cast<size_t> (c)];
        column.reserve (static_cast<size_t> (viewport_.height));

        for (int r = 0; r < viewport_.height; ++r)
        {
            ClipCell::Callbacks cellCbs;
            cellCbs.onTrigger = [this] (int t, int rr) { if (cbs_.onCellTrigger) cbs_.onCellTrigger (t, rr); };
            cellCbs.onTriggerImmediate = [this] (int t, int rr) { if (cbs_.onCellTriggerImmediate) cbs_.onCellTriggerImmediate (t, rr); };
            cellCbs.onRelease = [this] (int t, int rr) { if (cbs_.onCellRelease) cbs_.onCellRelease (t, rr); };
            cellCbs.onCreatePlaceholder = [this] (int t, int rr) { if (cbs_.onCellCreatePlaceholder) cbs_.onCellCreatePlaceholder (t, rr); };
            cellCbs.onDelete = [this] (int t, int rr) { if (cbs_.onCellDelete) cbs_.onCellDelete (t, rr); };

            auto cell = std::make_unique<ClipCell> (viewport_.logicalTrack (c), viewport_.logicalRow (r), cellCbs);
            addAndMakeVisible (*cell);
            column.push_back (std::move (cell));
        }
    }
}

juce::Colour GridView::colourForColumn (const Session& session, int logicalTrack) const
{
    if (logicalTrack < 0 || logicalTrack >= session.getNumTracks())
        return Theme::textDim;

    const auto argb = session.getTrack (logicalTrack).getColourARGB();
    return argb != 0 ? juce::Colour (argb) : Theme::trackColours[static_cast<size_t> (logicalTrack % 8)];
}

void GridView::refresh (const Session& session, const GridViewport& viewport, double beatsPerBar,
                         const std::vector<TrackRegistry::TrackInfo>& reaperTracks, MediaTrack* hostTrack,
                         double positionQuarterNotes)
{
    viewport_ = viewport;
    lastTotalTracks_ = session.getNumTracks();
    lastTotalRows_ = session.getNumRows();
    lastReaperTracks_ = reaperTracks;

    trackRangeLabel_.setText ("Tracks " + juce::String (viewport_.originTrack + 1) + "-"
                              + juce::String (juce::jmin (lastTotalTracks_, viewport_.originTrack + viewport_.width))
                              + " / " + juce::String (lastTotalTracks_), juce::dontSendNotification);
    rowRangeLabel_.setText ("Scenes " + juce::String (viewport_.originRow + 1) + "-"
                            + juce::String (juce::jmin (lastTotalRows_, viewport_.originRow + viewport_.height))
                            + " / " + juce::String (lastTotalRows_), juce::dontSendNotification);

    pageTracksLeft_.setEnabled (viewport_.originTrack > 0);
    pageTracksRight_.setEnabled (viewport_.originTrack + viewport_.width < lastTotalTracks_);
    pageRowsUp_.setEnabled (viewport_.originRow > 0);
    pageRowsDown_.setEnabled (viewport_.originRow + viewport_.height < lastTotalRows_);

    for (int c = 0; c < viewport_.width; ++c)
    {
        const int logicalTrack = viewport_.logicalTrack (c);
        const bool trackExists = logicalTrack >= 0 && logicalTrack < session.getNumTracks();
        auto& h = headers_[static_cast<size_t> (c)];

        if (trackExists)
        {
            const auto& track = session.getTrack (logicalTrack);
            const auto colour = colourForColumn (session, logicalTrack);

            h.nameLabel->setText (track.getName().isNotEmpty() ? track.getName() : ("Col " + juce::String (logicalTrack + 1)),
                                  juce::dontSendNotification);
            h.nameLabel->setColour (juce::Label::textColourId, colour);

            // JUCE ComboBox reserves id 0 for "nothing selected" and wants
            // positive ids, so "No routing" gets id 1 and real tracks start
            // at i+2 (decoded in the onChange handler above). Rebuilt from
            // scratch every refresh (the REAPER track list can change at
            // any time), so the previously-selected routing must be
            // re-established afterwards or the combo would silently revert
            // to showing nothing selected each time.
            h.routingCombo->setEnabled (true);
            h.routingCombo->clear (juce::dontSendNotification);
            h.routingCombo->addItem ("No routing", 1);
            int selectedId = 1;
            for (size_t i = 0; i < reaperTracks.size(); ++i)
            {
                const auto& rt = reaperTracks[i];
                const juce::String label = (rt.track == hostTrack) ? (rt.name + " (this plugin's track)") : rt.name;
                const int id = static_cast<int> (i) + 2;
                h.routingCombo->addItem (label.isNotEmpty() ? label : "(unnamed)", id);
                if (h.currentDest != nullptr && rt.track == h.currentDest)
                    selectedId = id;
            }
            h.routingCombo->setSelectedId (selectedId, juce::dontSendNotification);

            h.stopButton->setEnabled (true);
            h.stopButton->setColour (juce::TextButton::buttonColourId,
                                     track.getActiveSlot() >= 0 ? colour.withAlpha (0.55f) : Theme::btnNormal);
        }
        else
        {
            h.nameLabel->setText ("-", juce::dontSendNotification);
            h.nameLabel->setColour (juce::Label::textColourId, Theme::textDim);
            h.routingCombo->setEnabled (false);
            h.stopButton->setEnabled (false);
        }
    }

    for (int r = 0; r < viewport_.height; ++r)
    {
        const int logicalRow = viewport_.logicalRow (r);
        bool anyActive = false;
        bool anyOccupied = false;

        for (int c = 0; c < viewport_.width; ++c)
        {
            const int logicalTrack = viewport_.logicalTrack (c);
            if (logicalTrack < 0 || logicalTrack >= session.getNumTracks())
                continue;
            const auto& track = session.getTrack (logicalTrack);
            if (track.getActiveSlot() == logicalRow) anyActive = true;
            if (track.hasClip (logicalRow)) anyOccupied = true;
        }

        auto& btn = *sceneButtons_[static_cast<size_t> (r)];
        btn.setButtonText ("Scene " + juce::String (logicalRow + 1));
        btn.setColour (juce::TextButton::buttonColourId,
                       anyActive ? Theme::playGreen.withAlpha (0.55f) : (anyOccupied ? Theme::btnHover : Theme::btnNormal));
    }

    for (int c = 0; c < viewport_.width; ++c)
    {
        const int logicalTrack = viewport_.logicalTrack (c);
        const bool trackExists = logicalTrack >= 0 && logicalTrack < session.getNumTracks();
        const auto colour = colourForColumn (session, logicalTrack);

        for (int r = 0; r < viewport_.height; ++r)
        {
            const int logicalRow = viewport_.logicalRow (r);
            auto& cell = *cells_[static_cast<size_t> (c)][static_cast<size_t> (r)];
            cell.setTrackIndex (logicalTrack);
            cell.setRowIndex (logicalRow);

            ClipCell::State state;
            double progress = 0.0;

            if (trackExists)
            {
                const auto& track = session.getTrack (logicalTrack);
                state.hasClip = track.hasClip (logicalRow);
                state.isActive = track.getActiveSlot() == logicalRow;
                state.isStopQueued = state.isActive && track.isStopQueued();
                state.isQueued = track.getQueuedSlot() == logicalRow;

                if (state.hasClip)
                {
                    state.lengthBeats = track.getClip (logicalRow)->getLengthBeats();
                    if (state.isActive && state.lengthBeats > 0.0)
                        progress = std::fmod (juce::jmax (0.0, positionQuarterNotes), state.lengthBeats) / state.lengthBeats;
                }
            }

            cell.setState (state, colour, beatsPerBar, progress);
        }
    }
}

void GridView::resized()
{
    auto bounds = getLocalBounds();

    auto topStrip = bounds.removeFromTop (kTopStripHeight);
    auto leftHalf = topStrip.removeFromLeft (topStrip.getWidth() / 2);
    pageTracksLeft_.setBounds (leftHalf.removeFromLeft (28).reduced (2));
    trackRangeLabel_.setBounds (leftHalf.removeFromLeft (130));
    pageTracksRight_.setBounds (leftHalf.removeFromLeft (28).reduced (2));
    addTrackButton_.setBounds (leftHalf.removeFromLeft (84).reduced (2));

    pageRowsUp_.setBounds (topStrip.removeFromLeft (28).reduced (2));
    rowRangeLabel_.setBounds (topStrip.removeFromLeft (130));
    pageRowsDown_.setBounds (topStrip.removeFromLeft (28).reduced (2));
    addSceneButton_.setBounds (topStrip.removeFromLeft (84).reduced (2));

    auto gridArea = bounds.reduced (4);
    auto sceneRailColumn = gridArea.removeFromLeft (kSceneRailWidth);
    sceneRailColumn.removeFromTop (kHeaderHeight); // blank corner cell
    auto headerRowArea = gridArea.removeFromTop (kHeaderHeight);

    const int numCols = juce::jmax (1, viewport_.width);
    const int numRows = juce::jmax (1, viewport_.height);
    const int cellW = headerRowArea.getWidth() / numCols;
    const int cellH = sceneRailColumn.getHeight() / numRows;

    for (int c = 0; c < viewport_.width; ++c)
    {
        auto cellBounds = headerRowArea.removeFromLeft (cellW).reduced (2);
        auto& h = headers_[static_cast<size_t> (c)];
        h.nameLabel->setBounds (cellBounds.removeFromTop (18));
        h.routingCombo->setBounds (cellBounds.removeFromTop (22));
        cellBounds.removeFromTop (2);
        h.stopButton->setBounds (cellBounds.removeFromTop (18));
    }

    for (int r = 0; r < viewport_.height; ++r)
        sceneButtons_[static_cast<size_t> (r)]->setBounds (sceneRailColumn.removeFromTop (cellH).reduced (2));

    for (int c = 0; c < viewport_.width; ++c)
    {
        auto colBounds = gridArea.removeFromLeft (cellW);
        for (int r = 0; r < viewport_.height; ++r)
            cells_[static_cast<size_t> (c)][static_cast<size_t> (r)]->setBounds (colBounds.removeFromTop (cellH).reduced (2));
    }
}

void GridView::paint (juce::Graphics& g)
{
    g.fillAll (Theme::backgroundDeep);
    auto area = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (Theme::glass.withAlpha (0.35f));
    g.fillRoundedRectangle (area, Theme::cornerLarge);
    g.setColour (Theme::borderHi.withAlpha (0.4f));
    g.drawRoundedRectangle (area, Theme::cornerLarge, 1.0f);
}

} // namespace reaperlooper
