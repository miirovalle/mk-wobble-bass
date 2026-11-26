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
| `Source/DSP/` | Portable engine: parameters, wavetables, oscillators, filters, LFOs, envelopes, distortion, effects, voice, engine, presets |
| `Source/Plugin/` | JUCE wrapper: APVTS parameter layout, MIDI, host tempo, state, presets, editor |
| `Source/UI/` | Theme, touch knobs, animated wobble/scope display, preset browser, pages |
| `Tests/CoreTests.cpp` | Headless DSP tests (tuning, wobble sync, filters, distortion, 56 presets) |
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
| 1 voice, distortion | 0.36 % |
| 8 voices, all effects | 1.86 % |
| 8 voices, neuro preset | 2.53 % |

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
