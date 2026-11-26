#pragma once

// Parameter storage shared by the UI/host thread and the audio thread.
//
// * Targets are plain atomics: any thread may write them at any time.
// * Smoothed values live on the audio thread and are advanced once per control
//   block (32 samples), so zipper noise is removed without per-sample cost.
// * Loading a preset raises a flag; the audio thread performs the snap itself,
//   so no lock is ever taken.

#include "DSP/Params.h"

#include <array>
#include <atomic>

namespace mk
{

class ParamStore
{
public:
    ParamStore() noexcept;

    void prepare (double sampleRate) noexcept;

    void set (P p, float value) noexcept { setByIndex ((int) p, value); }
    void setByIndex (int index, float value) noexcept;
    float get (P p) const noexcept { return getByIndex ((int) p); }
    float getByIndex (int index) const noexcept;

    void setAll (const ParamSnapshot& snapshot) noexcept;
    ParamSnapshot targets() const noexcept;

    void resetToDefaults() noexcept;
    void requestSnap() noexcept { snapRequested.store (true, std::memory_order_release); }

    // Moves the smoothed values towards their targets. Called once per control
    // block from the audio thread.
    void advance (int numSamples) noexcept;

    void snapToTargets() noexcept;

    const ParamSnapshot& smoothed() const noexcept { return smoothedValues; }

    double sampleRate() const noexcept { return sr; }

private:
    std::array<std::atomic<float>, (size_t) kParamCount> targetValues;
    ParamSnapshot smoothedValues {};
    std::array<float, (size_t) kParamCount> smoothingCoeff {};
    double sr = 48000.0;
    std::atomic<bool> snapRequested { false };
};

} // namespace mk
