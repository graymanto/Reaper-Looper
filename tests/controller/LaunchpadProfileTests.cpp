#include <catch2/catch_all.hpp>
#include "core/controller/LaunchpadProfile.h"

using namespace reaperlooper;

//==============================================================================
// APC Mini Mk2 - the hardware this project is actually verified against.

TEST_CASE ("APC Mini Mk2: note 0 is the physical bottom-left pad", "[controller][apc]")
{
    const auto profile = LaunchpadProfile::forDevice (LaunchpadDeviceType::ApcMiniMk2);
    const auto coord = profile.noteToGrid (0);
    REQUIRE (coord.has_value());
    REQUIRE (coord->col == 0);
    REQUIRE (coord->row == 7); // our row 7 = bottom
}

TEST_CASE ("APC Mini Mk2: note 56 is the physical top-left pad", "[controller][apc]")
{
    const auto profile = LaunchpadProfile::forDevice (LaunchpadDeviceType::ApcMiniMk2);
    const auto coord = profile.noteToGrid (56);
    REQUIRE (coord.has_value());
    REQUIRE (coord->col == 0);
    REQUIRE (coord->row == 0);
}

TEST_CASE ("APC Mini Mk2: note 63 is the physical top-right pad", "[controller][apc]")
{
    const auto profile = LaunchpadProfile::forDevice (LaunchpadDeviceType::ApcMiniMk2);
    const auto coord = profile.noteToGrid (63);
    REQUIRE (coord.has_value());
    REQUIRE (coord->col == 7);
    REQUIRE (coord->row == 0);
}

TEST_CASE ("APC Mini Mk2: gridToNote is the exact inverse of noteToGrid", "[controller][apc]")
{
    const auto profile = LaunchpadProfile::forDevice (LaunchpadDeviceType::ApcMiniMk2);
    for (int note = 0; note <= 63; ++note)
    {
        const auto coord = profile.noteToGrid (note);
        REQUIRE (coord.has_value());
        REQUIRE (profile.gridToNote (coord->col, coord->row) == note);
    }
}

TEST_CASE ("APC Mini Mk2: notes outside 0-63 are not grid pads", "[controller][apc]")
{
    const auto profile = LaunchpadProfile::forDevice (LaunchpadDeviceType::ApcMiniMk2);
    REQUIRE_FALSE (profile.noteToGrid (-1).has_value());
    REQUIRE_FALSE (profile.noteToGrid (64).has_value());
}

TEST_CASE ("APC Mini Mk2: LED messages use the documented channels/velocities", "[controller][apc]")
{
    const auto profile = LaunchpadProfile::forDevice (LaunchpadDeviceType::ApcMiniMk2);
    constexpr int padNote = 10;
    constexpr int colourIndex = 0; // -> velocity 72 per kApcColourVelocity

    const auto off = profile.makeLedMessage (padNote, LedState::Off, colourIndex);
    REQUIRE (off.getChannel() == 7);
    REQUIRE (off.getVelocity() == 0);

    const auto idle = profile.makeLedMessage (padNote, LedState::OccupiedIdle, colourIndex);
    REQUIRE (idle.getRawData()[0] == 0x91);

    const auto queued = profile.makeLedMessage (padNote, LedState::Queued, colourIndex);
    REQUIRE (queued.getRawData()[0] == 0x98);
    REQUIRE (queued.getVelocity() == 74);

    const auto active = profile.makeLedMessage (padNote, LedState::Active, colourIndex);
    REQUIRE (active.getRawData()[0] == 0x96);
    REQUIRE (active.getVelocity() == 72);
}

TEST_CASE ("APC Mini Mk2: StopQueued uses a genuine blink channel, not the Queued pulse channel", "[controller][apc]")
{
    // Regression test for the fix described in LaunchpadProfile.h: the
    // original port used channel 0x99 ("pulsing 1/4" per Akai's own spec),
    // which is visually too close to Queued's 0x98 pulse and isn't a blink
    // at all. Blink channels are 0x9B-0x9F.
    const auto profile = LaunchpadProfile::forDevice (LaunchpadDeviceType::ApcMiniMk2);
    const auto stopQueued = profile.makeLedMessage (10, LedState::StopQueued, 0);

    const auto channel = static_cast<int> (stopQueued.getRawData()[0]) - 0x90;
    REQUIRE (channel >= 0x0B);
    REQUIRE (channel <= 0x0F);
    REQUIRE (channel != 0x08); // not Queued's pulse channel
    REQUIRE (channel != 0x09); // not the old, incorrect "blink" channel
}

TEST_CASE ("APC Mini Mk2: scene row <-> note mapping", "[controller][apc]")
{
    const auto profile = LaunchpadProfile::forDevice (LaunchpadDeviceType::ApcMiniMk2);

    REQUIRE (profile.noteToSceneRow (112) == 0);
    REQUIRE (profile.noteToSceneRow (119) == 7);
    REQUIRE_FALSE (profile.noteToSceneRow (111).has_value());
    REQUIRE_FALSE (profile.noteToSceneRow (120).has_value());

    for (int row = 0; row < 8; ++row)
        REQUIRE (profile.noteToSceneRow (profile.sceneRowToNote (row)) == row);
}

