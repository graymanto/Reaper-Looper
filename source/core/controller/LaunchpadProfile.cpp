#include "LaunchpadProfile.h"

namespace reaperlooper
{

// ---------------------------------------------------------------------------
// APC Mini Mk2 colour palette
//
// The mk2 uses a fixed 128-colour palette; velocity selects the colour.
// Channel determines brightness/behaviour (from Akai's own communication
// protocol spec):
//   ch  1 (0x90): 10 % dim
//   ch  2 (0x91): 25 % dim
//   ch  7 (0x96): 100 % solid  <- primary use
//   ch  9 (0x98): pulsing 1/8   <- queued
//   ch 13 (0x9C): blinking 1/16 <- stop-queued (a genuine blink channel -
//                 the original 0x99 used here was "pulsing 1/4" per Akai's
//                 spec, not a blink, and was visually too close to Queued)
// ---------------------------------------------------------------------------
namespace
{
    // Per-column colour velocities for the APC Mini Mk2's fixed palette.
    // Same hue order Phase 4's UI theme should use for its track palette,
    // so hardware and screen agree: Red, Orange, Yellow, Lime, Cyan, Blue, Violet, Pink.
    constexpr int kApcColourVelocity[8] = { 72, 61, 74, 87, 78, 41, 67, 57 };

    // ---------------------------------------------------------------------------
    // Launchpad Mini Mk3 - SysEx RGB (0-63 per channel) per pad.
    // F0 00 20 29 02 0D 03 <note> <R> <G> <B> F7
    // ---------------------------------------------------------------------------
    struct RGB63 { uint8_t r, g, b; };

    constexpr RGB63 kLpColourRGB[8] = {
        { 63,  0,  0 }, // Red
        { 63, 35,  0 }, // Orange
        { 63, 63,  0 }, // Yellow
        { 20, 63,  0 }, // Lime
        {  0, 50, 50 }, // Cyan
        {  0, 20, 63 }, // Blue
        { 40,  0, 63 }, // Violet
        { 63, 10, 42 }, // Pink
    };

    RGB63 dimRGB (RGB63 c)
    {
        return { static_cast<uint8_t> (c.r / 5), static_cast<uint8_t> (c.g / 5), static_cast<uint8_t> (c.b / 5) };
    }

