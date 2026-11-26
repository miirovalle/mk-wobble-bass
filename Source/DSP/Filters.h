#pragma once

// Multimode filter: 12/24/36 dB state-variable, HP, BP, notch, ladder, diode
// ladder, comb and formant. Coefficients are computed once per control block;
// the per-sample path is a handful of multiply-adds plus one predictable
// switch, which is what keeps 16 voices affordable on an A10X.

#include "DSP/MkCommon.h"
#include "DSP/Params.h"

namespace mk
{

class ChannelFilter
{
public:
    static int requiredCombSamples (double sampleRate) noexcept
    {
        return clampValue ((int) (sampleRate / 40.0), 64, 2048);
    }

    void prepare (double sampleRate, float* combBuffer, int combBufferSize) noexcept
    {
        sr = sampleRate > 0.0 ? sampleRate : 48000.0;
        comb = combBuffer;
        combSize = std::max (combBufferSize, 8);
        rampCoeff = 1.0f - std::exp (-1.0f / (float) (sr * 0.004));
        reset();
    }

    void reset() noexcept
    {
        for (auto& s : stages)
            s.reset();
        ladderZ.fill (0.0f);
        biquad.fill ({});
        currentCutoff = targetCutoff = -1.0f;
        combWrite = 0;
        combLp = 0.0f;
        if (comb != nullptr)
            std::fill (comb, comb + combSize, 0.0f);
    }

    void setCoefficients (int filterType, float cutoffHz, float resonance, float formantShift,
                          int vowel, int quality) noexcept
    {
        type = filterType;
        qualityLevel = quality;
        targetCutoff = cutoffHz;

        const float nyquist = (float) (sr * 0.5);
        const float fc = clampf (cutoffHz, 15.0f, nyquist * 0.92f);
        const float res = clampf (resonance, 0.0f, 0.995f);

        // Q rises from 0.5 (flat) to ~18 at full resonance.
        const float q = 0.5f + res * res * 17.5f;
        k = 1.0f / q;

        const float w = kPi * fc / (float) sr;
        g = fastTan (w);
        const float gk = g + k;
        a1 = 1.0f / (1.0f + g * gk);
        a2 = g * a1;
        a3 = g * a2;

        ladderG = g / (1.0f + g);
        ladderFeedback = res * 4.2f;

        if (type == FilterComb)
        {
            combDelay = clampValue ((int) ((float) sr / std::max (fc, 30.0f)), 2, combSize - 1);
            combFeedback = res * 0.985f;
        }
        else if (type == FilterFormant)
        {
            updateFormants (fc, formantShift, vowel);
        }
    }

    inline float process (float x, float drive) noexcept
    {
        if (drive > 0.001f)
        {
            const float gain = 1.0f + drive * 5.0f;
            x = fastTanh (x * gain) * (1.0f / (1.0f + drive * 1.8f));
        }

        switch (type)
        {
            case FilterLP24:
                return stages[1].tickLow (stages[0].tickLow (x, a1, a2, a3), a1, a2, a3);
            case FilterLP36:
                return stages[2].tickLow (
                    stages[1].tickLow (stages[0].tickLow (x, a1, a2, a3), a1, a2, a3), a1, a2, a3);
            case FilterHP:
                return stages[0].tickHigh (x, a1, a2, a3, k);
            case FilterBP:
                return stages[0].tickBand (x, a1, a2, a3);
            case FilterNotch:
                return x - k * stages[0].tickBand (x, a1, a2, a3);
            case FilterLadder:
                return ladder (x, false);
            case FilterDiode:
                return ladder (x, true);
            case FilterComb:
                return combFilter (x);
            case FilterFormant:
                return formant (x);
            case FilterSvf:
            case FilterLP12:
            default:
                return stages[0].tickLow (x, a1, a2, a3);
        }
    }

    bool isStable() const noexcept
    {
        for (const auto& s : stages)
            if (! std::isfinite (s.ic1) || ! std::isfinite (s.ic2))
                return false;
        for (const float z : ladderZ)
            if (! std::isfinite (z))
                return false;
        return true;
    }

private:
    struct Svf
    {
        float ic1 = 0.0f, ic2 = 0.0f;

        inline void reset() noexcept { ic1 = ic2 = 0.0f; }

        inline float tickLow (float x, float a1, float a2, float a3) noexcept
        {
            const float v3 = x - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = v1 + v1 - ic1;
            ic2 = v2 + v2 - ic2;
            return v2;
        }

