#pragma once

// Modulation oscillators: four general-purpose LFOs and the wobble engine.
//
// Both advance at control rate (once per 32-sample block) which is inaudible
// for modulation and removes 32x the phase math from the audio path. The wobble
// engine additionally applies a one-pole smoother so sample-and-hold and random
// shapes never produce filter clicks.

#include "DSP/MkCommon.h"
#include "DSP/Params.h"

#include <atomic>

namespace mk
{

// Shared shape evaluation. Wobble shapes 0..8 reuse the LFO table so the two
// systems sound consistent; shape 9 (custom) morphs between two shapes.
inline float shapeValue (int shape, float phase01, float customMix) noexcept
{
    const float p = phase01 - std::floor (phase01);

    switch (shape)
    {
        case ShapeSine: return SineTable::instance().lookup (p);
        case ShapeTriangle: return 1.0f - 4.0f * std::fabs (p - 0.5f);
        case ShapeSawUp: return 2.0f * p - 1.0f;
        case ShapeSawDown: return 1.0f - 2.0f * p;
        case ShapeSquare: return p < 0.5f ? 1.0f : -1.0f;
        case ShapePulse: return p < 0.25f ? 1.0f : -1.0f;
        case ShapeRamp: return p * p * 2.0f - 1.0f;
        case ShapeCustom:
        {
            // Custom shape: sine blended into a stepped ladder, morphable.
            const float a = SineTable::instance().lookup (p);
            const float b = std::floor (p * 4.0f) * 0.5f - 0.75f;
            return a + (b - a) * clampf (customMix, 0.0f, 1.0f);
        }
        default:
            return 0.0f;
    }
}

class ModLfo
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate > 0.0 ? sampleRate : 48000.0;
        phase = 0.0f;
        fadeGain = 1.0f;
        rng.seed (0x1234u);
    }

    void setParameters (int shapeIn, float rateHz, float depthIn, float phaseOffset,
                        float offsetIn, float fadeSec, bool retrigger, double bpm, bool sync,
                        int division) noexcept
    {
        shape = shapeIn;
        depth = clampf (depthIn, 0.0f, 1.0f);
        startPhase = phaseOffset - std::floor (phaseOffset);
        offset = clampf (offsetIn, -1.0f, 1.0f);
        retrig = retrigger;

        float hz = rateHz;
        if (sync)
        {
            int count = 0;
            const auto* divisions = syncDivisions (count);
            const int index = clampValue (division, 0, count - 1);
            const float beats = divisions[index].beats;
            hz = (float) (bpm / 60.0) / std::max (beats, 0.01f);
        }

        increment = clampf (hz, 0.001f, 60.0f) * 32.0f / (float) sr;
        fadeIncrement = fadeSec > 0.001f ? 32.0f / (fadeSec * (float) sr) : 1.0f;
    }

    void trigger() noexcept
    {
        if (retrig)
            phase = startPhase;
        fadeGain = fadeIncrement >= 1.0f ? 1.0f : 0.0f;
        holdValue = rng.nextBipolar();
    }

    void advance (int numSamples) noexcept
    {
        const float scale = (float) numSamples * (1.0f / 32.0f);
        phase += increment * scale;
        if (phase >= 1.0f)
        {
            phase -= std::floor (phase);
            if (shape == LfoRandom || shape >= ShapeRandom)
                holdValue = rng.nextBipolar();
        }
        if (fadeGain < 1.0f)
            fadeGain = std::min (1.0f, fadeGain + fadeIncrement * scale);
    }

    float value() const noexcept
    {
        float v;
        if (shape == LfoRandom)
            v = holdValue;
        else
            v = shapeValue (shape, phase, 0.0f);
        return (v * depth + offset) * fadeGain;
    }

    float getPhase() const noexcept { return phase; }

private:
    double sr = 48000.0;
    float phase = 0.0f, increment = 0.0f, depth = 0.0f, startPhase = 0.0f, offset = 0.0f;
    float fadeGain = 1.0f, fadeIncrement = 1.0f, holdValue = 0.0f;
    int shape = LfoSine;
    bool retrig = false;
    XorShiftRng rng;
};

