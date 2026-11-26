# WOBBLE MACHINE

Professional dubstep bass synthesizer for **VST3** (Windows, macOS) and
**AUv3** (iPadOS 17+). One shared C++20 DSP engine drives every format, so the
sound and the parameter behaviour cannot diverge between platforms.

Wobble, deep sub, growl, neuro, metallic, screech, riddim, tearout, FM and Reese
bass are all built from the same voice: two wavetable/FM oscillators, an
independent sub, two multimode filters, a parallel vowel resonator, per-voice
distortion and a mastered stereo effects chain.

## Philosophy

**Professional sound. Insanely easy controls. Massive bass.**

The EASY tab is six bipolar macro knobs. The centre position of every knob
leaves the patch exactly as it was authored, so a factory preset sounds right at
a glance and still has a full range of shaping in both directions. Everything
the EASY knobs do is visible and editable on the ADVANCED tab.

## Signature features

| Feature | What it does |
| --- | --- |
| **ONE-KNOB FILTH** | GRIT drives the distortion stage, the filter drive and the mix. From the centre upwards it also switches the distortion on, so a clean patch turns filthy without opening ADVANCED. |
| **SMART WOBBLE** | The wobble is always tempo-locked when its rate selector is touched. Sixteen divisions cover 1/1 to 1/32 plus triplets and dotted rhythms; the strip under the knobs shows the live wobble. |
| **SUB LOCK** | The sub oscillator has its own clean path. With SUB LOCK on it bypasses the filter, the distortion and the stereo width, so it stays powerful and perfectly centred no matter how hard the main bass is driven. |
| **BASS MORPH** | Two bass characters (A and the MORPH TO character) are blended by the BASS MORPH knob at control rate, including the filter, drive, formant and wobble routing. |
| **RANDOM BASS** | A seeded generator that only produces playable patches: valid filter, audible wobble, bounded level. Same seed, same patch, so it is testable and repeatable. |
| **INSTANT PRESETS** | 62 factory presets in 15 categories, plus a browser for user presets, favourites, import and export. |

## The six bass characters

| Character | Sound |
| --- | --- |
| WOBBLE | Classic UK dubstep swing on top of whatever the patch already is |
| GROWL | FM body, extra resonance and tube grit |
| REESE | Wide detuned saw stack with less wobble |
| NEURO | Wavetable motion, resonance and wavefolded drive |
| SCREAM | Bright, hard-clipped screech with pitch wobble |
| TALKING | Parallel vowel resonator driven by the wobble |

Characters only shape continuous parameters. Distortion and FM are switched on
when the patch has not already chosen a mode, so ADVANCED edits are respected.

## Interface

### EASY

| Control | Notes |
| --- | --- |
| **WOBBLE** | Amount of wobble on the filter, wavetable position and resonance |
| **BASS** | Sub level, low-end weight and cutoff |
| **GROWL** | FM depth, oscillator 2 body, resonance and filter drive |
| **GRIT** | ONE-KNOB FILTH: distortion drive, mix and filter drive |
| **MOVEMENT** | LFO depth, chorus, envelope movement and wavetable drift |
| **WIDTH** | Unison spread, detune and stereo width |
| **BASS CHARACTER** | Six buttons selecting character A |
| **BASS MORPH + MORPH TO** | Blend from character A to a second character |
| **WOBBLE RATE** | 1/1, 1/2, 1/4, 1/8, 1/16, 1/32, triplets and dotted, host-synced |
| **SUB / MASTER** | Clean sub level and output gain |
| **RANDOM / PRESETS** | Header buttons, available on both tabs |

### ADVANCED

Nine sections, built from the parameter metadata so every parameter has a
control:

| Section | Contents |
| --- | --- |
| **WOBBLE** | Drawable wobble curve, rate, depth and wobble routing type |
| **OSC** | Oscillator 1, oscillator 2 (layer/FM/ring/pitch), sub, FM feedback and oscillator 3 |
| **FILTER** | Two multimode filters (LP 12/24/36, HP, BP, notch, comb, formant, ladder, diode, SVF), routing, plus the talking formant resonator and vowel |
| **ENV** | Amp, filter, pitch and mod ADSRs, voice mode, polyphony, glide, quality, oversampling |
| **LFO** | Three tempo-syncable LFO slots with ten shapes and direct targets |
| **MATRIX** | Twelve modulation slots (23 sources, 25 destinations) and the eight macro knobs |
| **DIST** | Twelve distortion algorithms, pre/post routing, multiband drive |
| **FX** | Chorus, phaser, flanger, delay, reverb, OTT-style three-band compressor, bus compressor and multiband drive |
| **GATE** | 64-step trance gate with 100 rhythms |

The output scope and the playable keyboard are visible on every tab.

## DSP engine

    MIDI -> VOICE ENGINE -> OSCILLATORS -> SUB GENERATOR -> FILTER ENGINE
         -> FORMANT / MODULATION -> DISTORTION -> EFFECTS -> MASTER OUTPUT

* Band-limited PolyBLEP oscillators and mip-mapped wavetables (8 banks x 8
  frames), unison up to seven voices, FM, phase modulation and ring modulation.
* The sub is generated separately and, with SUB LOCK, summed after the filter,
  distortion and width stage, so it is always mono and clean.
* Wobble modulates cutoff, resonance, wavetable position, FM amount, drive,
  formant position, pulse width, stereo width and oscillator phase.
* Four LFOs, four ADSR envelopes, velocity, mod wheel, aftertouch, key tracking,
  random and eight macros feed a twelve-slot matrix.
* An OTT-style three-band compressor pushes quiet detail up while holding the peaks back.
* Nonlinear stages can run at 2x oversampling; every parameter is smoothed over
  ~12 ms on the audio thread.