        inline float tickBand (float x, float a1, float a2, float a3) noexcept
        {
            const float v3 = x - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = v1 + v1 - ic1;
            ic2 = v2 + v2 - ic2;
            return v1;
        }

        inline float tickHigh (float x, float a1, float a2, float a3, float kk) noexcept
        {
            const float v3 = x - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = v1 + v1 - ic1;
            ic2 = v2 + v2 - ic2;
            return x - kk * v1 - v2;
        }
    };

    struct Biquad
    {
        float b0 = 1.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float z1 = 0.0f, z2 = 0.0f;
        float gain = 1.0f;

        inline void reset() noexcept { z1 = z2 = 0.0f; }

        inline float tick (float x) noexcept
        {
            const float y = b0 * x + z1;
            z1 = b2 * x - a1 * y + z2;
            z2 = -a2 * y;
            return y * gain;
        }
    };

    inline float ladder (float x, bool diode) noexcept
    {
        const float sat = diode ? fastTanh (x * 1.2f) * 0.85f : x;
        float in = sat - ladderFeedback * ladderZ[3];
        in = fastTanh (in);

        float y = in;
        for (int i = 0; i < 4; ++i)
        {
            const float v = (y - ladderZ[(size_t) i]) * ladderG;
            y = v + ladderZ[(size_t) i];
            ladderZ[(size_t) i] = y + v;
        }

        return diode ? y * 0.8f : y;
    }

    inline float combFilter (float x) noexcept
    {
        int read = combWrite - combDelay;
        if (read < 0)
            read += combSize;

        const float delayed = comb[read];
        combLp += (delayed - combLp) * 0.35f;
        comb[combWrite] = flushDenormal (x + combLp * combFeedback);

        if (++combWrite >= combSize)
            combWrite = 0;

        return (x + delayed) * 0.7f;
    }

    inline float formant (float x) noexcept
    {
        return biquad[0].tick (x) + biquad[1].tick (x) + biquad[2].tick (x);
    }

    void updateFormants (float fc, float shiftSemitones, int vowel) noexcept
    {
        static const float centre[5][3] = {
            { 800.0f, 1150.0f, 2900.0f }, { 400.0f, 1700.0f, 2600.0f },
            { 350.0f, 2000.0f, 2800.0f }, { 450.0f, 800.0f, 2830.0f },
            { 325.0f, 700.0f, 2530.0f }
        };
        static const float level[3] = { 1.0f, 0.55f, 0.3f };
        static const float bw[3] = { 90.0f, 120.0f, 180.0f };

        const int v = clampValue (vowel, 0, 4);
        const float shift = std::exp2 (clampf (shiftSemitones, -24.0f, 24.0f) * (1.0f / 12.0f));
        // Cutoff acts as a formant tracker so the filter envelope still moves the vowel.
        const float tracker = std::pow (clampf (fc / 800.0f, 0.25f, 4.0f), 0.35f);

        for (int i = 0; i < 3; ++i)
        {
            const float f = clampf (centre[v][i] * shift * tracker, 30.0f, (float) sr * 0.45f);
            const float q = f / bw[i];
            const float w0 = kTwoPi * f / (float) sr;
            const float alpha = std::sin (w0) / (2.0f * std::max (q, 0.7f));
            const float a0 = 1.0f + alpha;

            auto& bq = biquad[(size_t) i];
            bq.b0 = alpha / a0;
            bq.b2 = -alpha / a0;
            bq.a1 = (-2.0f * std::cos (w0)) / a0;
            bq.a2 = (1.0f - alpha) / a0;
            bq.gain = level[i] * 2.2f * std::sqrt (1.0f / (1.0f + q * 0.1f));
        }
    }

    std::array<Svf, 3> stages {};
    std::array<Biquad, 3> biquad {};
    std::array<float, 4> ladderZ {};

    float* comb = nullptr;
    int combSize = 64, combWrite = 0, combDelay = 16;
    float combFeedback = 0.0f, combLp = 0.0f;

    double sr = 48000.0;
    float g = 0.0f, k = 1.0f, a1 = 1.0f, a2 = 0.0f, a3 = 0.0f;
    float ladderG = 0.0f, ladderFeedback = 0.0f;
    float rampCoeff = 0.0f;
    float currentCutoff = -1.0f, targetCutoff = -1.0f;
    int type = FilterLP12;
    int qualityLevel = 1;
};

} // namespace mk
