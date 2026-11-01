#include "LaunchpadController.h"

namespace reaperlooper
{

LaunchpadController::LaunchpadController()
{
    inputCollector_.reset (44100.0); // updated properly in prepare()

    for (auto& col : lastLedState_)
        col.fill (0xFF); // forces a full LED refresh on first sendLedUpdates()
    lastSceneLedState_.fill (0xFF);
    lastTrackButtonLedState_.fill (0xFF);
}

LaunchpadController::~LaunchpadController()
{
    clearAllLeds();
    closeInput();
    closeOutput();
}

//==============================================================================
// Configuration

void LaunchpadController::prepare (double sampleRate)
{
    inputCollector_.reset (sampleRate);
}

void LaunchpadController::setDeviceType (LaunchpadDeviceType type)
{
    if (profile_.type == type)
        return;

    profile_ = LaunchpadProfile::forDevice (type);

    for (auto& col : lastLedState_)
        col.fill (0xFF);
    lastSceneLedState_.fill (0xFF);
    lastTrackButtonLedState_.fill (0xFF);

    if (inputDevice_)
        openInput(); // protocol may have changed - re-open under the new profile

    if (outputDevice_)
        sendInit();
}

void LaunchpadController::setInputDeviceId (const juce::String& deviceId)
{
    if (inputDeviceId_ == deviceId)
        return;
    inputDeviceId_ = deviceId;
    closeInput();
    if (deviceId.isNotEmpty())
        openInput();
}

void LaunchpadController::setOutputDeviceId (const juce::String& deviceId)
{
    if (outputDeviceId_ == deviceId)
        return;
    outputDeviceId_ = deviceId;
    closeOutput();
    if (deviceId.isNotEmpty())
    {
        openOutput();
        sendInit();
    }
}

//==============================================================================
// Device open/close

void LaunchpadController::openInput()
{
    closeInput();
    if (inputDeviceId_.isEmpty() || profile_.type == LaunchpadDeviceType::None)
        return;

    if (auto dev = juce::MidiInput::openDevice (inputDeviceId_, this))
    {
        dev->start();
        inputDevice_ = std::move (dev);
    }
}

void LaunchpadController::closeInput()
{
    if (inputDevice_ != nullptr)
    {
        inputDevice_->stop();
        inputDevice_.reset();
    }
}

void LaunchpadController::openOutput()
{
    closeOutput();
    if (outputDeviceId_.isEmpty() || profile_.type == LaunchpadDeviceType::None)
        return;

    outputDevice_ = juce::MidiOutput::openDevice (outputDeviceId_);
}

void LaunchpadController::closeOutput()
{
    outputDevice_.reset();
}

void LaunchpadController::clearAllLeds()
{
    if (profile_.type == LaunchpadDeviceType::None || outputDevice_ == nullptr)
        return;

    const GridViewport physical; // default {0,0,8,8} - both devices' physical surface, independent of any logical viewport

    for (int c = 0; c < physical.width; ++c)
        for (int r = 0; r < physical.height; ++r)
        {
            const int padNote = profile_.gridToNote (c, r);
            if (padNote >= 0)
                sendOutputMessage (profile_.makeLedMessage (padNote, LedState::Off, c));
        }

    for (int r = 0; r < physical.height; ++r)
    {
        const int sceneNote = profile_.sceneRowToNote (r);
        if (sceneNote >= 0)
            sendOutputMessage (profile_.makeSceneLedMessage (sceneNote, false, false));
    }

    for (int c = 0; c < physical.width; ++c)
    {
        const int note = profile_.trackButtonToNote (c);
        if (note >= 0)
            sendOutputMessage (profile_.makeTrackButtonLedMessage (note, false));
    }

    for (auto& col : lastLedState_)
        col.fill (0xFF);
    lastSceneLedState_.fill (0xFF);
    lastTrackButtonLedState_.fill (0xFF);
}

void LaunchpadController::sendInit()
{
    for (const auto& msg : profile_.makeInitMessages())
        sendOutputMessage (msg);
}

void LaunchpadController::sendOutputMessage (const juce::MidiMessage& msg)
{
    const juce::ScopedLock sl (outputLock_);
    if (outputDevice_ != nullptr)
        outputDevice_->sendMessageNow (msg);
}

//==============================================================================
// juce::MidiInputCallback - MIDI thread

void LaunchpadController::handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& msg)
{
    inputCollector_.addMessageToQueue (msg);
}

//==============================================================================
// Audio thread

