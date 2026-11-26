#pragma once

// Band-limited oscillator unit: PolyBLEP saw/square/pulse/triangle, table sine,
// mip-mapped wavetable scanning with frame morphing, supersaw and noise.
//
// One instance per oscillator slot per voice. Unison is a fixed array of seven
// phase accumulators, so rendering never allocates and inactive unison voices
// cost nothing because the loop bound is the configured count.

#include "DSP/MkCommon.h"
#include "DSP/Params.h"
#include "DSP/Wavetables.h"

namespace mk
{

class OscUnit
{
public:
    static constexpr int kMaxUnison = 7;

    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate > 0.0 ? sampleRate : 48000.0;
        reset (0.0f, 0.0f);
    }

    void reset (float phase01, float seed) noexcept
    {
        for (int i = 0; i < kMaxUnison; ++i)
        {
            phase[(size_t) i] = std::fmod (phase01 + (float) i * 0.11f, 1.0f);
            triState[(size_t) i] = -1.0f;
        }
        rng.seed ((uint32_t) (seed * 7919.0f) + 17u);
    }

    // Control rate. pitchSemitones already contains glide, bend and modulation.
    void configure (int waveform, int bank, float wtPos, int unisonCount, float detuneCents,
                    float spread, float pulseWidth, float level, bool cubicQuality) noexcept
    {
        wave = waveform;
        bankIndex = bank;
        pos = clampf (wtPos, 0.0f, 1.0f);
        levelValue = level;
        pw = clampf (pulseWidth, 0.05f, 0.95f);
        cubic = cubicQuality;

        int count = clampValue (unisonCount, 1, kMaxUnison);
        if (wave == WaveSupersaw)
            count = std::max (count, std::min (5, kMaxUnison));

        if (count != activeUnison || detuneCents != detune || spread != spreadAmount)
        {
            activeUnison = count;
            detune = detuneCents;
            spreadAmount = spread;

            // Spread unison voices evenly across the stereo field and detune
            // them symmetrically, so mono sum stays centred.
            for (int i = 0; i < count; ++i)
            {
                const float centred = count == 1 ? 0.0f
                                                 : ((float) i / (float) (count - 1)) * 2.0f - 1.0f;
                ratio[(size_t) i] = fastSemitoneRatio (centred * detune * 0.01f * 0.5f);
                const float pan = centred * spreadAmount;
                panGains (pan, panL[(size_t) i], panR[(size_t) i]);
                const float normalise = count > 1 ? 1.0f / std::sqrt ((float) count) : 1.0f;
                panL[(size_t) i] *= normalise;
                panR[(size_t) i] *= normalise;
            }
        }

        const bool needsTable = wave == WaveWavetable || wave == WaveFM;
        if (needsTable)
        {
            const auto& set = WavetableSet::instance();
            const float framePos = pos * (float) (WavetableSet::kFrames - 1);
            const int f0 = clampValue ((int) framePos, 0, WavetableSet::kFrames - 1);
            const int f1 = std::min (f0 + 1, WavetableSet::kFrames - 1);
            frameA = &set.frame (bankIndex, f0);
            frameB = &set.frame (bankIndex, f1);
            frameMix = framePos - (float) f0;
        }
    }

    inline void setPitch (float semitones) noexcept
    {
        fundamental = midiToHz (semitones);
        for (int i = 0; i < activeUnison; ++i)
            increment[(size_t) i] = clampf (fundamental * ratio[(size_t) i] / (float) sr, 0.0f, 0.49f);

        if (wave == WaveWavetable || wave == WaveFM)
            mip = WavetableSet::mipForFrequency (fundamental, sr);
    }

