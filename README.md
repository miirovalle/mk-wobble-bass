# MK WOBBLE BASS

Wobble bass synthesizer with one shared C++ DSP engine behind two products:

* **AUv3 instrument for iPadOS 17+** (A10X / 4 GB target)
* **VST3 instrument for macOS and Windows**

The engine is a dependency-free static library (`mkwobble_core`). Both plug-in
formats and the offline tests/benchmark drive exactly the same code, so sound and
parameter behaviour cannot diverge between platforms.

## Layout

| Path | Purpose |
| --- | --- |
| `Source/DSP/` | Portable engine: parameters, wavetables, oscillators, filters, LFOs, envelopes, distortion, effects, voice, engine, presets, drawable wobble shape, trance gate |
| `Source/Plugin/` | JUCE wrapper: APVTS parameter layout, MIDI, host tempo, state, presets, editor |
| `Source/UI/` | Theme, knobs, drawable LFO canvas, trance gate pattern, output scope, preset browser |
| `Tests/CoreTests.cpp` | Headless DSP tests (tuning, wobble sync, filters, distortion, 56 presets, trance gate, idle silence) |
| `Tools/Benchmark.cpp` | CPU and RAM measurement |
| `Tools/MakeThumbnail.cpp` | Renders the real editor at 2x for marketing assets |
| `Assets/` | iOS icon, thumbnail, App Store screenshot |

## Build

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 `
      -DMK_JUCE_SOURCE_DIR="C:/MUSIKA 2025/KOODI kansio/JUCE"
cmake --build build --config Release --parallel 8
```

`MK_BUILD_PLUGIN=OFF` builds only the core, tests and benchmark (no JUCE).
On iOS, configure with `-G Xcode -DCMAKE_SYSTEM_NAME=iOS` — the AUv3 extension
and the standalone container are produced from the same target.

## CI (GitHub Actions)

Two workflows under `.github/workflows/` build every product on GitHub runners.
No Apple certificates are needed for either one.

| Workflow | Produces | Runner |
| --- | --- | --- |
| `build-vst3.yml` | VST3 + Standalone (Windows x64), VST3 + Audio Unit (macOS universal `arm64;x86_64`), DSP tests (Linux) | `windows-latest`, `macos-15`, `ubuntu-latest` |
| `ios-auv3.yml` | `MK-WOBBLE-BASS-<version>-ios.ipa` (container app with the AUv3 `.appex` embedded) and a standalone `.appex.zip` | `macos-15` |

Each run uploads its builds as workflow artifacts (90 days). Pushing a `v*` tag
also attaches the same files to that GitHub release. JUCE is cloned once per tag
into `external/JUCE` and cached, so the framework is not re-downloaded every run.

### Signed iPadOS builds

`ios-auv3.yml` produces an **unsigned** IPA unless these repository secrets
exist. Unsigned is what AltStore, Sideloadly and TrollStore expect; they re-sign
on install.

| Secret | Value |
| --- | --- |
| `APPLE_TEAM_ID` | Team id, e.g. `ABCDE12345` |
| `APPLE_CERT_P12_BASE64` | base64 of an Apple Development or Distribution `.p12` |
| `APPLE_CERT_PASSWORD` | password of that `.p12` |
| `APPLE_API_KEY_P8_BASE64` | base64 of an App Store Connect API key (`.p8`) |
| `APPLE_API_KEY_ID` | 10-character key id |
| `APPLE_API_ISSUER_ID` | issuer UUID of that key |

With the secrets in place the job builds with automatic signing
(`-allowProvisioningUpdates`), so Xcode provisions both bundle ids itself: the
container (`audio.mk.wobblebass`) and the extension
(`audio.mk.wobblebass.wobblebassAUv3`). Run the workflow manually and set
`signing` to `unsigned` to force a sideload build even when the secrets exist.

The job fails fast if the produced `.appex` is missing from `Payload/…/PlugIns`
or does not declare the `com.apple.AudioUnit-UI` extension point, so a broken
IPA never reaches the artifacts.

## The interface

Two tabs, a handful of controls, nothing else. The output waveform sits under the
header on both tabs.

**WOBBLE tab**

