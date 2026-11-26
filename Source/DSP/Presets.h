#pragma once

// Factory preset library. Presets are stored as compact override strings that
// are parsed on the message thread when a preset is chosen; the audio thread
// only ever sees the resulting parameter snapshot.
//
// 14 categories x 4 presets = 56, each with a genuinely different signal chain
// (oscillator type, filter mode, distortion algorithm, wobble shape and rate).

#include "DSP/Params.h"

namespace mk
{

struct FactoryPreset
{
    const char* category;
    const char* name;
    const char* settings;
};

int factoryPresetCount() noexcept;
const FactoryPreset& factoryPreset (int index) noexcept;
const char* const* presetCategories (int& count) noexcept;

// Starts from the parameter defaults, then applies every "id=value" override.
bool applyPresetString (const char* settings, ParamSnapshot& snapshot) noexcept;
bool applyFactoryPreset (int index, ParamSnapshot& snapshot) noexcept;

// RANDOM BASS: a seeded generator that only produces musically useful patches
// (audible wobble, valid filter, bounded level). Same seed, same patch.
void makeRandomBass (ParamSnapshot& snapshot, uint32_t seed) noexcept;

// Matching helper for the browser (case-insensitive substring).
bool presetMatches (const char* name, const char* category, const char* query) noexcept;

} // namespace mk
