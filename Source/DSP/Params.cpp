#include "DSP/Params.h"
#include "DSP/MkCommon.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace mk
{
namespace
{

constexpr ParamMeta kMeta[] = {
#define MK_PARAM_META(id, group, lo, hi, def, unit) { #id, Group::group, lo, hi, def, Unit::unit },
    MK_PARAMS (MK_PARAM_META)
#undef MK_PARAM_META
};

static_assert (sizeof (kMeta) / sizeof (kMeta[0]) == (size_t) kParamCount,
               "parameter metadata and enum must stay in sync");

const char* const kPolyModes[] = { "Poly", "Mono", "Legato" };
const char* const kQuality[] = { "Eco", "Normal", "High" };
const char* const kOnOff[] = { "Off", "On" };
const char* const kFreeSync[] = { "Free", "Sync" };
const char* const kSyncFree[] = { "Sync", "Free" };
const char* const kBanks[] = { "Basic", "Analog", "Digital", "Metallic",
                               "Aggressive", "Formant", "Vocal", "Growl" };
const char* const kWaves[] = { "Saw", "Square", "Pulse", "Triangle", "Sine",
                               "Supersaw", "Wavetable", "FM", "Noise" };
const char* const kOsc3Waves[] = { "Sine", "Saw", "Square", "Wavetable", "Noise" };
const char* const kOsc2Modes[] = { "Layer", "FM", "Ring", "Pitch" };
const char* const kSubWaves[] = { "Sine", "Triangle", "Square" };
const char* const kSubOctaves[] = { "-1 Oct", "-2 Oct", "Root" };
const char* const kFmWaves[] = { "Sine", "Triangle", "Saw", "Square" };
const char* const kFilterTypes[] = { "LP 12", "LP 24", "LP 36", "HP", "BP", "Notch",
                                     "Comb", "Formant", "Ladder", "Diode", "SVF" };
const char* const kRoutes[] = { "Off", "Serial", "Parallel" };
const char* const kVowels[] = { "A", "E", "I", "O", "U" };
const char* const kPrePost[] = { "Post", "Pre" };
const char* const kWobbleShapes[] = { "Sine", "Triangle", "Saw Up", "Saw Down", "Square",
                                      "Pulse", "Ramp", "Random", "S&H", "Custom" };
const char* const kLfoShapes[] = { "Sine", "Triangle", "Saw Down", "Saw Up", "Square", "Random" };
const char* const kPhStages[] = { "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12" };
const char* const kDistTypes[] = { "Off", "Soft Clip", "Hard Clip", "Tube", "Diode", "Foldback",
                                   "Wavefold", "Bitcrush", "Downsample", "Digital", "Analog",
                                   "Multistage" };
const char* const kModSources[] = { "None", "LFO 1", "LFO 2", "LFO 3", "LFO 4", "Wobble",
                                    "Amp Env", "Filter Env", "Pitch Env", "Mod Env", "Velocity",
                                    "Mod Wheel", "Aftertouch", "Key Track", "Random",
                                    "Macro 1", "Macro 2", "Macro 3", "Macro 4",
                                    "Macro 5", "Macro 6", "Macro 7", "Macro 8" };
const char* const kModDests[] = { "None", "Pitch", "Cutoff", "Resonance", "WT Pos 1", "WT Pos 2",
                                  "WT Pos 3", "FM Amount", "Pulse Width", "Drive", "Width",
                                  "Sub Level", "Ring Mod", "Formant", "Osc 2 Pitch", "LFO Depth",
                                  "FX Mix", "Dist Mix", "Delay Mix", "Reverb Mix", "Chorus Mix" };

static_assert (sizeof (kModSources) / sizeof (kModSources[0]) == (size_t) kModSourceCount, "mod sources");
static_assert (sizeof (kModDests) / sizeof (kModDests[0]) == (size_t) kModDestCount, "mod dests");

const SyncDivision kDivisions[] = {
    { "1/1", 4.0f }, { "1/2", 2.0f }, { "1/4", 1.0f }, { "1/8", 0.5f },
    { "1/16", 0.25f }, { "1/32", 0.125f }, { "1/64", 0.0625f },
    { "1/2T", 4.0f / 3.0f }, { "1/4T", 2.0f / 3.0f }, { "1/8T", 1.0f / 3.0f },
    { "1/16T", 1.0f / 6.0f }, { "1/32T", 1.0f / 12.0f },
    { "1/2D", 3.0f }, { "1/4D", 1.5f }, { "1/8D", 0.75f }, { "1/16D", 0.375f },
};

constexpr int kDivisionCount = (int) (sizeof (kDivisions) / sizeof (kDivisions[0]));

const char* const* syncDivisionNames()
{
    static const char* names[kDivisionCount] {};
    static bool built = false;
    if (! built)
    {
        for (int i = 0; i < kDivisionCount; ++i)
            names[i] = kDivisions[i].name;
        built = true;
    }
    return names;
}

} // namespace

const ParamMeta& paramMeta (int index) noexcept
{
    const int clamped = clampValue (index, 0, kParamCount - 1);
    return kMeta[clamped];
}

const char* paramId (int index) noexcept
{
    return paramMeta (index).id;
}

const char* paramGroupName (Group g) noexcept
{
    static const char* const names[] = {
#define MK_GROUP_NAME(name) #name,
        MK_GROUPS (MK_GROUP_NAME)
#undef MK_GROUP_NAME
    };
    const int i = clampValue ((int) g, 0, (int) Group::kGroupCount - 1);
    return names[i];
}

const char* paramName (int index) noexcept
{
    static std::vector<std::string> names;
    if (names.empty())
    {
        names.reserve ((size_t) kParamCount);
        for (int i = 0; i < kParamCount; ++i)
        {
            const char* id = kMeta[i].id;
            std::string out;
            out.reserve (std::strlen (id) + 4);
            for (const char* c = id; *c != '\0'; ++c)
            {
                const bool isUpper = *c >= 'A' && *c <= 'Z';
                const bool isDigit = *c >= '0' && *c <= '9';
                const bool havePrev = c != id;
                const char prev = havePrev ? c[-1] : '\0';
                const bool prevIsLetter = (prev >= 'a' && prev <= 'z') || (prev >= 'A' && prev <= 'Z');
                const bool prevIsDigit = prev >= '0' && prev <= '9';

                if (havePrev && ((isUpper && (prevIsLetter || prevIsDigit))
                                 || (isDigit && prevIsLetter)))
                    out.push_back (' ');

                if (! havePrev && ! isUpper && ! isDigit)
                    out.push_back ((char) (*c - 'a' + 'A'));
                else
                    out.push_back (*c);
            }
            names.push_back (std::move (out));
        }
    }
    return names[(size_t) clampValue (index, 0, kParamCount - 1)].c_str();
}

const char* const* paramChoices (int index, int& count) noexcept
{
    const int id = clampValue (index, 0, kParamCount - 1);
    count = 0;

    switch (id)
    {
        case (int) P::polyMode: count = 3; return kPolyModes;
        case (int) P::quality: count = 3; return kQuality;
        case (int) P::oversample: count = 2; return kOnOff;
        case (int) P::adaptive: count = 2; return kOnOff;
        case (int) P::osc1Wave: case (int) P::osc2Wave: count = 9; return kWaves;
        case (int) P::osc3Wave: count = 5; return kOsc3Waves;
        case (int) P::osc1Bank: case (int) P::osc2Bank: case (int) P::osc3Bank: count = 8; return kBanks;
        case (int) P::osc2Mode: count = 4; return kOsc2Modes;
        case (int) P::subWave: count = 3; return kSubWaves;
        case (int) P::subOct: count = 3; return kSubOctaves;
        case (int) P::fmModWave: count = 4; return kFmWaves;
        case (int) P::fltType: case (int) P::flt2Type: count = 11; return kFilterTypes;
        case (int) P::fltRoute: case (int) P::distRoute: count = 3; return kRoutes;
        case (int) P::fltFormant: count = 5; return kVowels;
        case (int) P::distType: count = 12; return kDistTypes;
        case (int) P::distPre: count = 2; return kPrePost;
        case (int) P::wobMode: count = 2; return kSyncFree;
        case (int) P::wobDiv: case (int) P::dlDiv: count = kDivisionCount; return syncDivisionNames();
        case (int) P::wobShape: count = 10; return kWobbleShapes;
        case (int) P::phStages: count = 11; return kPhStages;
        case (int) P::dlSync: count = 2; return kFreeSync;
        case (int) P::chOn: case (int) P::phOn: case (int) P::flOn: case (int) P::dlOn:
        case (int) P::rvOn: case (int) P::cpOn: case (int) P::mbOn: case (int) P::dlPing:
            count = 2; return kOnOff;
        default: break;
    }

    const int lfoFirst = (int) P::lfo1Sync;
    const int lfoLast = (int) P::lfo4Retrig;
    if (index >= lfoFirst && index <= lfoLast)
    {
        switch ((index - lfoFirst) % 9)
        {
            case 0: count = 2; return kFreeSync;
            case 1: count = kDivisionCount; return syncDivisionNames();
            case 3: count = 6; return kLfoShapes;
            case 8: count = 2; return kOnOff;
            default: return nullptr;
        }
    }

    const int matFirst = (int) P::mat1Src;
    const int matLast = (int) P::mat12Amt;
    if (index >= matFirst && index <= matLast)
    {
        switch ((index - matFirst) % 3)
        {
            case 0: count = kModSourceCount; return kModSources;
            case 1: count = kModDestCount; return kModDests;
            default: return nullptr;
        }
    }

    return nullptr;
}

const char* formatParamValue (int index, float value, char* buffer, int bufferSize) noexcept
{
    int choiceCount = 0;
    if (const auto* choices = paramChoices (index, choiceCount))
    {
        const int i = clampValue ((int) std::lround (value), 0, choiceCount - 1);
        std::snprintf (buffer, (size_t) bufferSize, "%s", choices[i]);
        return buffer;
    }

    const auto& meta = paramMeta (index);
    switch (meta.unit)
    {
        case Unit::Pct:
            std::snprintf (buffer, (size_t) bufferSize, "%.0f%%", value * 100.0f);
            break;
        case Unit::Hz:
            if (value < 1000.0f)
                std::snprintf (buffer, (size_t) bufferSize, "%.1f Hz", value);
            else
                std::snprintf (buffer, (size_t) bufferSize, "%.2f kHz", value / 1000.0f);
            break;
        case Unit::Db:
            std::snprintf (buffer, (size_t) bufferSize, "%+.1f dB", value);
            break;
        case Unit::Ms:
            if (value < 1000.0f)
                std::snprintf (buffer, (size_t) bufferSize, "%.0f ms", value);
            else
                std::snprintf (buffer, (size_t) bufferSize, "%.2f s", value / 1000.0f);
            break;
        case Unit::Semi:
            std::snprintf (buffer, (size_t) bufferSize, "%.1f st", value);
            break;
        case Unit::Cents:
            std::snprintf (buffer, (size_t) bufferSize, "%.0f ct", value);
            break;
        case Unit::Sec:
            std::snprintf (buffer, (size_t) bufferSize, "%.0f ms", value * 1000.0f);
            break;
        case Unit::Ratio:
            std::snprintf (buffer, (size_t) bufferSize, "%.2fx", value);
            break;
        case Unit::Beats:
            std::snprintf (buffer, (size_t) bufferSize, "%.2f beats", value);
            break;
        case Unit::Raw:
        default:
            std::snprintf (buffer, (size_t) bufferSize, "%.2f", value);
            break;
    }
    return buffer;
}

float normaliseParam (int index, float plain) noexcept
{
    const auto& meta = paramMeta (index);
    const float clamped = clampf (plain, meta.lo, meta.hi);
    const bool logarithmic = (meta.unit == Unit::Hz || meta.unit == Unit::Ms
                              || meta.unit == Unit::Sec || meta.unit == Unit::Ratio)
                             && meta.lo > 0.0f;
    if (logarithmic)
        return std::log (clamped / meta.lo) / std::log (meta.hi / meta.lo);
    if (std::abs (meta.hi - meta.lo) < 1.0e-9f)
        return 0.0f;
    return (clamped - meta.lo) / (meta.hi - meta.lo);
}

float denormaliseParam (int index, float normalised) noexcept
{
    const auto& meta = paramMeta (index);
    const float n = clampf (normalised, 0.0f, 1.0f);
    const bool logarithmic = (meta.unit == Unit::Hz || meta.unit == Unit::Ms
                              || meta.unit == Unit::Sec || meta.unit == Unit::Ratio)
                             && meta.lo > 0.0f;
    if (logarithmic)
        return meta.lo * std::pow (meta.hi / meta.lo, n);
    return meta.lo + n * (meta.hi - meta.lo);
}

void resetSnapshotToDefaults (ParamSnapshot& snapshot) noexcept
{
    for (int i = 0; i < kParamCount; ++i)
        snapshot.values[i] = kMeta[i].def;
}

const SyncDivision* syncDivisions (int& count) noexcept
{
    count = kDivisionCount;
    return kDivisions;
}

} // namespace mk