| Element | What it does |
| --- | --- |
| **DRAW THE WOBBLE** | Draw the wobble shape with one finger; the playhead shows where the wobble is now |
| **SINE / CLEAR** | Puts a sine back on the canvas, or flattens it |
| **RATE** | Wobble speed in Hz. Turning it always switches the wobble to its own clock, so the knob is never dead |
| **DEPTH** | How far the wobble swings |
| **WOBBLE TYPE** | What the wobble moves: FILTER, RESO, PITCH, WAVE, DIRT, WIDTH, FORMANT, FM |

Drawing on the canvas switches the engine to the drawn curve, so what you see is
exactly what modulates the sound. The curve is saved with the plug-in state.

**GATE tab**

| Element | What it does |
| --- | --- |
| **TRANCE GATE** | 64 steps in four rows of sixteen: tap or drag to open and close them. The outline follows the playhead and the bar at the bottom shows the live gain |
| **< 1/100 name >** | Steps through the 100 built-in rhythms (`Source/DSP/GatePresets.cpp`); each one also sets its rate, depth and smoothing |
| **RATE** | 1/32, 1/16, 1/8 or 1/4, always locked to the host tempo |
| **DEPTH** | How far closed steps go: 0 % leaves the signal untouched, 100 % is full silence |
| **SMOOTH** | Attack/release of the gate: 0 % is a hard chop, 100 % is a soft pump |
| **GATE ON** | Bypasses the gate while keeping the pattern |

The gate is an original, self-contained implementation
([TranceGate.h](Source/DSP/TranceGate.h)): no Gate12 or other third-party gating
code is compiled into this plug-in, so nothing here changes the licensing of the
project. It runs between the voices and the master effects, so reverb and delay
tails keep ringing through the gaps.

## Test and validate

```powershell
.\build\Release\mk_wobble_tests.exe
.\build\MKPluginSmokeTest_artefacts\Release\MKPluginSmokeTest.exe
.\build\Release\mk_wobble_bench.exe
pluginval.exe --strictness-level 8 --validate-in-process --skip-gui-tests `
    "build\MKWobbleBass_artefacts\Release\VST3\MK WOBBLE BASS.vst3"
```

The smoke test drives the real processor (MIDI, automation, on-screen keyboard,
preset loading, state round-trip) and writes off-screen PNG snapshots of every
UI page.

## iOS assets

| File | Use |
| --- | --- |
| `Assets/AppIcon-1024.png` | App icon (`ICON_BIG`, iPadOS 17 single-size) |
| `Assets/iOS-Thumbnail-1024.png` | Square store tile / documentation thumbnail |
| `Assets/iOS-Screenshot-2732x2048.png` | App Store iPad screenshot (landscape) |
| `Assets/ui-render@2x.png` | Raw 2x editor render used by the composer |

Regenerate:

```powershell
python Tools\make_app_icon.py 1024      # icon
cmake --build build --config Release --target MKThumbnail
.\build\MKThumbnail_artefacts\Release\MKThumbnail.exe
python Tools\make_thumbnails.py         # thumbnail + store screenshot
```

## Performance (measured, Release, 48 kHz, 128-sample blocks)

| Scenario | % of one core |
| --- | --- |
| 1 voice, no effects | 0.24 % |
| 1 voice, distortion | 0.31 % |
| 1 voice, all effects | 0.49 % |
| 8 voices, all effects | 1.84 % |
| 8 voices, neuro preset | 2.57 % |

Idle costs essentially nothing: with no voice active and the effect tails
decayed, the whole master chain is skipped, and any non-finite sample resets the
DSP state instead of turning into noise.

Static footprint: 262 KB wavetable banks (procedural, 8 banks x 8 frames x
9 mips) + ~710 KB engine buffers (16 voices, comb lines, delay, reverb).

How the budget is kept:

* modulation, filter coefficients, pitch and wavetable position update once per
  32-sample control block; only oscillators, filters, envelopes and gain run per sample
* band-limited PolyBLEP oscillators and mip-mapped wavetables (no oversampling
  unless requested)
* inactive oscillators, filters, distortion and effects are skipped entirely
* fixed-size voice pool with stealing plus an adaptive voice limiter that backs
  off when a block overruns 80 % of its budget
* no allocation, locking or file I/O on the audio thread

## Playing from the plug-in window

The bottom strip is a real keyboard covering C-2 to C3 (MIDI 0-60), the register
this instrument lives in. Click or tap keys, or click the strip once and play
with the computer keyboard. Notes go through `MidiKeyboardState`, so hosts
record them like any other MIDI input.
