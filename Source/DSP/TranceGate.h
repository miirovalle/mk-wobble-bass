#pragma once

// Simple tempo-synced trance gate: sixteen on/off steps, depth and a smoothing
// control for click-free (or deliberately pumping) transitions.
//
// This is an original implementation written for this plug-in; it does not use
// Gate12 or any other third-party gating code.

#include "DSP/MkCommon.h"

#include <atomic>

namespace mk
{

class TranceGate
{
public:
    static constexpr int kSteps = 16;

    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate > 0.0 ? sampleRate : 48000.0;
        reset();
    }

    void reset() noexcept
    {
        phase = 0.0;
        step = kSteps - 1;
        gateOpen = true;
        envelope = 1.0f;
        stepDisplay.store (0, std::memory_order_relaxed);
        envelopeDisplay.store (1.0f, std::memory_order_relaxed);
        restartPending = true;
    }

    //--------------------------------------------------------------------------
    // Pattern (message thread writes, audio thread reads), one bit per step.
    uint16_t patternMask() const noexcept { return pattern.load (std::memory_order_relaxed); }

    void setPattern (uint16_t mask) noexcept
    {
        pattern.store ((uint16_t) (mask & ((1u << kSteps) - 1u)), std::memory_order_relaxed);
    }

    bool stepIsOn (int index) const noexcept
    {
        return (patternMask() & (1u << clampValue (index, 0, kSteps - 1))) != 0;
    }

    void setStep (int index, bool on) noexcept
    {
        if (index < 0 || index >= kSteps)
            return;

        const uint16_t bit = (uint16_t) (1u << index);
        uint16_t current = pattern.load (std::memory_order_relaxed);

        for (;;)
        {
            const uint16_t next = on ? (uint16_t) (current | bit) : (uint16_t) (current & ~bit);

            if (next == current || pattern.compare_exchange_weak (current, next,
                                                                  std::memory_order_relaxed))
                return;
        }
    }

    void toggleStep (int index) noexcept { setStep (index, ! stepIsOn (index)); }

    int currentStep() const noexcept { return stepDisplay.load (std::memory_order_relaxed); }
    float currentEnvelope() const noexcept { return envelopeDisplay.load (std::memory_order_relaxed); }

    //--------------------------------------------------------------------------
    void setParameters (bool enabledIn, int division, float depthIn, float smoothIn,
                        double bpm) noexcept
    {
        enabled = enabledIn;
        depth = clampf (depthIn, 0.0f, 1.0f);

        const int stepsPerBeat = division <= 0 ? 8 : (division == 1 ? 4 : (division == 2 ? 2 : 1));
        const double tempo = bpm > 1.0 ? bpm : 120.0;
        stepSamples = sr * 60.0 / tempo / (double) stepsPerBeat;

        const float smooth = clampf (smoothIn, 0.0f, 1.0f);
        const float attackMs = 0.5f + smooth * 20.0f;
        const float releaseMs = 4.0f + smooth * 120.0f;

        attackCoeff = 1.0f - std::exp (-1.0f / (attackMs * 0.001f * (float) sr));
        releaseCoeff = 1.0f - std::exp (-1.0f / (releaseMs * 0.001f * (float) sr));
    }

    void process (float* left, float* right, int numSamples) noexcept
    {
        if (! enabled)
        {
            if (envelope < 1.0f)
            {
                envelope = 1.0f;
                envelopeDisplay.store (1.0f, std::memory_order_relaxed);
            }
            return;
        }

        if (restartPending)
        {
            restartPending = false;
            step = kSteps - 1;
            phase = stepSamples;    // the first sample lands on step 0
        }

        for (int i = 0; i < numSamples; ++i)
        {
            phase += 1.0;

            if (phase >= stepSamples)
            {
                phase -= stepSamples;
                step = (step + 1) % kSteps;
                gateOpen = (pattern.load (std::memory_order_relaxed) & (1u << step)) != 0;
                stepDisplay.store (step, std::memory_order_relaxed);
            }

            const float target = gateOpen ? 1.0f : 0.0f;
            const float coeff = target > envelope ? attackCoeff : releaseCoeff;
            envelope += (target - envelope) * coeff;
            envelope = flushDenormal (envelope);

            const float gain = 1.0f - depth * (1.0f - envelope);
            left[i] *= gain;
            right[i] *= gain;

            envelopeDisplay.store (gain, std::memory_order_relaxed);
        }
    }

private:
    std::atomic<uint16_t> pattern { 0x5555 };   // classic every-other-step gate
    std::atomic<int> stepDisplay { 0 };
    std::atomic<float> envelopeDisplay { 1.0f };

    double sr = 48000.0;
    double phase = 0.0;
    double stepSamples = 12000.0;
    int step = kSteps - 1;
    bool gateOpen = true;
    bool enabled = true;
    float depth = 0.92f;
    float envelope = 1.0f;
    float attackCoeff = 0.5f;
    float releaseCoeff = 0.05f;
    bool restartPending = true;
};

} // namespace mk
