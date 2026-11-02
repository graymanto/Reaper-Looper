# ReaperLooper

A REAPER-native, Ableton Session View / Logic Live Loops-style clip-launching
looper, with hardware grid controller support (starting with the Akai APC
mini mk2). Built as a JUCE VST3 plugin that talks to REAPER through its
full C++ extension API — not a sandboxed subset — so instruments and
effects stay on real REAPER tracks and the looper routes MIDI into them,
rather than maintaining its own shadow mixer.

**Status: early, active development.** The engine, REAPER integration,
hardware control, and a working grid UI all exist and are tested — but
cell editing is still a placeholder (see [Roadmap](#roadmap)) and audio
clips/Record Performance aren't built yet. Not ready for end users.

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
- **A real grid UI.** The clip grid, track headers with a REAPER-track
  routing picker and stop button, the scene-launch rail, paging/add
  controls for growing past the visible window, and a transport strip
  showing REAPER's live position/tempo. Mouse clicks and APC mini mk2 pad
  presses are fully equivalent inputs — both go through the same engine
  calls. Cell content is still a placeholder (see below); the real editor
  is next.

## Roadmap

- [x] REAPER-native plugin shell (VST3 + full C++ API access)
- [x] Core looper engine (dynamic grid, quantized launch, scene launch,
      legato, follow actions, capture)
- [x] REAPER adapter (track creation, MIDI routing, transport/tempo sync,
      undo integration)
- [x] APC mini mk2 hardware control
- [x] The grid UI
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

Insert `ReaperLooper` as an FX on any track in REAPER (or run the
Standalone build to try the UI without REAPER - it'll just show
"not connected" where REAPER data would appear). A new instance starts
with an 8×8 grid: double-click an empty cell for a short placeholder clip
(real cell editing is next on the roadmap), click it to launch, press
Play in REAPER first if launches don't seem to resolve - quantized
launches only resolve while the transport is running. "+ Track"/"+ Scene"
grow the grid past 8×8; the paging arrows (`<` `>` `^` `v`) move the
visible window.

## Architecture

- `source/core/` — the framework-agnostic looper engine (`Session`,
  `Track`, `Clip`) and the hardware grid-controller abstraction
  (`LaunchpadProfile`, `GridViewport`). No REAPER or GUI dependency; fully
  unit-testable in isolation.
- `source/reaper/` — the REAPER integration layer: acquiring the API,
  transport/tempo-map bridging, track creation, and MIDI routing.
- `source/plugin/` — the JUCE VST3 plugin itself, `LaunchpadController`
  (opens a controller's MIDI ports, translates pad/button presses into
  engine calls, drives its LEDs), and `UiEventQueue` (the equivalent path
  for mouse input - everything that mutates the grid is applied on the
  audio thread, whether it came from hardware or the UI).
- `source/ui/` — the grid UI: `Theme`/`ReaperLooperLookAndFeel` (the
  visual language), `GridView`/`ClipCell` (the grid itself), `TransportBar`.
- `tests/` — Catch2, mirroring `source/`.

### Known limitations

- Cells can't be edited yet — double-clicking an empty one creates a fixed
  1-bar placeholder clip (one note), and that's the only way to get
  content into the grid today. Real editing (a built-in piano roll, or
  REAPER's own MIDI editor via a hidden holding track) is next.
- No density-weighted visualization on a clip's circular badge (an evenly
  spaced tick ring instead) and no bundled custom typeface (system fonts) -
  both cosmetic, deferred rather than essential.
- If two grid columns are routed to the same REAPER track, they currently
  share one REAPER send (matched by destination track, not by channel) -
  the most recently set channel wins for it. Narrow edge case, not yet
  fixed.
- A Gate-mode pad press's immediate note-off lands at the start of the
  audio block it's processed in, not its exact input sample — a minor,
  acknowledged timing approximation (quantized launches, by contrast, are
  resolved sample-accurately).
- Launchpad Mini Mk3 mapping exists in code but is unverified against real
  hardware, and its queued/stop-queued states use static colour rather
  than the Mk3's native flash/pulse modes.

## License

TBD.