TEST_CASE ("APC Mini Mk2: track button <-> note mapping", "[controller][apc]")
{
    const auto profile = LaunchpadProfile::forDevice (LaunchpadDeviceType::ApcMiniMk2);

    REQUIRE (profile.noteToTrackButton (100) == 0);
    REQUIRE (profile.noteToTrackButton (107) == 7);
    REQUIRE_FALSE (profile.noteToTrackButton (99).has_value());
    REQUIRE_FALSE (profile.noteToTrackButton (108).has_value());

    for (int col = 0; col < 8; ++col)
        REQUIRE (profile.noteToTrackButton (profile.trackButtonToNote (col)) == col);
}

TEST_CASE ("APC Mini Mk2: accepts both note-on and note-off (needed for Gate/Repeat modes)", "[controller][apc]")
{
    const auto profile = LaunchpadProfile::forDevice (LaunchpadDeviceType::ApcMiniMk2);
    REQUIRE (profile.isValidInputNoteOnOrOff (juce::MidiMessage::noteOn (1, 10, (juce::uint8) 100)));
    REQUIRE (profile.isValidInputNoteOnOrOff (juce::MidiMessage::noteOff (1, 10)));
}

TEST_CASE ("APC Mini Mk2: requires no init messages; name hint matches the device", "[controller][apc]")
{
    const auto profile = LaunchpadProfile::forDevice (LaunchpadDeviceType::ApcMiniMk2);
    REQUIRE (profile.makeInitMessages().empty());
    REQUIRE (profile.deviceNameHint() == "APC Mini mk2");
}

//==============================================================================
// Launchpad Mini Mk3 - mapping logic only; not verified against real hardware.

TEST_CASE ("Launchpad Mini Mk3: note 81 is the physical top-left pad", "[controller][launchpad]")
{
    const auto profile = LaunchpadProfile::forDevice (LaunchpadDeviceType::LaunchpadMiniMk3);
    const auto coord = profile.noteToGrid (81);
    REQUIRE (coord.has_value());
    REQUIRE (coord->col == 0);
    REQUIRE (coord->row == 0);
}

TEST_CASE ("Launchpad Mini Mk3: note 11 is the physical bottom-left pad", "[controller][launchpad]")
{
    const auto profile = LaunchpadProfile::forDevice (LaunchpadDeviceType::LaunchpadMiniMk3);
    const auto coord = profile.noteToGrid (11);
    REQUIRE (coord.has_value());
    REQUIRE (coord->col == 0);
    REQUIRE (coord->row == 7);
}

TEST_CASE ("Launchpad Mini Mk3: gridToNote is the exact inverse of noteToGrid", "[controller][launchpad]")
{
    const auto profile = LaunchpadProfile::forDevice (LaunchpadDeviceType::LaunchpadMiniMk3);
    for (int col = 0; col < 8; ++col)
        for (int row = 0; row < 8; ++row)
        {
            const int note = profile.gridToNote (col, row);
            const auto coord = profile.noteToGrid (note);
            REQUIRE (coord.has_value());
            REQUIRE (coord->col == col);
            REQUIRE (coord->row == row);
        }
}

TEST_CASE ("Launchpad Mini Mk3: requires a programmer-mode init message", "[controller][launchpad]")
{
    const auto profile = LaunchpadProfile::forDevice (LaunchpadDeviceType::LaunchpadMiniMk3);
    const auto init = profile.makeInitMessages();
    REQUIRE (init.size() == 1);
    REQUIRE (init[0].isSysEx());
}

TEST_CASE ("Launchpad Mini Mk3: only accepts channel 1", "[controller][launchpad]")
{
    const auto profile = LaunchpadProfile::forDevice (LaunchpadDeviceType::LaunchpadMiniMk3);
    REQUIRE (profile.isValidInputNoteOnOrOff (juce::MidiMessage::noteOn (1, 10, (juce::uint8) 100)));
    REQUIRE_FALSE (profile.isValidInputNoteOnOrOff (juce::MidiMessage::noteOn (2, 10, (juce::uint8) 100)));
}

//==============================================================================
// DeviceType::None

TEST_CASE ("DeviceType::None is inert everywhere", "[controller]")
{
    const auto profile = LaunchpadProfile::forDevice (LaunchpadDeviceType::None);
    REQUIRE_FALSE (profile.noteToGrid (0).has_value());
    REQUIRE (profile.gridToNote (0, 0) == -1);
    REQUIRE_FALSE (profile.isValidInputNoteOnOrOff (juce::MidiMessage::noteOn (1, 10, (juce::uint8) 100)));
    REQUIRE (profile.deviceNameHint().isEmpty());
}
