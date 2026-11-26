#include "DSP/Engine.h"
#include "DSP/Presets.h"
#include "DSP/GatePresets.h"
#include "DSP/TranceGate.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace
{
int failures = 0;

void check (bool condition, const char* message)
{
    if (! condition)
    {
        std::printf ("FAIL: %s\n", message);
        ++failures;
    }
}

double rms (const std::vector<float>& data)
{
    double sum = 0.0;
    for (const float v : data)
        sum += (double) v * v;
    return std::sqrt (sum / (double) std::max<size_t> (data.size(), 1));
}

// Goertzel magnitude: cheap single-bin DFT, enough for tuning assertions.
double magnitude (const std::vector<float>& data, double frequency, double sampleRate)
{
    const double w = 2.0 * 3.14159265358979323846 * frequency / sampleRate;
    const double coeff = 2.0 * std::cos (w);
    double s0 = 0.0, s1 = 0.0, s2 = 0.0;
    for (const float sample : data)
    {
        s0 = (double) sample + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    const double power = s1 * s1 + s2 * s2 - coeff * s1 * s2;
    return 2.0 * std::sqrt (std::max (power, 0.0)) / (double) data.size();
}

struct Render
{
    std::vector<float> left, right;
};

Render renderEngine (mk::Engine& engine, int numSamples)
{
    Render output;
    output.left.assign ((size_t) numSamples, 0.0f);
    output.right.assign ((size_t) numSamples, 0.0f);

    constexpr int block = 128;
    for (int offset = 0; offset < numSamples; offset += block)
    {
        const int chunk = std::min (block, numSamples - offset);
        engine.process (output.left.data() + offset, output.right.data() + offset, chunk);
    }
    return output;
}

void testSilenceAndOutput()
{
    mk::Engine engine;
    engine.prepare (48000.0, 512);

    const auto silent = renderEngine (engine, 4096);
    check (rms (silent.left) == 0.0, "idle engine is exactly silent");

    engine.noteOn (33, 1.0f);
    const auto sound = renderEngine (engine, 96000);
    check (rms (sound.left) > 0.01, "default preset produces audio");
    check (rms (sound.left) < 0.6, "default preset is not clipping");

    for (const float v : sound.left)
        check (std::isfinite (v) && std::fabs (v) <= 1.0f, "output stays finite and bounded");
}

void testTuning()
{
    for (const double rate : { 44100.0, 48000.0, 96000.0 })
    {
        mk::Engine engine;
        engine.prepare (rate, 512);
        engine.params().set (mk::P::osc1Wave, 4.0f);   // sine
        engine.params().set (mk::P::osc1Level, 1.0f);
        engine.params().set (mk::P::subLevel, 0.0f);
        engine.params().set (mk::P::fltCutoff, 18000.0f);
        engine.params().set (mk::P::fltEnvAmt, 0.0f);
        engine.params().set (mk::P::wobDepth, 0.0f);
        engine.params().set (mk::P::distType, 0.0f);
        engine.params().requestSnap();
        engine.noteOn (45, 1.0f);   // A2 = 110 Hz

        renderEngine (engine, (int) (rate * 0.05));
        const auto steady = renderEngine (engine, (int) (rate * 0.5));

        const double expected = 110.0;
        const double atExpected = magnitude (steady.left, expected, rate);
        const double atThird = magnitude (steady.left, expected * 1.5, rate);
        check (atExpected > 0.02, "A2 fundamental is audible");
        check (atExpected > atThird * 4.0, "partials are absent from a sine");
    }
}

void testWobbleSync()
{
    mk::Engine engine;
    engine.prepare (48000.0, 512);
    engine.setTempo (120.0);
    engine.params().set (mk::P::wobMode, 0.0f);   // sync
    engine.params().set (mk::P::wobDiv, 2.0f);    // 1/4
    engine.params().set (mk::P::wobDepth, 1.0f);
    engine.params().requestSnap();

    engine.noteOn (33, 1.0f);
    renderEngine (engine, 2048);
    check (std::fabs (engine.wobbleRateHz() - 2.0f) < 0.01f, "1/4 at 120 BPM is 2 Hz");

    engine.setTempo (90.0);
    renderEngine (engine, 2048);
    check (std::fabs (engine.wobbleRateHz() - 1.5f) < 0.01f, "1/4 at 90 BPM is 1.5 Hz");

    engine.params().set (mk::P::wobDiv, 4.0f);    // 1/16
    renderEngine (engine, 2048);
    check (std::fabs (engine.wobbleRateHz() - 6.0f) < 0.05f, "1/16 at 90 BPM is 6 Hz");

    engine.params().set (mk::P::wobDiv, 13.0f);   // 1/4 dotted
    renderEngine (engine, 2048);
    check (std::fabs (engine.wobbleRateHz() - (1.5f / 1.5f)) < 0.05f, "1/4D at 90 BPM is 1 Hz");

    engine.params().set (mk::P::wobMode, 1.0f);   // free
    engine.params().set (mk::P::wobRate, 7.5f);
    renderEngine (engine, 9600);                  // let the rate smoother settle
    check (std::fabs (engine.wobbleRateHz() - 7.5f) < 0.1f, "free wobble rate follows the knob");
}

void testFilterAndDistortion()
{
    const auto renderWith = [] (int filterType, float cutoff, int distType, float drive)
    {
        mk::Engine engine;
        engine.prepare (48000.0, 512);
        auto& store = engine.params();
        store.set (mk::P::osc1Wave, 0.0f);
        store.set (mk::P::fltType, (float) filterType);
        store.set (mk::P::fltCutoff, cutoff);
        store.set (mk::P::fltReso, 0.45f);
        store.set (mk::P::fltEnvAmt, 0.0f);
        store.set (mk::P::wobDepth, 0.0f);
        store.set (mk::P::subLevel, 0.0f);
        store.set (mk::P::distType, (float) distType);
        store.set (mk::P::distRoute, distType == 0 ? 0.0f : 1.0f);
        store.set (mk::P::distDrive, drive);
        store.set (mk::P::distMix, 1.0f);
        store.requestSnap();
        engine.noteOn (45, 1.0f);
        renderEngine (engine, 24000);
        return renderEngine (engine, 48000);
    };

    const auto bright = renderWith (mk::FilterLP24, 8000.0f, 0, 0.0f);
    const auto dark = renderWith (mk::FilterLP24, 120.0f, 0, 0.0f);
    check (magnitude (bright.left, 660, 48000) > magnitude (dark.left, 660, 48000) * 4.0,
           "24 dB low-pass removes upper harmonics");

    for (int type = 1; type <= 11; ++type)
    {
        const auto shaped = renderWith (mk::FilterLP24, 1200.0f, type, 0.55f);
        char message[96];
        std::snprintf (message, sizeof (message), "distortion %d produces output (rms %.4f)",
                       type, rms (shaped.left));
        check (rms (shaped.left) > 0.002, message);
        for (const float v : shaped.left)
            check (std::isfinite (v), "distortion output is finite");
    }
}

void testPolyphonyAndStealing()
{
    mk::Engine engine;
    engine.prepare (48000.0, 512);
    engine.params().set (mk::P::polyphony, 8.0f);
    engine.params().set (mk::P::adaptive, 0.0f);
    engine.params().requestSnap();

    for (int i = 0; i < 8; ++i)
        engine.noteOn (28 + i * 2, 0.9f);

    renderEngine (engine, 2048);
    check (engine.activeVoiceCount() == 8, "eight simultaneous voices");

    // A ninth note must steal, not exceed the limit.
    engine.noteOn (52, 0.9f);
    renderEngine (engine, 1024);
    check (engine.activeVoiceCount() <= 9, "voice stealing keeps the pool bounded");
}

void testMonoAndGlide()
{
    mk::Engine engine;
    engine.prepare (48000.0, 512);
    engine.params().set (mk::P::polyMode, 1.0f);
    engine.params().set (mk::P::glideTime, 80.0f);
    engine.params().requestSnap();

    engine.noteOn (33, 1.0f);
    renderEngine (engine, 4800);
    engine.noteOn (45, 1.0f);
    renderEngine (engine, 64);
    check (engine.activeVoiceCount() == 1, "mono mode uses exactly one voice");

    engine.noteOff (45);
    renderEngine (engine, 48000);
    check (engine.activeVoiceCount() <= 1, "mono release is clean");
}

void testPresets()
{
    const int count = mk::factoryPresetCount();
    check (count >= 50, "at least fifty factory presets");

    mk::ParamSnapshot a, b;
    for (int i = 0; i < count; ++i)
    {
        const auto& preset = mk::factoryPreset (i);
        check (mk::applyFactoryPreset (i, a), "preset parses");
        mk::applyFactoryPreset (i, b);

        int differences = 0;
        for (int p = 0; p < mk::kParamCount; ++p)
            if (std::fabs (a.values[p] - b.values[p]) > 1.0e-6f)
                ++differences;
        check (differences == 0, "preset parsing is deterministic");

        mk::Engine engine;
        engine.prepare (48000.0, 512);
        engine.params().setAll (a);
        engine.params().requestSnap();
        engine.noteOn (33, 1.0f);
        const auto out = renderEngine (engine, 12000);
        for (const float v : out.left)
            check (std::isfinite (v) && std::fabs (v) <= 1.0f, "preset output is finite and bounded");
        check (rms (out.left) > 0.0005, preset.name);
    }
}

void testMatrixAndMacros()
{
    mk::Engine engine;
    engine.prepare (48000.0, 512);
    auto& store = engine.params();
    store.set (mk::P::wobDepth, 0.0f);
    store.set (mk::P::easyBass, 0.5f);
    store.requestSnap();

    engine.noteOn (33, 1.0f);
    renderEngine (engine, 12000);
    const auto before = renderEngine (engine, 24000);

    // BASS is a bipolar macro: below the centre it opens the filter, above it
    // drops the cutoff and adds sub.
    store.set (mk::P::easyBass, 1.0f);
    renderEngine (engine, 12000);
    const auto after = renderEngine (engine, 24000);

    check (magnitude (before.left, 400, 48000) > magnitude (after.left, 400, 48000) * 1.2,
           "bass macro closes the filter");

    store.set (mk::P::easyBass, 0.0f);
    renderEngine (engine, 12000);
    const auto opened = renderEngine (engine, 24000);
    check (magnitude (opened.left, 400, 48000) > magnitude (after.left, 400, 48000),
           "bass macro opens the filter from below the centre");

    store.set (mk::P::mat1Src, (float) mk::SrcModWheel);
    store.set (mk::P::mat1Dst, (float) mk::DstPitch);
    store.set (mk::P::mat1Amt, 0.5f);
    engine.setModWheel (1.0f);
    renderEngine (engine, 24000);
    check (engine.activeVoiceCount() == 1, "matrix routing does not disturb voice allocation");
}

void testCharactersAndMorph()
{
    // Every character has to change the sound, and the morph knob has to move
    // continuously between two of them without blowing up.
    mk::Engine reference;
    reference.prepare (48000.0, 512);
    reference.params().set (mk::P::wobDepth, 0.0f);
    reference.params().set (mk::P::bassMorph, 0.0f);
    reference.params().set (mk::P::bassChar, (float) mk::CharWobble);
    reference.params().set (mk::P::morphChar, (float) mk::CharWobble);
    reference.params().requestSnap();
    reference.noteOn (33, 1.0f);
    renderEngine (reference, 12000);
    const auto plain = renderEngine (reference, 24000);

    for (int character = 1; character < mk::kCharacterCount; ++character)
    {
        mk::Engine engine;
        engine.prepare (48000.0, 512);
        engine.params().set (mk::P::wobDepth, 0.0f);
        engine.params().set (mk::P::bassChar, (float) character);
        engine.params().set (mk::P::morphChar, (float) character);
        engine.params().requestSnap();
        engine.noteOn (33, 1.0f);
        renderEngine (engine, 12000);
        const auto shaped = renderEngine (engine, 24000);

        double difference = 0.0;
        for (size_t i = 0; i < shaped.left.size(); ++i)
            difference += std::fabs ((double) shaped.left[i] - (double) plain.left[i]);

        check (difference > 1.0, "character changes the sound");
        check (rms (shaped.left) > 0.005, "character stays audible");
    }

    mk::Engine morphed;
    morphed.prepare (48000.0, 512);
    morphed.params().set (mk::P::bassChar, (float) mk::CharReese);
    morphed.params().set (mk::P::morphChar, (float) mk::CharTalking);
    morphed.params().set (mk::P::bassMorph, 0.5f);
    morphed.params().requestSnap();
    morphed.noteOn (33, 1.0f);
    const auto blend = renderEngine (morphed, 24000);
    check (rms (blend.left) > 0.005, "morph between two characters stays audible");
    for (const float v : blend.left)
        check (std::isfinite (v) && std::fabs (v) <= 1.0f, "morphed character stays bounded");
}

void testSubLock()
{
    // A distorted, wide patch: with SUB LOCK the sub must stay centred and
    // clean, so a sub-only note is mono no matter the drive or width.
    mk::Engine engine;
    engine.prepare (48000.0, 512);
    auto& store = engine.params();
    store.set (mk::P::osc1Level, 0.0f);
    store.set (mk::P::subLevel, 0.8f);
    store.set (mk::P::distType, (float) mk::DistTube);
    store.set (mk::P::distRoute, 1.0f);
    store.set (mk::P::distDrive, 1.0f);
    store.set (mk::P::distMix, 1.0f);
    store.set (mk::P::chOn, 1.0f);
    store.set (mk::P::chMix, 1.0f);
    store.set (mk::P::chWidth, 1.0f);
    store.set (mk::P::easyWidth, 1.0f);
    store.set (mk::P::wobDepth, 0.0f);
    store.set (mk::P::subLock, 1.0f);
    store.requestSnap();

    engine.noteOn (33, 1.0f);
    const auto locked = renderEngine (engine, 24000);
    check (rms (locked.left) > 0.01, "sub is audible with SUB LOCK");

    double maxSide = 0.0;
    for (size_t i = 0; i < locked.left.size(); ++i)
        maxSide = std::max (maxSide, std::fabs ((double) locked.left[i] - (double) locked.right[i]));
    check (maxSide < 1.0e-4, "SUB LOCK keeps the sub perfectly mono");

    // The sub is a pure sine, so a driven harmonic in the locked render means
    // the sub leaked into the distortion stage.
    const double cleanHarmonic = magnitude (locked.left, 165.0, 48000);
    check (cleanHarmonic < 0.002, "SUB LOCK keeps the sub clean of distortion");

    store.set (mk::P::subLock, 0.0f);
    renderEngine (engine, 24000);
    const auto loose = renderEngine (engine, 24000);
    const double drivenHarmonic = magnitude (loose.left, 165.0, 48000);
    check (drivenHarmonic > cleanHarmonic * 2.0, "without SUB LOCK the sub is driven with the bass");
}

void testOneKnobFilth()
{
    mk::Engine engine;
    engine.prepare (48000.0, 512);
    auto& store = engine.params();
    store.set (mk::P::osc1Wave, (float) mk::WaveSine);
    store.set (mk::P::osc1Level, 1.0f);
    store.set (mk::P::subLevel, 0.0f);
    store.set (mk::P::fltCutoff, 18000.0f);
    store.set (mk::P::fltEnvAmt, 0.0f);
    store.set (mk::P::wobDepth, 0.0f);
    store.set (mk::P::distType, 0.0f);
    store.set (mk::P::distRoute, 0.0f);
    store.set (mk::P::easyGrit, 0.5f);
    store.requestSnap();

    engine.noteOn (33, 1.0f);
    renderEngine (engine, 12000);
    const auto clean = renderEngine (engine, 24000);

    // A single knob: from the centre upwards the distortion switches itself on.
    store.set (mk::P::easyGrit, 1.0f);
    renderEngine (engine, 12000);
    const auto filthy = renderEngine (engine, 24000);

    const auto harmonics = [] (const std::vector<float>& data)
    {
        double sum = 0.0;
        for (const double hz : { 110.0, 165.0, 220.0, 275.0, 330.0 })
            sum += magnitude (data, hz, 48000);
        return sum;
    };

    const double cleanHarmonic = harmonics (clean.left);
    const double filthyHarmonic = harmonics (filthy.left);

    check (filthyHarmonic > cleanHarmonic * 1.5, "GRIT is one-knob filth");
    check (rms (filthy.left) > 0.005, "filth stays audible");
    for (const float v : filthy.left)
        check (std::isfinite (v) && std::fabs (v) <= 1.0f, "filth stays bounded");
}

void testOttCompressor()
{
    mk::Engine engine;
    engine.prepare (48000.0, 512);
    auto& store = engine.params();
    store.set (mk::P::osc1Wave, (float) mk::WaveSine);
    store.set (mk::P::osc1Level, 0.4f);
    store.set (mk::P::subLevel, 0.0f);
    store.set (mk::P::fltCutoff, 18000.0f);
    store.set (mk::P::wobDepth, 0.0f);
    store.set (mk::P::outputDb, -18.0f);
    store.set (mk::P::cpOn, 0.0f);
    store.set (mk::P::ottOn, 0.0f);
    store.requestSnap();

    engine.noteOn (33, 1.0f);
    renderEngine (engine, 12000);
    const auto plain = renderEngine (engine, 24000);

    store.set (mk::P::ottOn, 1.0f);
    store.set (mk::P::ottAmount, 1.0f);
    store.set (mk::P::ottMix, 1.0f);
    renderEngine (engine, 12000);
    const auto ott = renderEngine (engine, 24000);

    check (rms (ott.left) > rms (plain.left) * 1.2, "OTT lifts quiet material");
    for (const float v : ott.left)
        check (std::isfinite (v) && std::fabs (v) <= 1.0f, "OTT stays bounded");
}

void testRandomBass()
{
    mk::ParamSnapshot a, b;
    mk::makeRandomBass (a, 12345u);
    mk::makeRandomBass (b, 12345u);
    for (int i = 0; i < mk::kParamCount; ++i)
        check (std::fabs (a.values[i] - b.values[i]) < 1.0e-6f, "random bass is deterministic");

    for (uint32_t seed = 1; seed <= 12; ++seed)
    {
        mk::ParamSnapshot snapshot;
        mk::makeRandomBass (snapshot, seed);

        mk::Engine engine;
        engine.prepare (48000.0, 512);
        engine.params().setAll (snapshot);
        engine.params().requestSnap();
        engine.noteOn (33, 1.0f);

        const auto out = renderEngine (engine, 24000);
        check (rms (out.left) > 0.01, "random bass is audible");
        for (const float v : out.left)
            check (std::isfinite (v) && std::fabs (v) <= 1.0f, "random bass stays bounded");
    }
}

void reportMemory()
{
    const size_t wavetables = mk::WavetableSet::memoryBytes();
    const size_t engine = sizeof (mk::Engine);
    std::printf ("RAM: wavetables %zu KB, engine state %zu KB, total static DSP %.1f MB\n",
                 wavetables / 1024, engine / 1024,
                 (double) (wavetables + engine) / (1024.0 * 1024.0));
}

void testTranceGate()
{
    mk::TranceGate gate;
    gate.prepare (48000.0);
    gate.setPattern (0x5555555555555555ull);            // odd steps open
    gate.setParameters (true, 1, 1.0f, 0.0f, 120.0);   // 1/16, full depth, no smoothing

    // 120 BPM, 1/16 -> 6000 samples per step at 48 kHz.
    std::vector<float> left (48000, 1.0f), right (48000, 1.0f);
    gate.process (left.data(), right.data(), (int) left.size());

    const auto averageOver = [] (const std::vector<float>& data, int start, int length)
    {
        double sum = 0.0;
        for (int i = start; i < start + length; ++i)
            sum += (double) data[(size_t) i];
        return sum / (double) length;
    };

    check (averageOver (left, 400, 1500) > 0.9, "gate passes an enabled step");
    check (averageOver (left, 6400, 1500) < 0.1, "gate silences a disabled step");

    // Bypass must be an exact pass-through.
    gate.setParameters (false, 1, 1.0f, 0.0f, 120.0);
    std::vector<float> passL (4800, 1.0f), passR (4800, 1.0f);
    gate.process (passL.data(), passR.data(), (int) passL.size());
    check (passL[4799] > 0.999f, "gate bypass is a pass-through");

    // Depth 0 must not attenuate anything even on closed steps.
    mk::TranceGate gentle;
    gentle.prepare (48000.0);
    gentle.setPattern (0ull);          // every step closed
    gentle.setParameters (true, 1, 0.0f, 0.0f, 120.0);
    std::vector<float> gentleL (4800, 1.0f), gentleR (4800, 1.0f);
    gentle.process (gentleL.data(), gentleR.data(), (int) gentleL.size());
    check (gentleL[4799] > 0.999f, "depth zero leaves the signal alone");
}

void testGatePresets()
{
    check (mk::gatePresetCount() == 100, "gate library ships 100 presets");

    for (int i = 0; i < mk::gatePresetCount(); ++i)
    {
        const auto& preset = mk::gatePreset (i);
        check (preset.name != nullptr && preset.name[0] != 0, "gate preset has a name");
        check (preset.division >= 0 && preset.division <= 3, "gate preset rate is valid");
        check (preset.depth >= 0.0f && preset.depth <= 1.0f, "gate preset depth is valid");
        check (preset.smooth >= 0.0f && preset.smooth <= 1.0f, "gate preset smoothing is valid");

        int length = 0;
        bool valid = true;

        for (const char* c = preset.bits; *c != 0; ++c, ++length)
            if (*c != '0' && *c != '1')
                valid = false;

        check (length == 16 && valid, "gate preset pattern is 16 bits");
        check (mk::gatePresetMask (i) != 0, "gate preset has at least one open step");
    }

    // The first bit of the string must be step 1 of the grid.
    mk::TranceGate gate;
    gate.prepare (48000.0);
    gate.setPattern (mk::gatePresetMask (0));      // 1010101010101010

    check (gate.stepIsOn (0) && ! gate.stepIsOn (1) && gate.stepIsOn (2),
           "gate preset bit order matches the grid");
}

void testIdleSilenceForEveryPreset()
{
    // Regression guard: no preset may ring, hiss or self-oscillate when no note
    // is playing, at every sample rate the iPad and desktop hosts use.
    for (const double rate : { 44100.0, 48000.0 })
    {
        for (int preset = 0; preset < mk::factoryPresetCount(); ++preset)
        {
            mk::Engine engine;
            engine.prepare (rate, 512);

            mk::ParamSnapshot snapshot;
            for (int p = 0; p < mk::kParamCount; ++p)
                snapshot.values[p] = mk::paramMeta (p).def;

            mk::applyFactoryPreset (preset, snapshot);
            engine.params().setAll (snapshot);
            engine.params().requestSnap();

            const auto idle = renderEngine (engine, (int) (rate * 0.25));
            check (rms (idle.left) < 1.0e-7, "preset is silent while idle");
        }
    }
}

void testReleaseTailEnds()
{
    mk::Engine engine;
    engine.prepare (48000.0, 512);
    engine.noteOn (33, 1.0f);
    renderEngine (engine, 24000);
    engine.allNotesOff (false);

    const auto tail = renderEngine (engine, 48000 * 3);
    const std::vector<float> lastSecond (tail.left.end() - 48000, tail.left.end());

    check (rms (lastSecond) < 1.0e-6, "release tail decays to silence");
    check (engine.activeVoiceCount() == 0, "all voices free after the tail");
}

} // namespace

int main()
{
    testSilenceAndOutput();
    testTuning();
    testWobbleSync();
    testFilterAndDistortion();
    testPolyphonyAndStealing();
    testMonoAndGlide();
    testMatrixAndMacros();
    testCharactersAndMorph();
    testSubLock();
    testOneKnobFilth();
    testOttCompressor();
    testRandomBass();
    testPresets();
    testTranceGate();
    testGatePresets();
    testIdleSilenceForEveryPreset();
    testReleaseTailEnds();
    reportMemory();

    if (failures == 0)
        std::printf ("PASS: engine, tuning, wobble, characters, sub lock, filth, ott, random, filters, voices, presets, gate\n");
    else
        std::printf ("%d failure(s)\n", failures);

    return failures == 0 ? 0 : 1;
}