* DC blockers after the voice and a soft limiter on the master keep the output
  bounded; any non-finite sample resets the chain instead of turning into noise.
* Host tempo and transport are read from the playhead; delay and wobble lock to
  the host clock.

## Layout

| Path | Purpose |
| --- | --- |
| `Source/DSP/` | Portable engine: parameters, wavetables, oscillators, filters, formant, LFOs, envelopes, distortion, effects, voice, engine, presets, randomizer, wobble shape, trance gate |
| `Source/Plugin/` | JUCE wrapper: APVTS layout, MIDI, host tempo, state, presets, editor |
| `Source/UI/` | Theme, knobs, EASY page, ADVANCED sections, matrix, scope, preset browser |
| `Tests/CoreTests.cpp` | Headless DSP tests |
| `Tools/Benchmark.cpp` | CPU and RAM measurement |
| `Tools/MakeThumbnail.cpp` | Renders the real editor at 2x for marketing assets |
| `Tools/PluginSmokeTest.cpp` | Drives the real processor and renders every UI page |
| `Assets/` | iOS icon, thumbnail, App Store screenshot |

## Build

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 `
      -DMK_JUCE_SOURCE_DIR="C:/MUSIKA 2025/KOODI kansio/JUCE"
cmake --build build --config Release --parallel 8
```

`MK_BUILD_PLUGIN=OFF` builds only the core, tests and benchmark (no JUCE).
On iOS, configure with `-G Xcode -DCMAKE_SYSTEM_NAME=iOS`; the AUv3 extension
and the standalone container are produced from the same target.

## CI (GitHub Actions)

| Workflow | Produces | Runner |
| --- | --- | --- |
| `build-vst3.yml` | VST3 + Standalone (Windows x64), VST3 + Audio Unit (macOS universal `arm64;x86_64`), DSP tests (Linux) | `windows-latest`, `macos-15`, `ubuntu-latest` |
| `ios-auv3.yml` | `WOBBLE-MACHINE-<version>-ios.ipa` (container app with the AUv3 `.appex` embedded) and a standalone `.appex.zip` | `macos-15` |

Each run uploads its builds as workflow artifacts (90 days). Pushing a `v*` tag
also attaches the same files to that GitHub release. JUCE is cloned once per tag
into `external/JUCE` and cached.

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
container (`audio.mk.wobblemachine`) and the extension
(`audio.mk.wobblemachine.wobblemachineAUv3`). Run the workflow manually and set
`signing` to `unsigned` to force a sideload build even when the secrets exist.

The job fails fast if the produced `.appex` is missing from `Payload/.../PlugIns`
or does not declare the `com.apple.AudioUnit-UI` extension point.

## Test and validate

```powershell
.\build\Release\mk_wobble_tests.exe
.\build\MKPluginSmokeTest_artefacts\Release\MKPluginSmokeTest.exe
.\build\Release\mk_wobble_bench.exe
pluginval.exe --strictness-level 8 --validate-in-process --skip-gui-tests `
    "build\MKWobbleBass_artefacts\Release\VST3\WOBBLE MACHINE.vst3"
```

The DSP suite covers tuning, wobble tempo sync, filters, distortion, polyphony,
mono/glide, the macro knobs, every character and the morph, SUB LOCK, ONE-KNOB
FILTH, the OTT compressor, RANDOM BASS, all 62 presets, the trance gate and idle
silence.

The smoke test drives the real processor (MIDI, automation, on-screen keyboard,
preset loading, randomize, state round-trip) and writes off-screen PNG snapshots
of EASY, every ADVANCED section and the minimum window size.

## Performance (measured, Release, 48 kHz, 128-sample blocks)

| Scenario | % of one core |
| --- | --- |
| 1 voice, no effects | 0.33 % |
| 1 voice, distortion | 0.28 % |
| 1 voice, all effects | 0.43 % |
| 4 voices, all effects | 0.99 % |
| 8 voices, all effects | 1.68 % |
| 8 voices, neuro preset | 2.52 % |
| 8 voices, OTT on | 1.74 % |
| 16 voices, all effects | 1.67 % (adaptive limit) |

Static footprint: 262 KB wavetable banks + ~868 KB engine buffers (16 voices,
formant and comb lines, delay, reverb).

How the budget is kept:

* modulation, filter coefficients, pitch and wavetable position update once per
  32-sample control block; only oscillators, filters, envelopes and gain run per
  sample
* inactive oscillators, filters, distortion, formant and effects are skipped
* the parallel formant bank only runs when its mix is above zero
* fixed-size voice pool with stealing and an adaptive voice limiter
* no allocation, locking or file I/O on the audio thread

## Playing from the plug-in window

The bottom strip is a real keyboard covering C-2 to C3 (MIDI 0-60), the register
this instrument lives in. Click or tap keys, or click the strip once and play
with the computer keyboard. Notes go through `MidiKeyboardState`, so hosts
record them like any other MIDI input.

## iOS assets

| File | Use |
| --- | --- |
| `Assets/AppIcon-1024.png` | App icon (`ICON_BIG`, iPadOS 17 single-size) |
| `Assets/iOS-Thumbnail-1024.png` | Square store tile / documentation thumbnail |
| `Assets/iOS-Screenshot-2732x2048.png` | App Store iPad screenshot (landscape) |
| `Assets/ui-render@2x.png` | Raw 2x editor render used by the composer |

Regenerate:

```powershell
python Tools\make_app_icon.py 1024
cmake --build build --config Release --target MKThumbnail
.\build\MKThumbnail_artefacts\Release\MKThumbnail.exe
python Tools\make_thumbnails.py
```