void LaunchpadController::processBlock (Session& session, const GridViewport& viewport,
                                         const HostTransportState& transport, int numSamples,
                                         std::vector<juce::MidiBuffer>& trackOutputs)
{
    if (profile_.type == LaunchpadDeviceType::None)
        return;

    if (trackOutputs.size() != static_cast<size_t> (session.getNumTracks()))
        trackOutputs.resize (static_cast<size_t> (session.getNumTracks()));

    juce::MidiBuffer buf;
    inputCollector_.removeNextBlockOfMessages (buf, numSamples);

    for (const auto meta : buf)
    {
        const auto& msg = meta.getMessage();
        if (! profile_.isValidInputNoteOnOrOff (msg))
            continue;

        const int note = msg.getNoteNumber();

        // Scene launch buttons first (own note range on both devices).
        if (const auto sceneRow = profile_.noteToSceneRow (note); sceneRow.has_value())
        {
            if (msg.isNoteOn (false))
                session.handleSceneLaunch (*sceneRow, transport);
            continue;
        }

        // Track buttons (APC Mini Mk2 only): stop that column.
        if (const auto col = profile_.noteToTrackButton (note); col.has_value())
        {
            const int logicalTrack = viewport.logicalTrack (*col);
            if (msg.isNoteOn (false) && logicalTrack >= 0 && logicalTrack < session.getNumTracks())
                session.getTrack (logicalTrack).queueStopAll (transport);
            continue;
        }

        // Regular grid pads - needs note-on AND note-off for Gate/Repeat modes.
        if (const auto coord = profile_.noteToGrid (note); coord.has_value())
        {
            const int logicalTrack = viewport.logicalTrack (coord->col);
            const int logicalRow = viewport.logicalRow (coord->row);
            if (logicalTrack < 0 || logicalTrack >= session.getNumTracks())
                continue;

            auto& track = session.getTrack (logicalTrack);
            auto& output = trackOutputs[static_cast<size_t> (logicalTrack)];

            if (msg.isNoteOn (false))
                track.handlePadDown (logicalRow, transport, output);
            else if (msg.isNoteOff())
                track.handlePadUp (logicalRow, transport, output);
        }
    }
}

//==============================================================================
// Message thread - LED updates

void LaunchpadController::sendLedUpdates (const Session& session, const GridViewport& viewport)
{
    if (profile_.type == LaunchpadDeviceType::None || outputDevice_ == nullptr)
        return;

    // ---- Grid pad LEDs ----
    for (int c = 0; c < viewport.width; ++c)
    {
        const int logicalTrack = viewport.logicalTrack (c);
        const bool trackExists = logicalTrack >= 0 && logicalTrack < session.getNumTracks();

        for (int r = 0; r < viewport.height; ++r)
        {
            LedState state = LedState::Off;

            if (trackExists)
            {
                const int logicalRow = viewport.logicalRow (r);
                const auto& track = session.getTrack (logicalTrack);
                const int activeSlot = track.getActiveSlot();
                const int queuedSlot = track.getQueuedSlot();

                if (activeSlot == logicalRow && track.isStopQueued())
                    state = LedState::StopQueued;
                else if (activeSlot == logicalRow)
                    state = LedState::Active;
                else if (queuedSlot == logicalRow)
                    state = LedState::Queued;
                else if (track.hasClip (logicalRow))
                    state = LedState::OccupiedIdle;
            }

            const auto stateU8 = static_cast<uint8_t> (state);
            if (lastLedState_[static_cast<size_t> (c)][static_cast<size_t> (r)] == stateU8)
                continue;
            lastLedState_[static_cast<size_t> (c)][static_cast<size_t> (r)] = stateU8;

            const int padNote = profile_.gridToNote (c, r);
            if (padNote >= 0)
                sendOutputMessage (profile_.makeLedMessage (padNote, state, logicalTrack % 8));
        }
    }

    // ---- Scene launch button LEDs ----
    for (int r = 0; r < viewport.height; ++r)
    {
        const int logicalRow = viewport.logicalRow (r);
        bool anyActive = false;
        bool anyOccupied = false;

        for (int c = 0; c < viewport.width; ++c)
        {
            const int logicalTrack = viewport.logicalTrack (c);
            if (logicalTrack < 0 || logicalTrack >= session.getNumTracks())
                continue;

            const auto& track = session.getTrack (logicalTrack);
            if (track.getActiveSlot() == logicalRow) anyActive = true;
            if (track.hasClip (logicalRow)) anyOccupied = true;
        }

        const uint8_t sceneState = anyActive ? 2u : (anyOccupied ? 1u : 0u);
        if (lastSceneLedState_[static_cast<size_t> (r)] == sceneState)
            continue;
        lastSceneLedState_[static_cast<size_t> (r)] = sceneState;

        const int sceneNote = profile_.sceneRowToNote (r);
        if (sceneNote >= 0)
            sendOutputMessage (profile_.makeSceneLedMessage (sceneNote, anyActive, anyOccupied));
    }

    // ---- Track button LEDs (ClipStop only - lit while the column has anything active) ----
    for (int c = 0; c < viewport.width; ++c)
    {
        const int note = profile_.trackButtonToNote (c);
        if (note < 0)
            continue;

        const int logicalTrack = viewport.logicalTrack (c);
        const bool lit = logicalTrack >= 0 && logicalTrack < session.getNumTracks()
                        && session.getTrack (logicalTrack).getActiveSlot() >= 0;

        const uint8_t newState = lit ? 1u : 0u;
        if (lastTrackButtonLedState_[static_cast<size_t> (c)] == newState)
            continue;
        lastTrackButtonLedState_[static_cast<size_t> (c)] = newState;
        sendOutputMessage (profile_.makeTrackButtonLedMessage (note, lit));
    }
}

} // namespace reaperlooper
