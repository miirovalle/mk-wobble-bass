// Offline CPU / memory benchmark for the wobble bass engine.
//
// Reports the cost as a percentage of one core, which is what matters for the
// A10X target (three cores, one of them already busy with the host). Rendering
// is done at 48 kHz with 128-sample blocks, the worst realistic case for a
// per-block control rate of 32 samples.

#include "DSP/Engine.h"
#include "DSP/Presets.h"

#include <chrono>
#include <cstdio>
#include <vector>

namespace
{

struct Scenario
{
    const char* name;
    int presetIndex;
    int voiceCount;
    bool effectsOn;
    bool distortionOn;
};

struct Result
{
    double percentOfCore;
    double realtimeFactor;
    double millisecondsPerSecond;
};

Result measure (const Scenario& scenario, double seconds)
{
    mk::Engine engine;
    engine.prepare (48000.0, 128);

    mk::ParamSnapshot snapshot;
    if (scenario.presetIndex >= 0)
        mk::applyFactoryPreset (scenario.presetIndex, snapshot);
    else
        mk::resetSnapshotToDefaults (snapshot);

    snapshot.values[(int) mk::P::adaptive] = 0.0f;
    snapshot.values[(int) mk::P::polyphony] = (float) std::max (scenario.voiceCount, 1);
    if (scenario.distortionOn)
    {
        snapshot.values[(int) mk::P::distType] = (float) mk::DistAnalog;
        snapshot.values[(int) mk::P::distRoute] = 1.0f;
        snapshot.values[(int) mk::P::distMix] = 0.6f;
        snapshot.values[(int) mk::P::distDrive] = 0.5f;
    }
    if (scenario.effectsOn)
    {
        snapshot.values[(int) mk::P::chOn] = 1.0f;
        snapshot.values[(int) mk::P::dlOn] = 1.0f;
        snapshot.values[(int) mk::P::rvOn] = 1.0f;
        snapshot.values[(int) mk::P::mbOn] = 1.0f;
    }
    else
    {
        snapshot.values[(int) mk::P::chOn] = 0.0f;
        snapshot.values[(int) mk::P::dlOn] = 0.0f;
        snapshot.values[(int) mk::P::rvOn] = 0.0f;
        snapshot.values[(int) mk::P::mbOn] = 0.0f;
    }

    engine.params().setAll (snapshot);
    engine.params().requestSnap();

    // Spread the notes across the bass register.
    const int baseNote = 28;
    for (int i = 0; i < scenario.voiceCount; ++i)
        engine.noteOn (baseNote + (i % 12) * 2, 0.85f);

    // Warm-up: fills delay and reverb tails so the measurement is steady state.
    std::vector<float> left (128, 0.0f), right (128, 0.0f);
    for (int i = 0; i < 400; ++i)
        engine.process (left.data(), right.data(), 128);

    const int totalSamples = (int) (seconds * 48000.0);
    const int blocks = totalSamples / 128;

    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < blocks; ++i)
        engine.process (left.data(), right.data(), 128);
    const auto elapsed = std::chrono::duration<double> (std::chrono::steady_clock::now() - start).count();

    Result result;
    result.realtimeFactor = seconds / elapsed;
    result.percentOfCore = 100.0 * elapsed / seconds;
    result.millisecondsPerSecond = elapsed * 1000.0 / seconds;
    return result;
}

} // namespace

int main()
{
    constexpr double seconds = 10.0;

    const Scenario scenarios[] = {
        { "1 voice, no FX (Eco path)", 3, 1, false, false },
        { "1 voice, distortion", 3, 1, false, true },
        { "1 voice, all FX on", 3, 1, true, true },
        { "4 voices, all FX on", 1, 4, true, true },
        { "8 voices, all FX on", 1, 8, true, true },
        { "8 voices, neuro preset", 17, 8, true, true },
        { "16 voices, all FX on", 1, 16, true, true },
    };

    std::printf ("MK WOBBLE BASS - 48 kHz, 128-sample blocks, Release build\n");
    std::printf ("%-28s %10s %12s %14s\n", "scenario", "% of core", "realtime x", "ms CPU/s audio");

    for (const auto& scenario : scenarios)
    {
        const auto result = measure (scenario, seconds);
        std::printf ("%-28s %9.2f%% %11.1fx %13.1f\n", scenario.name, result.percentOfCore,
                     result.realtimeFactor, result.millisecondsPerSecond);
    }

    std::printf ("\nStatic footprint\n");
    std::printf ("  wavetable banks : %6.1f KB (procedural, 8 banks x 8 frames x 9 mips)\n",
                 (double) mk::WavetableSet::memoryBytes() / 1024.0);

    mk::Engine probe;
    probe.prepare (48000.0, 128);
    std::printf ("  engine + buffers: %6.1f KB (16 voices, comb lines, delay, reverb)\n",
                 (double) probe.memoryFootprintBytes() / 1024.0);
    std::printf ("  total DSP       : %6.1f KB\n",
                 (double) (probe.memoryFootprintBytes() + mk::WavetableSet::memoryBytes()) / 1024.0);

    return 0;
}
