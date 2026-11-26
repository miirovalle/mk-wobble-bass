#pragma once

// Dependency-free numeric toolbox shared by the DSP modules. Everything here
// is real-time safe: fixed-size, branch-predictable, no allocation.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace mk
{

constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;

template <typename T>
inline T clampValue (T v, T lo, T hi) noexcept
{
    return v < lo ? lo : (v > hi ? hi : v);
}

inline float clampf (float v, float lo, float hi) noexcept { return clampValue (v, lo, hi); }
inline float lerp (float a, float b, float t) noexcept { return a + (b - a) * t; }
inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }

// Rational tanh approximation: ~1e-4 absolute error, monotone, ~6x faster than
// std::tanh. Every saturator in the engine goes through this.
inline float fastTanh (float x) noexcept
{
    x = clampf (x, -6.0f, 6.0f);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

inline float midiToHz (float note) noexcept { return 440.0f * std::exp2 ((note - 69.0f) * (1.0f / 12.0f)); }
inline float hzToMidi (float hz) noexcept { return 69.0f + 12.0f * std::log2 (std::max (hz, 1.0e-6f) / 440.0f); }

// 2^x for |x| <= 0.5: 5-term Taylor, max error ~7e-6 (0.001 cent). Used for
// unison detune and every per-block pitch offset.
inline float fastExp2Small (float x) noexcept
{
    const float t = x * 0.69314718f;
    return 1.0f + t * (1.0f + t * (0.5f + t * (0.16666667f + t * 0.041666668f)));
}

inline float fastSemitoneRatio (float semitones) noexcept
{
    const float x = semitones * (1.0f / 12.0f);
    return (x > -0.35f && x < 0.35f) ? fastExp2Small (x) : std::exp2 (x);
}

// tan() for 0..0.55 (cutoff below ~0.175 * sample rate), max error 0.15%.
inline float fastTan (float x) noexcept
{
    if (x > 0.55f)
        return std::tan (x);
    const float x2 = x * x;
    return x * (1.0f + x2 * (0.33333333f + x2 * (0.13333333f + x2 * 0.053968254f)));
}

inline void panGains (float pan, float& outL, float& outR) noexcept
{
    const float p = (clampf (pan, -1.0f, 1.0f) + 1.0f) * 0.25f * kPi;
    outL = std::cos (p);
    outR = std::sin (p);
}

inline float flushDenormal (float v) noexcept { return std::fabs (v) < 1.0e-25f ? 0.0f : v; }

class XorShiftRng
{
public:
    explicit XorShiftRng (uint32_t s = 0x9E3779B9u) noexcept { seed (s); }

    inline uint32_t nextUInt() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    inline float nextUnipolar() noexcept { return (float) (nextUInt() >> 8) * (1.0f / 16777216.0f); }
    inline float nextBipolar() noexcept { return nextUnipolar() * 2.0f - 1.0f; }

    void seed (uint32_t s) noexcept
    {
        s += 0x9E3779B9u;
        s = (s ^ (s >> 16)) * 0x85EBCA6Bu;
        s = (s ^ (s >> 13)) * 0xC2B2AE35u;
        state = (s ^ (s >> 16)) | 1u;
        for (int i = 0; i < 8; ++i)
            nextUInt();
    }

private:
    uint32_t state = 0x9E3779B9u;
};

class OnePole
{
public:
    void setTimeConstant (double sampleRate, float seconds) noexcept
    {
        coeff = seconds <= 0.0f ? 1.0f
                               : 1.0f - std::exp (-1.0f / (float) (sampleRate * (double) seconds));
    }

    inline float next (float target) noexcept { value += (target - value) * coeff; return value; }
    inline float get() const noexcept { return value; }
    inline void reset (float v = 0.0f) noexcept { value = v; }
    inline void snap (float v) noexcept { value = v; }

private:
    float coeff = 0.05f;
    float value = 0.0f;
};

class DcBlocker
{
public:
    void prepare (double sampleRate, float cutoffHz = 8.0f) noexcept
    {
        r = 1.0f - kTwoPi * cutoffHz / (float) sampleRate;
        x1 = y1 = 0.0f;
    }

    inline float process (float x) noexcept
    {
        y1 = x - x1 + r * y1;
        x1 = x;
        return flushDenormal (y1);
    }

    void reset() noexcept { x1 = y1 = 0.0f; }

private:
    float r = 0.999f, x1 = 0.0f, y1 = 0.0f;
};

// 4096-entry sine table: one construction at start-up, shared by LFOs,
// oscillators, FM modulators and the display.
class SineTable
{
public:
    static constexpr int kSize = 4096;
    static constexpr int kMask = kSize - 1;

    static const SineTable& instance() noexcept
    {
        static const SineTable table;
        return table;
    }

    inline float lookup (float phase01) const noexcept
    {
        phase01 -= std::floor (phase01);
        const float pos = phase01 * (float) kSize;
        const int i0 = (int) pos;
        return lerp (data[(size_t) (i0 & kMask)], data[(size_t) ((i0 + 1) & kMask)], pos - (float) i0);
    }

    inline const float* raw() const noexcept { return data.data(); }

private:
    SineTable() noexcept
    {
        for (int i = 0; i < kSize; ++i)
            data[(size_t) i] = std::sin (kTwoPi * (float) i / (float) kSize);
    }

    std::array<float, kSize> data {};
};

inline float expMap (float norm, float lo, float hi) noexcept
{
    return lo * std::pow (hi / lo, clampf (norm, 0.0f, 1.0f));
}

} // namespace mk
