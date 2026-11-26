#include "DSP/ParamStore.h"
#include "DSP/MkCommon.h"

namespace mk
{

ParamStore::ParamStore() noexcept
{
    for (int i = 0; i < kParamCount; ++i)
    {
        const auto& meta = paramMeta (i);
        targetValues[(size_t) i].store (meta.def, std::memory_order_relaxed);
        smoothedValues.values[i] = meta.def;
        smoothingCoeff[(size_t) i] = 1.0f;
    }
    prepare (48000.0);
}

void ParamStore::prepare (double sampleRate) noexcept
{
    sr = sampleRate > 0.0 ? sampleRate : 48000.0;

    for (int i = 0; i < kParamCount; ++i)
    {
        // Discrete switches must change instantly; continuous values glide
        // over ~12 ms which is below the audible zip threshold at any block size.
        int choiceCount = 0;
        const bool discrete = paramChoices (i, choiceCount) != nullptr;
        smoothingCoeff[(size_t) i] = discrete ? 1.0f : (float) (32.0 / (0.012 * sr));
    }
}

void ParamStore::setByIndex (int index, float value) noexcept
{
    if (index < 0 || index >= kParamCount)
        return;
    const auto& meta = paramMeta (index);
    targetValues[(size_t) index].store (clampf (value, meta.lo, meta.hi), std::memory_order_relaxed);
}

float ParamStore::getByIndex (int index) const noexcept
{
    if (index < 0 || index >= kParamCount)
        return 0.0f;
    return targetValues[(size_t) index].load (std::memory_order_relaxed);
}

void ParamStore::setAll (const ParamSnapshot& snapshot) noexcept
{
    for (int i = 0; i < kParamCount; ++i)
        targetValues[(size_t) i].store (snapshot.values[i], std::memory_order_relaxed);
}

ParamSnapshot ParamStore::targets() const noexcept
{
    ParamSnapshot snapshot;
    for (int i = 0; i < kParamCount; ++i)
        snapshot.values[i] = targetValues[(size_t) i].load (std::memory_order_relaxed);
    return snapshot;
}

void ParamStore::resetToDefaults() noexcept
{
    ParamSnapshot defaults;
    resetSnapshotToDefaults (defaults);
    setAll (defaults);
    requestSnap();
}

void ParamStore::advance (int numSamples) noexcept
{
    if (snapRequested.exchange (false, std::memory_order_acquire))
        snapToTargets();

    const float blockScale = (float) numSamples * (1.0f / 32.0f);

    for (int i = 0; i < kParamCount; ++i)
    {
        const float target = targetValues[(size_t) i].load (std::memory_order_relaxed);
        float& value = smoothedValues.values[i];
        const float coeff = clampf (smoothingCoeff[(size_t) i] * blockScale, 0.0f, 1.0f);
        if (coeff >= 1.0f)
            value = target;
        else
            value += (target - value) * coeff;
    }
}

void ParamStore::snapToTargets() noexcept
{
    for (int i = 0; i < kParamCount; ++i)
        smoothedValues.values[i] = targetValues[(size_t) i].load (std::memory_order_relaxed);
}

} // namespace mk
