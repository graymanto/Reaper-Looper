# ReaperLooper

A REAPER-native, Ableton Session View / Logic Live Loops-style clip-launching
looper, with hardware grid controller support (starting with the Akai APC
mini mk2). Built as a JUCE VST3 plugin that talks to REAPER through its
full C++ extension API — not a sandboxed subset — so instruments and
effects stay on real REAPER tracks and the looper routes MIDI into them,
rather than maintaining its own shadow mixer.

**Status: early, active development.** The core engine, REAPER integration,
and hardware control layer exist and are tested, but there is no visual
grid UI yet — see [Roadmap](#roadmap). Not ready for end users.

## Why

Most looper plugins feel like a world apart from the DAW they're loaded
into: their own mixer, their own transport, no awareness of your actual
track names or colours. ReaperLooper is built the other way around —
column routing creates real REAPER track sends, launches are quantized to
REAPER's own tempo map and bar grid, and everything (routing changes, track
creation) lands in REAPER's own undo history.

## Features so far

- **Full REAPER API access from a VST3.** Acquires REAPER's complete C++
  extension API via `IReaperHostApplication::getReaperApi` — the same
  mechanism REAPER's own native extensions use — rather than being limited
  to generic plugin parameters.
- **A dynamic clip grid engine.** Tracks and scenes grow as needed, not a
  fixed 8×8. Independent per-clip loop length and phase (no "master loop"
  forcing shorter clips silent). Host-bar-grid-relative launch
  quantization, resolved sample-accurately within an audio block.
- **Four launch modes** — Toggle, Trigger, Gate, Repeat — plus Ableton-style
  whole-row scene launch, **Legato** (a replacing clip continues from the
  old one's playback position instead of restarting), per-clip **Follow
  Actions** (auto-advance to another clip after N bars), and **Capture**
  (turn what was just played into a clip, retroactively).
- **Real REAPER track routing.** Pointing a grid column at a track creates
  an actual REAPER MIDI send, filtered to that column's channel — not an
  internal routing table.
- **APC mini mk2 support.** Full pad/scene/track-button mapping and
  diff-cached LED feedback reflecting live clip state (off / occupied /
  queued / playing / stop-queued), with correct note-on *and* note-off
  handling so Gate and Repeat launch modes work from real hardware.
- **Project save/load** via a versioned, validated serialization format.

## Roadmap

- [x] REAPER-native plugin shell (VST3 + full C++ API access)
- [x] Core looper engine (dynamic grid, quantized launch, scene launch,
      legato, follow actions, capture)
- [x] REAPER adapter (track creation, MIDI routing, transport/tempo sync,
      undo integration)
- [x] APC mini mk2 hardware control
- [ ] The grid UI
- [ ] Cell MIDI editing — a built-in piano roll, or REAPER's own MIDI
      editor via a hidden holding track, switchable
- [ ] Audio clips (tempo-synced file playback)
- [ ] Record Performance — capture a live grid performance as real MIDI
      items on the timeline, for song arrangement/prototyping
- [ ] Launchpad Mini Mk3 support
- [ ] Companion extension for a REAPER Action List entry / global hotkey

## Building

Requirements: CMake ≥ 3.22, Xcode 15+ / Clang 14+ (macOS), C++20.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target ReaperLooper_VST3 -j 8
```

The VST3 is installed to `~/Library/Audio/Plug-Ins/VST3/ReaperLooper.vst3`
automatically. Restart REAPER (or rescan plugins) to pick up a new build.

### Testing

```bash
cmake --build build --target ReaperLooperTests -j 8
ctest --test-dir build --output-on-failure
```

The engine and REAPER-adapter logic are both fully unit-tested without
needing a running REAPER instance — the adapter layer is built around a
`ReaperFunctions` function-pointer table that tests substitute with a fake
REAPER project (`tests/reaper/FakeReaper.h`).

### Trying it out

Insert `ReaperLooper` as an FX on any track in REAPER. With no grid UI yet,
its editor confirms the integration is live: REAPER connection status, the
current project's track list (names/colours, and which track hosts the
plugin instance), and whether an APC mini mk2 was found. With one
connected, press Play in REAPER, then press a lit pad — it should light
solid, and pressing it again should queue a stop on the next bar.

## Architecture

- `source/core/` — the framework-agnostic looper engine (`Session`,
  `Track`, `Clip`) and the hardware grid-controller abstraction
  (`LaunchpadProfile`, `GridViewport`). No REAPER or GUI dependency; fully
  unit-testable in isolation.
- `source/reaper/` — the REAPER integration layer: acquiring the API,
  transport/tempo-map bridging, track creation, and MIDI routing.
- `source/plugin/` — the JUCE VST3 plugin itself, and `LaunchpadController`
  (opens a controller's MIDI ports, translates pad/button presses into
  engine calls, drives its LEDs).
- `source/ui/` *(coming in a later phase)* — the grid interface.
- `tests/` — Catch2, mirroring `source/`.

### Known limitations

- A hardware controller's physical grid is fixed-size (8×8); the engine's
  logical grid can grow past that. The mapping between them
  (`GridViewport`) already supports paging, but nothing exposes scrolling
  controls yet — until the grid UI exists, only the bottom-left 8×8 of a
  larger project is reachable from hardware.
- A Gate-mode pad press's immediate note-off lands at the start of the
  audio block it's processed in, not its exact input sample — a minor,
  acknowledged timing approximation (quantized launches, by contrast, are
  resolved sample-accurately).
- Launchpad Mini Mk3 mapping exists in code but is unverified against real
  hardware, and its queued/stop-queued states use static colour rather
  than the Mk3's native flash/pulse modes.

## License

TBD.