    juce::MidiMessage makeLpSysEx (int padNote, uint8_t r, uint8_t g, uint8_t b)
    {
        const uint8_t data[] = {
            0xF0, 0x00, 0x20, 0x29, 0x02, 0x0D, 0x03,
            static_cast<uint8_t> (padNote),
            static_cast<uint8_t> (r & 0x3F),
            static_cast<uint8_t> (g & 0x3F),
            static_cast<uint8_t> (b & 0x3F),
            0xF7
        };
        return juce::MidiMessage (data, static_cast<int> (sizeof (data)));
    }
}

//==============================================================================
LaunchpadProfile LaunchpadProfile::forDevice (LaunchpadDeviceType type)
{
    LaunchpadProfile p;
    p.type = type;
    return p;
}

//==============================================================================
// APC Mini Mk2

std::optional<GridCoord> LaunchpadProfile::apcNoteToGrid (int note) const
{
    if (note < 0 || note > 63)
        return std::nullopt;

    // Akai's own numbering has note 0 at the physical BOTTOM-left and note
    // 56 at the physical top-left (note = deviceRow*8 + col, counted up
    // from the bottom). Our/the app's row 0 is the top, so invert.
    return GridCoord { note % 8, 7 - note / 8 };
}

int LaunchpadProfile::apcGridToNote (int col, int row) const
{
    return (7 - row) * 8 + col;
}

juce::MidiMessage LaunchpadProfile::apcMakeLedMessage (int padNote, LedState state, int colourIndex) const
{
    const int vel = (colourIndex >= 0) ? kApcColourVelocity[static_cast<size_t> (colourIndex) % 8] : 87;

    switch (state)
    {
        case LedState::Off:
            return juce::MidiMessage::noteOn (7, padNote, static_cast<juce::uint8> (0));

        case LedState::OccupiedIdle:
            return juce::MidiMessage (0x91, static_cast<juce::uint8> (padNote), static_cast<juce::uint8> (vel));

        case LedState::Queued:
            return juce::MidiMessage (0x98, static_cast<juce::uint8> (padNote), static_cast<juce::uint8> (74));

        case LedState::StopQueued:
            return juce::MidiMessage (0x9C, static_cast<juce::uint8> (padNote), static_cast<juce::uint8> (72));

        case LedState::Active:
            return juce::MidiMessage (0x96, static_cast<juce::uint8> (padNote), static_cast<juce::uint8> (vel));
    }

    return juce::MidiMessage::noteOn (7, padNote, static_cast<juce::uint8> (0));
}

//==============================================================================
// Launchpad Mini Mk3 (unverified against real hardware - see header)

std::optional<GridCoord> LaunchpadProfile::lpNoteToGrid (int note) const
{
    if (note < 11 || note > 88)
        return std::nullopt;

    const int rel = note - 11;
    const int col = rel % 10;
    const int row = rel / 10;
    if (col > 7 || row > 7)
        return std::nullopt; // side buttons / top row

    return GridCoord { col, 7 - row };
}

int LaunchpadProfile::lpGridToNote (int col, int row) const
{
    return 81 - row * 10 + col;
}

juce::MidiMessage LaunchpadProfile::lpMakeLedMessage (int padNote, LedState state, int colourIndex) const
{
    const auto c = kLpColourRGB[static_cast<size_t> (colourIndex >= 0 ? colourIndex % 8 : 0)];

    switch (state)
    {
        case LedState::Off:
            return makeLpSysEx (padNote, 0, 0, 0);

        case LedState::OccupiedIdle:
        {
            const auto dim = dimRGB (c);
            return makeLpSysEx (padNote, dim.r, dim.g, dim.b);
        }

        case LedState::Queued:
            return makeLpSysEx (padNote, 63, 63, 0); // yellow - Phase 8: use native pulse instead

        case LedState::StopQueued:
            return makeLpSysEx (padNote, 63, 0, 0);  // red - Phase 8: use native flash instead

        case LedState::Active:
            return makeLpSysEx (padNote, c.r, c.g, c.b);
    }

    return makeLpSysEx (padNote, 0, 0, 0);
}

//==============================================================================
// Dispatch

std::optional<GridCoord> LaunchpadProfile::noteToGrid (int note) const
{
    switch (type)
    {
        case LaunchpadDeviceType::ApcMiniMk2:       return apcNoteToGrid (note);
        case LaunchpadDeviceType::LaunchpadMiniMk3: return lpNoteToGrid (note);
        case LaunchpadDeviceType::None:             return std::nullopt;
    }
    return std::nullopt;
}

int LaunchpadProfile::gridToNote (int col, int row) const
{
    switch (type)
    {
        case LaunchpadDeviceType::ApcMiniMk2:       return apcGridToNote (col, row);
        case LaunchpadDeviceType::LaunchpadMiniMk3: return lpGridToNote (col, row);
        case LaunchpadDeviceType::None:             return -1;
    }
    return -1;
}

int LaunchpadProfile::inputChannel() const
{
    return 0; // both devices use MIDI channel 1 (0-indexed) for grid pads
}

bool LaunchpadProfile::isValidInputNoteOnOrOff (const juce::MidiMessage& msg) const
{
    if (! (msg.isNoteOn (false) || msg.isNoteOff()))
        return false;

    switch (type)
    {
        case LaunchpadDeviceType::ApcMiniMk2:
            // Grid pads, scene buttons and track buttons all arrive as
            // note-on/off on this dedicated device; channel varies by
            // control (see each noteTo*() above) but note ranges never
            // overlap, so no channel filtering is needed here.
            return true;

        case LaunchpadDeviceType::LaunchpadMiniMk3:
            return (msg.getChannel() - 1) == 0; // all pads/buttons send on channel 1 only

        case LaunchpadDeviceType::None:
            return false;
    }
    return false;
}

juce::MidiMessage LaunchpadProfile::makeLedMessage (int padNote, LedState state, int colourIndex) const
{
    switch (type)
    {
        case LaunchpadDeviceType::ApcMiniMk2:       return apcMakeLedMessage (padNote, state, colourIndex);
        case LaunchpadDeviceType::LaunchpadMiniMk3: return lpMakeLedMessage (padNote, state, colourIndex);
        case LaunchpadDeviceType::None:             return {};
    }
    return {};
}

std::vector<juce::MidiMessage> LaunchpadProfile::makeInitMessages() const
{
    if (type == LaunchpadDeviceType::LaunchpadMiniMk3)
    {
        const uint8_t sysex[] = { 0xF0, 0x00, 0x20, 0x29, 0x02, 0x0D, 0x0E, 0x01, 0xF7 }; // enter programmer mode
        return { juce::MidiMessage (sysex, static_cast<int> (sizeof (sysex))) };
    }
    return {}; // APC Mini Mk2 requires no init sequence
}

juce::String LaunchpadProfile::deviceNameHint() const
{
    switch (type)
    {
        case LaunchpadDeviceType::ApcMiniMk2:       return "APC Mini mk2";
        case LaunchpadDeviceType::LaunchpadMiniMk3: return "Launchpad Mini";
        case LaunchpadDeviceType::None:             return {};
    }
    return {};
}

//==============================================================================
// Scene launch buttons
//
// APC Mini Mk2:  notes 112-119, channel 1 (112 = top/row 0, 119 = bottom/row 7)
// Launchpad Mk3: right-column notes 89, 79, ..., 19 (89 = top/row 0, 19 = bottom/row 7)
//==============================================================================

std::optional<int> LaunchpadProfile::noteToSceneRow (int note) const
{
    switch (type)
    {
        case LaunchpadDeviceType::ApcMiniMk2:
            if (note >= 112 && note <= 119)
                return note - 112;
            return std::nullopt;

        case LaunchpadDeviceType::LaunchpadMiniMk3:
        {
            if (note < 19 || note > 89)
                return std::nullopt;
            const int rel = note - 19;
            if (rel % 10 != 0)
                return std::nullopt;
            const int deviceRow = rel / 10; // 0 = bottom device row, 7 = top
            return 7 - deviceRow;
        }

        case LaunchpadDeviceType::None:
            return std::nullopt;
    }
    return std::nullopt;
}

int LaunchpadProfile::sceneRowToNote (int row) const
{
    if (row < 0 || row > 7)
        return -1;

    switch (type)
    {
        case LaunchpadDeviceType::ApcMiniMk2:
            return 112 + row;

        case LaunchpadDeviceType::LaunchpadMiniMk3:
            return 19 + (7 - row) * 10;

        case LaunchpadDeviceType::None:
            return -1;
    }
    return -1;
}

juce::MidiMessage LaunchpadProfile::makeSceneLedMessage (int sceneNote, bool active, bool occupied) const
{
    switch (type)
    {
        case LaunchpadDeviceType::ApcMiniMk2:
            if (active)   return juce::MidiMessage::noteOn (1, sceneNote, static_cast<juce::uint8> (1)); // solid green
            if (occupied) return juce::MidiMessage::noteOn (1, sceneNote, static_cast<juce::uint8> (2)); // blink green
            return juce::MidiMessage::noteOn (1, sceneNote, static_cast<juce::uint8> (0));

        case LaunchpadDeviceType::LaunchpadMiniMk3:
            if (active)   return makeLpSysEx (sceneNote, 20, 63, 0);
            if (occupied) return makeLpSysEx (sceneNote, 5, 15, 0);
            return makeLpSysEx (sceneNote, 0, 0, 0);

        case LaunchpadDeviceType::None:
            return {};
    }
    return {};
}

//==============================================================================
// Track buttons
//
// APC Mini Mk2: notes 100-107 (track button 0 = note 100, ..., 7 = note 107).
// Launchpad Mini Mk3: no track buttons.
//==============================================================================

std::optional<int> LaunchpadProfile::noteToTrackButton (int note) const
{
    if (type == LaunchpadDeviceType::ApcMiniMk2 && note >= 100 && note <= 107)
        return note - 100;
    return std::nullopt;
}

int LaunchpadProfile::trackButtonToNote (int col) const
{
    if (type == LaunchpadDeviceType::ApcMiniMk2 && col >= 0 && col <= 7)
        return 100 + col;
    return -1;
}

juce::MidiMessage LaunchpadProfile::makeTrackButtonLedMessage (int note, bool lit) const
{
    if (type == LaunchpadDeviceType::ApcMiniMk2)
        return juce::MidiMessage::noteOn (1, note, lit ? static_cast<juce::uint8> (1) : static_cast<juce::uint8> (0));
    return {};
}

} // namespace reaperlooper
