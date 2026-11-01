# ReaperLooper

A REAPER-native, clip-launching looper, built as VST3 that talks 
to REAPER through its full C++ extension API (not a sandboxed subset). 
Instruments and effects stay on real REAPER tracks, the plugin routes MIDI into them.

## Status: Phase 0 (skeleton) + Phase 1 (core engine) complete

**Phase 0** — a minimal MIDI-effect VST3 that:
- Acquires REAPER's C++ extension API via `IReaperHostApplication::getReaperApi`
  (see `source/reaper/ReaperApi.{h,cpp}`) — the same mechanism a native
  REAPER extension uses, reached from inside a plugin instance instead.
- Shows live connection status and the real project's track list in its
  editor, proving the bridge end-to-end. **Confirmed working against a real
  REAPER project.**

**Phase 1** — the framework-agnostic looper engine (`source/core/`), not yet
wired into the plugin's UI or audio path:
- Dynamic `Session`/`Track`/`Clip` grid model — not a fixed 8x8 like
  `sequenced`; tracks and scenes grow via `Session::addTrack()`/`addScene()`.
- Host-bar-grid-relative launch quantization (`LaunchQuantizer`), resolved
  sample-accurately within a block (`Track::processBlock` splits the block
  at the exact sample a boundary falls on, not just at block granularity).
- Independent per-clip loop phase, no session-wide master loop
  (`LoopSegmenter` handles mid-block loop-boundary crossings).
- Four launch modes (Toggle/Trigger/Gate/Repeat), Ableton-style scene
  launch, Legato (phase carries over when switching clips), per-clip Follow
  Actions, and Capture (retroactively turn recent live input into a clip).
- `ValueTree`-based serialization (`ProjectSerializer`), mirroring
  `sequenced`'s pattern.
- 38 Catch2 tests covering all of the above (`ctest --test-dir build`).

**Known Phase 1 limitations** (by design, to keep scope bounded - see the
plan for the reasoning):
- Tracks and scenes can be added and grown, but not removed or reordered.
- Multi-bar quantize/follow-action targets beyond the current bar assume
  future bars are the same length as the current one; this self-corrects
  every block as fresh transport data arrives, so it's only approximate
  when a tempo/time-signature change falls within the lookahead window.
- At most one launch/stop boundary resolves per audio block; a Repeat
  retrigger interval shorter than one block will have later retriggers
  land on the following block(s) instead.
- Capture's loop length is an explicit parameter, not auto-detected
  (unlike Ableton's heuristic), and stopping/clearing an active clip from
  outside the audio thread (`Track::clearClip`) does not itself flush
  sounding notes - callers must stop it first.

Not yet implemented: the REAPER adapter beyond `getReaperApi` (track
creation/routing, transport bridge - Phase 2), hardware control (APC mini
mk2 - Phase 3), the grid UI (Phase 4), the dual-mode cell editor (Phase 5),
audio clips (Phase 6), and Record Performance (Phase 7).

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target ReaperLooper_VST3 -j 8
```

The VST3 is copied to `~/Library/Audio/Plug-Ins/VST3/ReaperLooper.vst3`
automatically (`COPY_PLUGIN_AFTER_BUILD TRUE`). Restart REAPER (or rescan
plugins) to pick up a new build.

**Requirements:** CMake ≥ 3.22, Xcode 15+ / Clang 14+, C++20.

## Test

```bash
cmake --build build --target ReaperLooperTests -j 8
ctest --test-dir build --output-on-failure
```

## Manual verification (do this next)

1. Open REAPER, add an audio or MIDI track.
2. Insert ReaperLooper as a track FX (MIDI effect).
3. Open its editor — it should read **"REAPER API: connected"** and list
   every track in the current project by name.
4. In a non-REAPER host (or the Standalone build, also produced by the
   `ReaperLooper_VST3`/`_Standalone` targets), it should read **"Not
   connected to REAPER"** — that's expected, not a bug.

## Project layout

- `source/core/` — framework-agnostic looper engine (Phase 1+). Links only
  `juce_core`/`juce_audio_basics`/`juce_data_structures` — no REAPER API,
  no GUI, fully unit-testable without a running REAPER instance.
- `source/reaper/` — the REAPER adapter: API acquisition (`ReaperApi`),
  and (from Phase 2) track creation/routing, transport/tempo bridging, undo
  blocks. `source/reaper/sdk/` vendors the REAPER SDK headers needed
  (`reaper_plugin_functions.h`, `reaper_vst3_interfaces.h`) plus
  `ReaperForwardDecls.h`, a minimal stand-in for the real `reaper_plugin.h`
  that avoids pulling in WDL/SWELL (not needed for the VST3-via-`getReaperApi`
  approach this project uses — see that file's comment for why).
- `source/plugin/` — JUCE VST3 entry points.
- `source/ui/` — grid/transport UI (Phase 4+, not yet present).
- `tests/` — Catch2, mirrors `source/`.
