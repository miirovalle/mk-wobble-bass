#include "DSP/Wavetables.h"

#include <cmath>

namespace mk
{
namespace
{

struct HarmonicSpec
{
    float amplitude;
    float phase;
};

float hash01 (int k) noexcept
{
    uint32_t x = (uint32_t) k * 0x9E3779B9u;
    x ^= x >> 15;
    x *= 0x85EBCA6Bu;
    x ^= x >> 13;
    return (float) (x & 0xFFFFFFu) * (1.0f / 16777216.0f);
}

float gaussian (float x, float centre, float width) noexcept
{
    const float d = (x - centre) / width;
    return std::exp (-0.5f * d * d);
}

// One spectral recipe per bank: evaluated only for the top mip, which is then
// decimated into the band-limited lower levels.
HarmonicSpec spectrum (int bank, int k, float t) noexcept
{
    const float kf = (float) k;

    switch (bank)
    {
        case 0: // Basic: sine -> saw, a little odd-harmonic bite at the top
        {
            if (k == 1)
                return { 1.0f, 0.0f };
            const float saw = 1.0f / kf;
            const float odd = (k & 1) ? 0.25f / kf : 0.0f;
            return { saw * (0.15f + 0.85f * t) + odd * t * t, 0.0f };
        }
        case 1: // Analog: warm saw, gentle rolloff, slight phase smear
        {
            const float rolloff = 1.0f - 0.4f * t * kf * (1.0f / 256.0f);
            return { (1.0f / kf) * std::max (0.0f, rolloff), 0.06f * t * kf };
        }
        case 2: // Digital: comb-shaped, hollow and glassy
        {
            const float comb = 0.35f + 0.65f * std::fabs (std::cos (kPi * kf * (0.06f + 0.5f * t)));
            return { comb / std::pow (kf, 0.9f), 0.15f * t * kf };
        }
        case 3: // Metallic: inharmonic phase scatter over a bright base
        {
            const float scatter = (hash01 (k * 7 + 3) * 2.0f - 1.0f) * t * kTwoPi;
            const float amp = (0.5f + 0.5f * std::cos (kf * 2.4f)) / std::pow (kf, 0.65f);
            return { amp, scatter };
        }
        case 4: // Aggressive: moving resonant peak over a dense saw
        {
            const float centre = 4.0f + 30.0f * t;
            const float width = 2.0f + 8.0f * t;
            const float peak = 1.0f + 6.0f * gaussian (kf, centre, width);
            return { peak / std::pow (kf, 0.8f), 0.0f };
        }
        case 5: // Formant: three formant regions sliding with morph
        {
            const float f1 = 3.0f + 5.0f * t;
            const float f2 = 10.0f + 10.0f * t;
            const float f3 = 18.0f + 14.0f * t;
            const float amp = 4.0f * gaussian (kf, f1, 1.8f)
                            + 2.5f * gaussian (kf, f2, 2.6f)
                            + 1.4f * gaussian (kf, f3, 3.4f)
                            + 0.25f / kf;
            return { amp, 0.0f };
        }
        case 6: // Vocal: vowel-like, morphing from an open A to a closed U
        {
            const float a1 = lerp (5.0f, 2.2f, t);
            const float a2 = lerp (13.0f, 6.0f, t);
            const float a3 = lerp (24.0f, 12.0f, t);
            const float amp = 3.4f * gaussian (kf, a1, 1.5f)
                            + 2.2f * gaussian (kf, a2, 2.2f)
                            + 1.1f * gaussian (kf, a3, 3.0f);
            return { amp, 0.0f };
        }
        case 7: // Growl: dense bass with phase warping for a talking edge
        default:
        {
            const float amp = 1.0f / std::pow (kf, 0.5f);
            return { amp, kf * t * 1.2f };
        }
    }
}

} // namespace

int WavetableSet::mipForFrequency (float frequency, double sampleRate) noexcept
{
    const float nyquistHarmonics = (float) (sampleRate * 0.5) / std::max (frequency, 1.0e-3f);
    if (nyquistHarmonics >= 256.0f)
        return 0;

    const float level = 8.0f - std::log2 (std::max (nyquistHarmonics, 1.0f));
    return clampValue ((int) std::ceil (level), 0, kMips - 1);
}

const char* WavetableSet::bankName (int bank) noexcept
{
    static const char* const names[kBankCount] = { "Basic", "Analog", "Digital", "Metallic",
                                                   "Aggressive", "Formant", "Vocal", "Growl" };
    return names[clampValue (bank, 0, kBankCount - 1)];
}

WavetableSet::WavetableSet() noexcept
{
    for (int bank = 0; bank < kBankCount; ++bank)
    {
        for (int f = 0; f < kFrames; ++f)
        {
            buildFrame (bank, f, (float) f / (float) (kFrames - 1));

            FrameView& view = views[(size_t) (bank * kFrames + f)];
            float* base = samples.data() + (size_t) (bank * kFrames + f) * (size_t) kMipSum;
            int offset = 0;
            for (int m = 0; m < kMips; ++m)
            {
                const int len = kMaxLen >> m;
                view.mip[m] = base + offset;
                view.len[m] = len;
                offset += len;
            }
        }
    }
}

void WavetableSet::buildFrame (int bank, int frameIndex, float morph) noexcept
{
    float* base = samples.data() + (size_t) (bank * kFrames + frameIndex) * (size_t) kMipSum;
    const SineTable& sine = SineTable::instance();

    // Top mip: full 256-harmonic spectrum at 512 samples.
    std::fill (base, base + kMaxLen, 0.0f);
    for (int k = 1; k <= kMaxLen / 2; ++k)
    {
        const HarmonicSpec spec = spectrum (bank, k, morph);
        if (std::fabs (spec.amplitude) < 1.0e-4f)
            continue;

        const float phaseOffset = spec.phase * (1.0f / kTwoPi);
        for (int n = 0; n < kMaxLen; ++n)
            base[n] += spec.amplitude * sine.lookup ((float) k * (float) n / (float) kMaxLen + phaseOffset);
    }

    // Lower mips: binomial low-pass + decimate by two. One 4-tap pass per level
    // keeps start-up cheap; a full per-mip resynthesis would be nine times the work.
    int offset = kMaxLen;
    int prevLen = kMaxLen;
    const float* prev = base;
    for (int m = 1; m < kMips; ++m)
    {
        const int len = prevLen >> 1;
        float* cur = base + offset;
        for (int n = 0; n < len; ++n)
        {
            const int i = 2 * n;
            const float a = prev[(i - 1 + prevLen) & (prevLen - 1)];
            const float b = prev[i & (prevLen - 1)];
            const float c = prev[(i + 1) & (prevLen - 1)];
            const float d = prev[(i + 2) & (prevLen - 1)];
            cur[n] = (a + 3.0f * b + 3.0f * c + d) * 0.125f;
        }
        offset += len;
        prev = cur;
        prevLen = len;
    }

    // Normalise every frame to the same peak so morphing never jumps in level.
    float peak = 0.0f;
    for (int n = 0; n < kMaxLen; ++n)
        peak = std::max (peak, std::fabs (base[n]));

    if (peak > 1.0e-6f)
    {
        const float gain = 0.95f / peak;
        int written = 0;
        for (int m = 0; m < kMips; ++m)
        {
            const int len = kMaxLen >> m;
            float* dst = base + written;
            for (int n = 0; n < len; ++n)
                dst[n] *= gain;
            written += len;
        }
    }
}

const WavetableSet& WavetableSet::instance() noexcept
{
    static const WavetableSet set;
    return set;
}

} // namespace mk