    // Per sample. pmOffset is a phase offset in cycles (0..1) for FM/PM.
    inline void render (float& outL, float& outR, float pmOffset = 0.0f) noexcept
    {
        if (wave == WaveNoise)
        {
            const float n = rng.nextBipolar() * levelValue;
            outL += n;
            outR += n;
            return;
        }

        float sumL = 0.0f;
        float sumR = 0.0f;

        for (int i = 0; i < activeUnison; ++i)
        {
            float& ph = phase[(size_t) i];
            const float dt = increment[(size_t) i];
            float sample = 0.0f;

            switch (wave)
            {
                case WaveSaw:
                    sample = renderSaw (ph, dt);
                    break;
                case WaveSquare:
                    sample = renderSquare (ph, dt, 0.5f);
                    break;
                case WavePulse:
                    sample = renderSquare (ph, dt, pw);
                    break;
                case WaveTriangle:
                    sample = renderTriangle (ph, dt, (size_t) i);
                    break;
                case WaveSine:
                    sample = SineTable::instance().lookup (ph);
                    break;
                case WaveSupersaw:
                    sample = renderSaw (ph, dt);
                    break;
                case WaveWavetable:
                    sample = renderTable (ph + pmOffset, dt);
                    break;
                case WaveFM:
                default:
                    sample = SineTable::instance().lookup (ph + pmOffset);
                    break;
            }

            ph += dt;
            if (ph >= 1.0f)
                ph -= 1.0f;

            sumL += sample * panL[(size_t) i];
            sumR += sample * panR[(size_t) i];
        }

        outL += sumL * levelValue;
        outR += sumR * levelValue;
    }

    float level() const noexcept { return levelValue; }
    int waveform() const noexcept { return wave; }

private:
    static inline float polyBlep (float t, float dt) noexcept
    {
        if (t < dt)
        {
            const float x = t / dt;
            return x + x - x * x - 1.0f;
        }
        if (t > 1.0f - dt)
        {
            const float x = (t - 1.0f) / dt;
            return x * x + x + x + 1.0f;
        }
        return 0.0f;
    }

    static inline float renderSaw (float t, float dt) noexcept
    {
        return 2.0f * t - 1.0f - polyBlep (t, dt);
    }

    static inline float renderSquare (float t, float dt, float duty) noexcept
    {
        float s = t < duty ? 1.0f : -1.0f;
        s += polyBlep (t, dt);
        float t2 = t + (1.0f - duty);
        if (t2 >= 1.0f)
            t2 -= 1.0f;
        s -= polyBlep (t2, dt);
        return s;
    }

    inline float renderTriangle (float t, float dt, size_t index) noexcept
    {
        // Differentiated square + leaky integrator: alias-free at low cost.
        const float square = t < 0.5f ? 1.0f : -1.0f;
        float& state = triState[index];
        state = (state + 4.0f * dt * square) * (1.0f - dt * 0.0009f);
        return clampf (state, -1.0f, 1.0f);
    }

    inline float renderTable (float t, float dt) noexcept
    {
        t -= std::floor (t);
        if (frameA == nullptr)
            return 0.0f;

        const float a = readFrame (*frameA, mip, t, dt);
        if (frameMix <= 0.0f || frameA == frameB)
            return a;
        const float b = readFrame (*frameB, mip, t, dt);
        return a + (b - a) * frameMix;
    }

    inline float readFrame (const WavetableSet::FrameView& frame, int mipLevel,
                            float phase01, float /*dt*/) const noexcept
    {
        const int len = frame.len[mipLevel];
        const float* data = frame.mip[mipLevel];
        const float pos = phase01 * (float) len;
        const int i1 = (int) pos;
        const float frac = pos - (float) i1;
        const int i0 = (i1 - 1 + len) & (len - 1);
        const int i2 = (i1 + 1) & (len - 1);
        const int i3 = (i1 + 2) & (len - 1);
        const int idx = i1 & (len - 1);

        if (cubic)
        {
            const float c0 = -0.5f * data[i0] + 1.5f * data[idx] - 1.5f * data[i2] + 0.5f * data[i3];
            const float c1 = data[i0] - 2.5f * data[idx] + 2.0f * data[i2] - 0.5f * data[i3];
            const float c2 = -0.5f * data[i0] + 0.5f * data[i2];
            return ((c0 * frac + c1) * frac + c2) * frac + data[idx];
        }

        return data[idx] + frac * (data[i2] - data[idx]);
    }

    double sr = 48000.0;
    float phase[kMaxUnison] {};
    float triState[kMaxUnison] {};
    float ratio[kMaxUnison] {};
    float panL[kMaxUnison] {};
    float panR[kMaxUnison] {};
    float increment[kMaxUnison] {};

    const WavetableSet::FrameView* frameA = nullptr;
    const WavetableSet::FrameView* frameB = nullptr;
    XorShiftRng rng;

    float fundamental = 55.0f;
    float levelValue = 1.0f;
    float pos = 0.0f, pw = 0.5f, detune = 12.0f, spreadAmount = 0.5f, frameMix = 0.0f;
    int wave = WaveSaw, bankIndex = 0, activeUnison = 1, mip = 0;
    bool cubic = false;
};

} // namespace mk
