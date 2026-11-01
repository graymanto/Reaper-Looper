#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <optional>
#include <vector>
#include <cstdint>

namespace reaperlooper
{

/**
 * Which hardware grid controller is active.
 *
 * Phase 3 only fully implements ApcMiniMk2 (the hardware actually owned and
 * tested against). LaunchpadMiniMk3's mapping/LED logic is ported here too
 * (mechanical, low-risk - it's unchanged protocol knowledge) so the
 * architecture already supports more than one device, but it has not been
 * verified against real hardware and its Queued/StopQueued states use
 * static colour rather than the Mk3's native flash/pulse note modes - see
 * Phase 8 in the roadmap.
 */
enum class LaunchpadDeviceType
{
    None,
    ApcMiniMk2,       ///< Akai APC Mini Mk2 (8x8 RGB grid).
    LaunchpadMiniMk3  ///< Novation Launchpad Mini Mk3 (8x8, programmer mode). Unverified - see above.
};

/** Visual state of a single pad, drives LED colour/brightness. */
enum class LedState
{
    Off,           ///< No clip present.
    OccupiedIdle,  ///< Clip exists but not playing or queued.
    Queued,        ///< Clip is queued to start on the next quantize boundary.
    StopQueued,    ///< Playing clip is queued to stop.
    Active         ///< Clip is actively playing.
};

/** A pad's position on the hardware's own fixed physical grid (not a logical Session coordinate - see GridViewport). */
struct GridCoord
{
    int col { 0 }; ///< Physical column (0 = leftmost).
    int row { 0 };  ///< Physical row (0 = topmost).
};

// ---------------------------------------------------------------------------

/**
 * Pure-data description of a hardware grid controller's MIDI mapping.
 *
 * No device I/O here - this is only mapping + LED message construction,
 * which makes it fully testable without any hardware. Ported from
 * `sequenced`'s LaunchpadProfile with two correctness fixes applied (see
 * the project plan for the full writeup):
 *  - the apcNoteToGrid() doc comment previously described the wrong pad
 *    origin (top-left) when the implementation (correctly) uses Akai's
 *    actual bottom-left origin - the comment is now accurate.
 *  - StopQueued previously used MIDI channel 0x99, which per Akai's own
 *    spec is "pulsing 1/4", not a blink (blink channels are 0x9B-0x9F);
 *    it's visually too similar to Queued's pulse. Now uses a genuine
 *    blink channel.
 */
struct LaunchpadProfile
{
    static LaunchpadProfile forDevice (LaunchpadDeviceType type);

    //==============================================================================
    // Grid <-> MIDI note mapping

    /** The grid coordinate for a MIDI note-on, or nullopt if the note isn't a grid pad (e.g. it's a side button). */
    std::optional<GridCoord> noteToGrid (int note) const;

    /** The MIDI note number for a given physical grid position. */
    int gridToNote (int col, int row) const;

    /** MIDI channel (0-indexed) grid pads arrive on. Not currently needed for filtering - see isValidInputNoteOn(). */
    int inputChannel() const;

    /** True if `msg` is a note-on/off that should be processed as a pad or button for this device. */
    bool isValidInputNoteOnOrOff (const juce::MidiMessage& msg) const;

    //==============================================================================
    // LED messages

    /** Builds a MIDI message (or SysEx) to set the LED for a pad, given its state and logical-track-derived colour index (0-7, wraps). May return an invalid message if the device is None. */
    juce::MidiMessage makeLedMessage (int padNote, LedState state, int colourIndex) const;

    /** Messages to send once, right after connecting (e.g. the Launchpad Mini Mk3 needs to enter "programmer mode"). */
    std::vector<juce::MidiMessage> makeInitMessages() const;

    /** The scene/launch-row index (0 = top) for a note, or nullopt if it's a grid pad or unrelated button. */
    std::optional<int> noteToSceneRow (int note) const;

    /** The MIDI note for the scene launch button at physical row `row`. -1 for DeviceType::None. */
    int sceneRowToNote (int row) const;

    /** LED message for a scene launch button. `active` = at least one clip in the row is playing; `occupied` = at least one clip exists in the row. */
    juce::MidiMessage makeSceneLedMessage (int sceneNote, bool active, bool occupied) const;

    /** The physical-column track-button index (0-7) for a note, or nullopt. APC Mini Mk2: notes 100-107. Launchpad Mini Mk3: none. */
    std::optional<int> noteToTrackButton (int note) const;

    /** The MIDI note for the track button at physical column `col`. -1 if not applicable. */
    int trackButtonToNote (int col) const;

    /** LED message for a track button (red single-colour on APC Mini Mk2; no-op elsewhere). */
    juce::MidiMessage makeTrackButtonLedMessage (int note, bool lit) const;

    /** Substring of the device name as reported by the OS, for auto-detection. Empty for DeviceType::None. */
    juce::String deviceNameHint() const;

    //==============================================================================

    LaunchpadDeviceType type { LaunchpadDeviceType::None };

private:
    // ---- APC Mini Mk2 ----
    /** note = row*8 + col, counted from the physical BOTTOM-left (Akai's own origin); inverted here so row 0 (our/the app's top) maps correctly. */
    std::optional<GridCoord> apcNoteToGrid (int note) const;
    int apcGridToNote (int col, int row) const;
    juce::MidiMessage apcMakeLedMessage (int padNote, LedState state, int colourIndex) const;

    // ---- Launchpad Mini Mk3 ----
    /** note = 81 - row*10 + col (row 0 = top, col 0 = left). */
    std::optional<GridCoord> lpNoteToGrid (int note) const;
    int lpGridToNote (int col, int row) const;
    juce::MidiMessage lpMakeLedMessage (int padNote, LedState state, int colourIndex) const;
};

} // namespace reaperlooper
