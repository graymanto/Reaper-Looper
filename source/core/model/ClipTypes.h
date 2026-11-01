#pragma once

namespace reaperlooper
{

/**
 * How a cell responds to pad presses, independent of the quantize value.
 * Set per-clip (Ableton-style); default is Toggle.
 */
enum class LaunchMode
{
    Toggle,   ///< Press launches; pressing the already-playing clip queues a stop.
    Trigger,  ///< Press (re-)launches from beat 0; never stops on re-press.
    Gate,     ///< Plays only while the pad is held: note-on launches immediately
              ///< (bypassing quantize), note-off stops immediately.
    Repeat    ///< While held, relaunches from beat 0 every quantize interval
              ///< (a stutter/retrigger effect). Quantize value controls the rate.
};

/** Host-bar-grid-relative launch/scene quantization (Ableton/Logic-style). */
enum class LaunchQuantize
{
    None,     ///< Resolves on the very next processed block.
    Quarter,  ///< Next quarter-note (1 QN) boundary.
    Half,     ///< Next half-note (2 QN) boundary.
    OneBar,
    TwoBars,
    FourBars,
    EightBars
};

/**
 * What a clip does after it has played for `afterBars` bars, per-clip,
 * resolved within its own track/column (Ableton Follow Actions, scoped to
 * a single track's clip slots - this project does not implement Ableton's
 * separate cross-track Follow Action Groups).
 */
enum class FollowActionType
{
    None,
    Same,     ///< Restart this same clip.
    Next,     ///< The clip in the next occupied row down (wraps).
    Previous, ///< The clip in the next occupied row up (wraps).
    First,    ///< The first occupied row in the track.
    Last,     ///< The last occupied row in the track.
    Any,      ///< A random occupied row (including this one).
    Other,    ///< A random occupied row, excluding this one.
    Specific  ///< The row named by FollowAction::targetRow.
};

/** Per-clip follow-action configuration. */
struct FollowAction
{
    FollowActionType type { FollowActionType::None };
    int afterBars { 1 };       ///< >= 1. Fires after this many bars of playback.
    int targetRow { -1 };      ///< Only used when type == Specific.
    float chance { 1.0f };     ///< 0-1. Probability the action fires at all; otherwise the clip just keeps looping.
};

} // namespace reaperlooper