class WobbleEngine
{
public:
    // The drawable shape: 128 points the UI writes and the audio thread reads
    // through relaxed atomics, so drawing never blocks or allocates.
    static constexpr int kCustomPoints = 128;

    WobbleEngine() noexcept
    {
        for (int i = 0; i < kCustomPoints; ++i)
            custom[(size_t) i].store (std::sin (kTwoPi * (float) i / (float) kCustomPoints),
                                      std::memory_order_relaxed);
    }

    void setCustomPoint (int index, float value) noexcept
    {
        if (index < 0 || index >= kCustomPoints)
            return;

        custom[(size_t) index].store (clampf (value, -1.0f, 1.0f), std::memory_order_relaxed);
    }

    float customPoint (int index) const noexcept
    {
        return custom[(size_t) clampValue (index, 0, kCustomPoints - 1)].load (std::memory_order_relaxed);
    }

    float customAt (float phase01) const noexcept
    {
        const float p = phase01 - std::floor (phase01);
        const float position = p * (float) kCustomPoints;
        const int i0 = (int) position & (kCustomPoints - 1);
        const int i1 = (i0 + 1) & (kCustomPoints - 1);
        return lerp (custom[(size_t) i0].load (std::memory_order_relaxed),
                     custom[(size_t) i1].load (std::memory_order_relaxed),
                     position - std::floor (position));
    }

    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate > 0.0 ? sampleRate : 48000.0;
        phase = 0.0f;
        smoothed = 0.0f;
        rng.seed (0xBEEF);
    }

    void setParameters (bool freeMode, int division, float rateHz, int shapeIn, float depthIn,
                        float phaseOffset, float offsetIn, float smoothing, double bpm) noexcept
    {
        shape = shapeIn;
        depth = clampf (depthIn, 0.0f, 1.0f);
        startPhase = phaseOffset - std::floor (phaseOffset);
        offset = clampf (offsetIn, -1.0f, 1.0f);

        float hz = rateHz;
        if (! freeMode)
        {
            int count = 0;
            const auto* divisions = syncDivisions (count);
            const int index = clampValue (division, 0, count - 1);
            hz = (float) (bpm / 60.0) / std::max (divisions[index].beats, 0.01f);
        }

        increment = clampf (hz, 0.005f, 40.0f) * 32.0f / (float) sr;
        smoothCoeff = 0.02f + clampf (smoothing, 0.0f, 1.0f) * 0.5f;
    }

    void trigger() noexcept
    {
        phase = startPhase;
        holdValue = rng.nextBipolar();
    }

    void advance (int numSamples) noexcept
    {
        const float scale = (float) numSamples * (1.0f / 32.0f);
        phase += increment * scale;
        if (phase >= 1.0f)
        {
            phase -= std::floor (phase);
            if (shape == ShapeRandom || shape == ShapeSampleHold)
                holdValue = rng.nextBipolar();
        }

        float raw;
        if (shape == ShapeRandom)
            raw = holdValue;
        else if (shape == ShapeSampleHold)
            raw = std::floor (holdValue * 4.0f) * 0.25f;
        else if (shape == ShapeCustom)
            raw = customAt (phase);
        else
            raw = shapeValue (shape, phase, 0.6f);

        const float target = (raw * depth + offset);
        smoothed += (target - smoothed) * smoothCoeff;
    }

    float value() const noexcept { return smoothed; }
    float rawPhase() const noexcept { return phase; }

    static float rateInHz (bool freeMode, int division, float rateHz, double bpm) noexcept
    {
        if (freeMode)
            return rateHz;
        int count = 0;
        const auto* divisions = syncDivisions (count);
        const int index = clampValue (division, 0, count - 1);
        return (float) (bpm / 60.0) / std::max (divisions[index].beats, 0.01f);
    }

private:
    double sr = 48000.0;
    float phase = 0.0f, increment = 0.0f, depth = 0.0f, startPhase = 0.0f, offset = 0.0f;
    float smoothed = 0.0f, smoothCoeff = 0.2f, holdValue = 0.0f;
    int shape = ShapeSine;
    XorShiftRng rng;
    std::array<std::atomic<float>, (size_t) kCustomPoints> custom {};
};

} // namespace mk
