#pragma once

// 100 ready-made trance gate rhythms. Each preset is a 16-step pattern written
// as a binary string (most significant bit = step 1) plus the rate, depth and
// smoothing it was designed around.

#include "DSP/MkCommon.h"

#include <cstdint>

namespace mk
{

struct GatePreset
{
    const char* name;
    const char* bits;      // 16 characters, '1' = step open
    int division;          // 0 = 1/32, 1 = 1/16, 2 = 1/8, 3 = 1/4
    float depth;           // 0..1
    float smooth;          // 0..1
};

int gatePresetCount() noexcept;
const GatePreset& gatePreset (int index) noexcept;
uint16_t gatePresetMask (int index) noexcept;

} // namespace mk
