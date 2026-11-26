#pragma once

// Procedural, mip-mapped wavetable set.
//
// Eight banks of eight frames, each with nine band-limited mip levels whose
// lengths halve (512, 256, ... 2 samples). Total footprint is fixed at 256 KB
// for the whole instrument: nothing is streamed, decoded or loaded from disk,
// and the tables are built once at first use.

#include "DSP/MkCommon.h"

#include <array>

namespace mk
{

class WavetableSet
{
public:
    static constexpr int kBankCount = 8;
    static constexpr int kFrames = 8;
    static constexpr int kMips = 9;
    static constexpr int kMaxLen = 512;
    static constexpr int kMipSum = 1022; // 512 + 256 + ... + 2

    struct FrameView
    {
        const float* mip[kMips] {};
        int len[kMips] {};
    };

    static const WavetableSet& instance() noexcept;

    const FrameView& frame (int bank, int frameIndex) const noexcept
    {
        const int b = clampValue (bank, 0, kBankCount - 1);
        const int f = clampValue (frameIndex, 0, kFrames - 1);
        return views[(size_t) (b * kFrames + f)];
    }

    // Nanoseconds of work avoided by picking a band-limited table: mip for a
    // playback frequency, so the top harmonic always stays below Nyquist.
    static int mipForFrequency (float frequency, double sampleRate) noexcept;

    static const char* bankName (int bank) noexcept;
    static constexpr size_t memoryBytes() noexcept
    {
        return sizeof (WavetableSet);
    }

private:
    WavetableSet() noexcept;

    void buildFrame (int bank, int frameIndex, float morph) noexcept;

    std::array<float, (size_t) (kBankCount * kFrames * kMipSum)> samples {};
    std::array<FrameView, (size_t) (kBankCount * kFrames)> views {};
};

} // namespace mk
