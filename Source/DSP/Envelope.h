#pragma once

// Per-voice ADSR. Linear attack (fast and click-free), exponential decay and
// release with a shape control that changes the decay target rather than doing
// a per-sample pow(). Costs three adds and one multiply per sample.
//
// Coefficients are built once per control block by the engine and shared by
// every voice, so no transcendental function runs per voice per block.

#include "DSP/MkCommon.h"

namespace mk
{

struct AdsrCoefficients
{
    float attackStep = 0.01f;
    float decayCoeff = 0.999f;
    float releaseCoeff = 0.999f;
    float sustain = 0.8f;
    float decayTarget = 0.8f;

    static AdsrCoefficients make (double sampleRate, float attackMs, float decayMs, float sustain,
                                  float releaseMs, float curve) noexcept
    {
        const float sr = (float) (sampleRate > 0.0 ? sampleRate : 48000.0);
        AdsrCoefficients c;
        c.attackStep = 1.0f / (std::max (attackMs, 0.3f) * 0.001f * sr);
        c.decayCoeff = std::exp (-1.0f / (std::max (decayMs, 2.0f) * 0.001f * sr));
        c.releaseCoeff = std::exp (-1.0f / (std::max (releaseMs, 1.0f) * 0.001f * sr));
        c.sustain = clampf (sustain, 0.0f, 1.0f);
        // Higher curve -> decay undershoots further, giving a snappier bass pluck.
        c.decayTarget = c.sustain - (1.0f - c.sustain) * clampf (curve, 0.0f, 1.0f) * 0.6f;
        return c;
    }
};

class AdsrEnvelope
{
public:
    enum class Stage : int { Idle = 0, Attack, Decay, Sustain, Release };

    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate > 0.0 ? sampleRate : 48000.0;
        coefficients = AdsrCoefficients::make (sr, 2.0f, 300.0f, 0.8f, 150.0f, 0.35f);
        reset();
    }

    void setCoefficients (const AdsrCoefficients& c) noexcept { coefficients = c; }

    void setParameters (float attackMs, float decayMs, float sustain, float releaseMs, float curve) noexcept
    {
        coefficients = AdsrCoefficients::make (sr, attackMs, decayMs, sustain, releaseMs, curve);
    }

    void reset() noexcept
    {
        stage = Stage::Idle;
        value = 0.0f;
    }

    void noteOn (bool retrigger) noexcept
    {
        if (retrigger || stage == Stage::Idle)
            value = 0.0f;
        stage = Stage::Attack;
    }

    void noteOff() noexcept
    {
        if (stage != Stage::Idle)
            stage = Stage::Release;
    }

    void quickRelease() noexcept
    {
        stage = Stage::Release;
        releaseCoeff = expCoeff (4.0f);
    }

    inline float process() noexcept
    {
        switch (stage)
        {
            case Stage::Attack:
                value += coefficients.attackStep;
                if (value >= 1.0f)
                {
                    value = 1.0f;
                    stage = Stage::Decay;
                }
                break;
            case Stage::Decay:
                value = coefficients.decayTarget
                      + (value - coefficients.decayTarget) * coefficients.decayCoeff;
                if (value <= coefficients.sustain + 1.0e-4f)
                {
                    value = coefficients.sustain;
                    stage = Stage::Sustain;
                }
                break;
            case Stage::Sustain:
                value += (coefficients.sustain - value) * 0.002f;
                break;
            case Stage::Release:
                value *= releaseCoeff;
                if (value < 1.0e-5f)
                {
                    value = 0.0f;
                    stage = Stage::Idle;
                }
                break;
            case Stage::Idle:
            default:
                value = 0.0f;
                break;
        }
        return value;
    }

    bool isActive() const noexcept { return stage != Stage::Idle; }
    bool isReleasing() const noexcept { return stage == Stage::Release; }
    float getValue() const noexcept { return value; }
    Stage getStage() const noexcept { return stage; }

private:
    float expCoeff (float ms) const noexcept
    {
        return std::exp (-1.0f / (ms * 0.001f * (float) sr));
    }

    double sr = 48000.0;
    Stage stage = Stage::Idle;
    float value = 0.0f;
    float releaseCoeff = 0.999f;
    AdsrCoefficients coefficients;
};

} // namespace mk
