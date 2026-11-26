#include "DSP/Presets.h"
#include "DSP/MkCommon.h"

#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace mk
{
namespace
{

// Compact override strings. Values are plain (not normalised) parameter values.
const FactoryPreset kPresets[] = {
    // ---------------------------------------------------------------- Classic Wobble
    { "Classic Wobble", "Wobble Classic",
      "osc1Wave=0;fltCutoff=520;fltReso=0.28;fltType=1;fltEnvAmt=1.2;wobDiv=13;wobDepth=0.75;"
      "wobShape=0;wobCut=0.9;subLevel=0.5;distType=1;distDrive=0.3;distMix=0.4" },
    { "Classic Wobble", "Wobble Deep Quarter",
      "osc1Wave=1;fltCutoff=340;fltReso=0.35;fltType=1;wobDiv=13;wobDepth=0.8;wobShape=0;"
      "wobCut=0.85;subLevel=0.6;distType=3;distDrive=0.35;ampD=420" },
    { "Classic Wobble", "Wobble Half Note",
      "osc1Wave=0;osc1Uni=2;osc1Detune=14;fltCutoff=420;fltType=2;wobDiv=2;wobDepth=0.7;"
      "wobShape=1;subLevel=0.45;chOn=1;chMix=0.3;distType=2;distDrive=0.25" },
    { "Classic Wobble", "Wobble Triplet Roll",
      "osc1Wave=2;osc1Pw=0.35;fltCutoff=600;fltType=1;wobDiv=9;wobDepth=0.65;wobShape=0;"
      "wobReso=0.4;subLevel=0.4;distType=10;distDrive=0.3;dlOn=1;dlMix=0.2" },

    // ---------------------------------------------------------------- Dubstep
    { "Dubstep", "Dubstep Growler",
      "osc1Wave=0;osc2Mode=1;osc2Wave=4;osc2Fm=0.55;osc2Ratio=2.5;fltCutoff=700;fltType=1;"
      "wobDiv=3;wobDepth=0.7;wobFm=0.5;subLevel=0.55;distType=3;distDrive=0.4;easyGrowl=0.6" },
    { "Dubstep", "Dubstep Yoi",
      "osc1Wave=6;osc1Bank=7;osc1Pos=0.4;osc2Mode=1;osc2Fm=0.4;osc2Wave=4;fltCutoff=900;"
      "fltReso=0.5;wobDiv=3;wobShape=2;wobDepth=0.6;wobWt=0.6;distType=5;distDrive=0.35;subLevel=0.5" },
    { "Dubstep", "Dubstep Rumble",
      "osc1Wave=0;osc1Oct=-1;fltCutoff=260;fltType=2;fltReso=0.3;wobDiv=13;wobDepth=0.55;"
      "wobSmooth=0.5;subLevel=0.7;subDrive=0.3;distType=10;distDrive=0.45;mbOn=1;mbDrive=0.4" },
    { "Dubstep", "Dubstep Snarl",
      "osc1Wave=1;osc2Mode=3;osc2Wave=0;osc2Semi=7;osc2Level=0.5;fltCutoff=800;fltType=8;"
      "fltReso=0.45;wobDiv=14;wobDepth=0.5;distType=11;distDrive=0.5;cpOn=1;cpThresh=-14" },

    // ---------------------------------------------------------------- Riddim
    { "Riddim", "Riddim Wob",
      "osc1Wave=1;fltCutoff=380;fltType=2;fltReso=0.28;wobDiv=10;wobDepth=0.65;wobShape=4;"
      "subLevel=0.55;distType=2;distDrive=0.45;distMix=0.55;cpOn=1;easyGrit=0.6" },
    { "Riddim", "Riddim Grinder",
      "osc1Wave=1;osc1Oct=-1;osc2Mode=1;osc2Wave=4;osc2Fm=0.7;osc2Ratio=1.5;fltCutoff=420;"
      "wobDiv=10;wobShape=4;wobFm=0.6;distType=7;distDrive=0.5;subLevel=0.5" },
    { "Riddim", "Riddim Chug",
      "osc1Wave=2;osc1Pw=0.28;fltCutoff=300;fltType=1;fltEnvAmt=2.4;fltD=90;fltS=0.05;"
      "wobDiv=4;wobDepth=0.5;subLevel=0.6;distType=3;distDrive=0.5;ampD=180" },
    { "Riddim", "Riddim Machine",
      "osc1Wave=6;osc1Bank=2;osc1Pos=0.6;fltCutoff=520;fltType=8;wobDiv=14;wobShape=5;wobDepth=0.6;"
      "wobWt=0.7;distType=8;distDrive=0.4;mbOn=1;mbDrive=0.5" },

    // ---------------------------------------------------------------- Heavy Growl
    { "Heavy Growl", "Growl Beast",
      "osc1Wave=0;osc2Mode=1;osc2Wave=4;osc2Fm=0.75;osc2Ratio=3;fltCutoff=650;fltType=9;"
      "fltReso=0.4;wobDiv=3;wobFm=0.55;lfo1Depth=1;mat1Src=2;mat1Dst=7;mat1Amt=0.45;distType=3;distDrive=0.5" },
    { "Heavy Growl", "Growl Sawtooth",
      "osc1Wave=0;osc2Mode=1;osc2Fm=0.6;osc2Wave=4;fltCutoff=780;fltType=1;fltDrive=0.4;"
      "wobDiv=9;wobShape=3;wobDepth=0.5;distType=1;distDrive=0.4;mbOn=1;mbDrive=0.35" },
    { "Heavy Growl", "Growl Larynx",
      "osc1Wave=0;osc2Mode=1;osc2Fm=0.5;fltType=7;fltFormant=3;fltCutoff=900;wobDiv=4;"
      "wobFormant=0.6;wobDepth=0.6;distType=4;distDrive=0.45;subLevel=0.5" },
    { "Heavy Growl", "Growl Destroyer",
      "osc1Wave=1;osc2Mode=2;osc2Wave=1;osc2Ring=0.6;osc2Level=0.4;fltCutoff=560;fltType=2;"
      "wobDiv=15;wobDepth=0.55;distType=5;distDrive=0.55;distTone=0.4;cpOn=1;cpThresh=-16" },

    // ---------------------------------------------------------------- Neurofunk
    { "Neurofunk", "Neuro Roller",
      "osc1Wave=0;osc2Mode=1;osc2Wave=4;osc2Fm=0.45;osc2Ratio=4;fltCutoff=700;fltType=1;"
      "wobDiv=3;wobDepth=0.5;lfo1Depth=1;lfo1Shape=2;mat1Src=1;mat1Dst=13;mat1Amt=0.6;"
      "distType=11;distDrive=0.45;subLevel=0.45" },
    { "Neurofunk", "Neuro Skitter",
      "osc1Wave=6;osc1Bank=3;osc1Pos=0.5;fltCutoff=900;fltType=3;flt2Type=4;fltRoute=1;"
      "flt2Cutoff=2400;wobDiv=15;wobShape=8;wobDepth=0.5;distType=8;distDrive=0.4;dlOn=1;dlMix=0.2" },
    { "Neurofunk", "Neuro Bass Line",
      "osc1Wave=0;osc1Uni=3;osc1Detune=22;osc1Spread=0.8;osc2Mode=3;osc2Wave=0;osc2Level=0.35;"
      "osc2Semi=12;fltCutoff=600;fltType=8;wobDiv=14;wobDepth=0.45;distType=10;distDrive=0.4;rvOn=1;rvMix=0.15" },
    { "Neurofunk", "Neuro Vice",
      "osc1Wave=1;osc2Mode=1;osc2Fm=0.8;osc2Ratio=6;fltCutoff=1050;fltType=9;fltReso=0.55;"
      "wobDiv=4;wobFm=0.5;distType=7;distDrive=0.5;distMix=0.6;cpOn=1;cpRatio=6" },

    // ---------------------------------------------------------------- Reese Bass
    { "Reese Bass", "Reese Classic",
      "osc1Wave=0;osc1Uni=2;osc1Detune=26;osc1Spread=0.7;osc2Wave=0;osc2Level=0.7;osc2Detune=20;"
      "osc2Oct=-1;fltCutoff=480;fltType=1;wobDepth=0.25;wobDiv=2;subLevel=0.55;distType=3;distDrive=0.25" },
    { "Reese Bass", "Reese Wide",
      "osc1Wave=0;osc1Uni=4;osc1Detune=30;osc1Spread=1;fltCutoff=620;fltType=2;wobDepth=0.2;"
      "wobDiv=2;chOn=1;chMix=0.4;chWidth=0.9;subLevel=0.5;easyWidth=0.65" },
    { "Reese Bass", "Reese Dread",
      "osc1Wave=0;osc1Oct=-1;osc1Uni=3;osc1Detune=18;fltCutoff=280;fltType=8;fltReso=0.35;"
      "wobDepth=0.3;wobDiv=2;subLevel=0.65;subDrive=0.25;distType=10;distDrive=0.35" },
    { "Reese Bass", "Reese Moving",
      "osc1Wave=0;osc1Uni=2;osc1Detune=24;fltCutoff=520;fltType=1;lfo1Depth=0.8;lfo1Div=4;"
      "mat1Src=1;mat1Dst=2;mat1Amt=0.35;wobDepth=0.3;subLevel=0.55;mbOn=1" },

    // ---------------------------------------------------------------- FM Bass
    { "FM Bass", "FM Growl Bass",
      "osc1Wave=4;osc2Mode=1;osc2Wave=4;osc2Fm=0.6;osc2Ratio=2;fltCutoff=900;fltType=1;"
      "wobDepth=0.4;wobFm=0.5;wobDiv=3;distType=1;distDrive=0.3;subLevel=0.45" },
    { "FM Bass", "FM Bell Bass",
      "osc1Wave=4;osc2Mode=1;osc2Wave=4;osc2Fm=0.5;osc2Ratio=3.5;osc1Level=0.7;fltCutoff=1400;"
      "fltType=0;ampD=600;ampS=0.6;subLevel=0.4;rvOn=1;rvMix=0.2;rvSize=0.6" },
    { "FM Bass", "FM Metal",
      "osc1Wave=4;osc2Mode=1;osc2Wave=3;osc2Fm=0.7;osc2Ratio=5;fltCutoff=1800;fltType=4;"
      "fltReso=0.4;wobDepth=0.35;wobFm=0.4;distType=9;distDrive=0.35;subLevel=0.4" },
    { "FM Bass", "FM Feedback Growl",
      "osc1Wave=4;fmFeedback=0.5;osc2Mode=1;osc2Wave=4;osc2Fm=0.65;osc2Ratio=1.5;fltCutoff=800;"
      "fltType=8;fltReso=0.4;wobDepth=0.45;wobDiv=9;distType=11;distDrive=0.4" },

    // ---------------------------------------------------------------- Metallic Bass
    { "Metallic Bass", "Metal Grinder",
      "osc1Wave=6;osc1Bank=3;osc1Pos=0.35;osc2Mode=2;osc2Wave=6;osc2Bank=3;osc2Ring=0.5;"
      "osc2Level=0.4;fltCutoff=1200;fltType=6;wobDepth=0.4;distType=5;distDrive=0.4" },
    { "Metallic Bass", "Metal Clank",
      "osc1Wave=6;osc1Bank=2;osc1Pos=0.7;fltCutoff=1600;fltType=10;fltReso=0.5;wobDepth=0.3;"
      "wobWt=0.5;distType=8;distDrive=0.45;chOn=1;chMix=0.25;subLevel=0.35" },
    { "Metallic Bass", "Metal Pipe",
      "osc1Wave=6;osc1Bank=3;osc1Pos=0.6;osc3Wave=0;osc3Level=0.3;osc3Semi=19;fltCutoff=900;"
      "fltType=6;fltReso=0.6;wobDepth=0.35;distType=9;distDrive=0.3;rvOn=1;rvMix=0.18" },
    { "Metallic Bass", "Metal Shard",
      "osc1Wave=6;osc1Bank=4;osc1Pos=0.5;osc2Mode=1;osc2Wave=4;osc2Fm=0.55;osc2Ratio=2.7;"
      "fltCutoff=1300;fltType=3;wobDepth=0.4;distType=6;distDrive=0.4;mbOn=1" },

    // ---------------------------------------------------------------- Talking Bass
    { "Talking Bass", "Talk Vowel A",
      "osc1Wave=0;fltType=7;fltFormant=0;fltCutoff=800;wobDepth=0.5;wobDiv=9;wobFormant=0.7;"
      "subLevel=0.5;distType=3;distDrive=0.35" },
    { "Talking Bass", "Talk Vowel E",
      "osc1Wave=6;osc1Bank=6;osc1Pos=0.2;fltType=7;fltFormant=1;fltCutoff=900;wobDepth=0.45;"
      "wobFormant=0.6;wobDiv=14;distType=4;distDrive=0.35;subLevel=0.45" },
    { "Talking Bass", "Talk Vocal Chop",
      "osc1Wave=6;osc1Bank=6;osc1Pos=0.55;fltType=7;fltFormant=3;fltCutoff=700;wobShape=8;"
      "wobDepth=0.6;wobDiv=10;wobFormant=0.8;distType=1;distDrive=0.3" },
    { "Talking Bass", "Talk Yoi Vowel",
      "osc1Wave=0;fltType=7;fltFormant=4;fltCutoff=1000;osc2Mode=1;osc2Fm=0.4;osc2Wave=4;"
      "wobDepth=0.5;wobDiv=3;wobFormant=0.5;distType=11;distDrive=0.4" },

    // ---------------------------------------------------------------- Formant Bass
    { "Formant Bass", "Formant Sweep",
      "osc1Wave=0;fltType=7;fltFormant=0;fltCutoff=600;lfo1Depth=1;lfo1Div=2;lfo1Shape=0;"
      "mat1Src=1;mat1Dst=13;mat1Amt=0.8;subLevel=0.5;distType=1;distDrive=0.25" },
    { "Formant Bass", "Formant Robot",
      "osc1Wave=2;osc1Pw=0.4;fltType=7;fltFormant=2;fltCutoff=850;wobShape=8;wobDepth=0.5;"
      "wobDiv=15;wobFormant=0.9;distType=9;distDrive=0.3;mbOn=1" },
    { "Formant Bass", "Formant Choir",
      "osc1Wave=6;osc1Bank=6;osc1Pos=0.4;fltType=7;fltFormant=1;fltCutoff=700;wobDepth=0.35;"
      "wobDiv=2;rvOn=1;rvMix=0.25;rvSize=0.7;dlOn=1;dlMix=0.15" },
    { "Formant Bass", "Formant Growl",
      "osc1Wave=0;osc2Mode=1;osc2Fm=0.45;osc2Wave=4;fltType=7;fltFormant=3;fltCutoff=750;"
      "wobDepth=0.55;wobDiv=4;wobFormant=0.7;distType=3;distDrive=0.45" },

    // ---------------------------------------------------------------- Bass House
    { "Bass House", "House Sub Bounce",
      "osc1Wave=4;osc1Level=0.5;osc2Wave=0;osc2Level=0.4;osc2Oct=-1;fltCutoff=400;fltType=1;"
      "subLevel=0.55;ampA=3;ampD=160;ampS=0.5;ampR=90;wobDepth=0.2" },
    { "Bass House", "House Stab Bass",
      "osc1Wave=1;fltCutoff=650;fltType=1;fltEnvAmt=2.6;fltD=120;fltS=0.02;ampD=200;ampS=0.25;"
      "subLevel=0.4;distType=2;distDrive=0.3;dlOn=1;dlMix=0.18" },
    { "Bass House", "House Dirty",
      "osc1Wave=0;osc1Uni=2;osc1Detune=12;fltCutoff=520;fltType=2;wobDepth=0.3;wobDiv=9;"
      "distType=3;distDrive=0.4;subLevel=0.5;cpOn=1;cpThresh=-16;cpMakeup=4" },
    { "Bass House", "House Womp",
      "osc1Wave=0;fltCutoff=430;fltType=1;fltReso=0.4;wobDepth=0.5;wobDiv=3;wobShape=1;"
      "subLevel=0.55;distType=10;distDrive=0.3;chOn=1;chMix=0.2" },

    // ---------------------------------------------------------------- Drum and Bass
    { "Drum and Bass", "DnB Roller",
      "osc1Wave=0;osc1Uni=2;osc1Detune=16;fltCutoff=560;fltType=1;wobDepth=0.4;wobDiv=14;"
      "subLevel=0.6;distType=3;distDrive=0.3;cpOn=1;cpThresh=-14" },
    { "Drum and Bass", "DnB Neuro",
      "osc1Wave=0;osc2Mode=1;osc2Fm=0.5;osc2Wave=4;osc2Ratio=3.2;fltCutoff=800;fltType=9;"
      "wobDepth=0.35;wobDiv=10;distType=11;distDrive=0.45;subLevel=0.5" },
    { "Drum and Bass", "DnB Liquid Sub",
      "osc1Wave=4;osc1Level=0.4;osc2Wave=0;osc2Level=0.45;fltCutoff=320;fltType=0;subLevel=0.7;"
      "ampD=700;ampS=0.7;wobDepth=0.12;cpOn=1;cpRatio=3" },
    { "Drum and Bass", "DnB Jump Up",
      "osc1Wave=2;osc1Pw=0.3;fltCutoff=700;fltType=1;fltEnvAmt=1.8;wobDepth=0.4;wobDiv=9;"
      "distType=4;distDrive=0.4;subLevel=0.55;mbOn=1;mbDrive=0.45" },

    // ---------------------------------------------------------------- Sub Bass
    { "Sub Bass", "Sub Pure",
      "osc1Wave=4;osc1Level=0.35;osc1Oct=-1;subLevel=0.85;subWave=0;fltCutoff=180;fltType=0;"
      "wobDepth=0.0;distType=0;cpOn=0;polyMode=1;glideTime=60" },
    { "Sub Bass", "Sub Warm",
      "osc1Wave=4;osc1Level=0.4;osc1Oct=-1;subLevel=0.75;subDrive=0.2;fltCutoff=220;fltType=0;"
      "distType=3;distDrive=0.15;distMix=0.25;cpOn=0;wobDepth=0.0" },
    { "Sub Bass", "Sub Dusty",
      "osc1Wave=4;osc1Oct=-1;subWave=1;subLevel=0.7;fltCutoff=260;fltType=0;distType=10;"
      "distDrive=0.25;distMix=0.35;polyMode=1;glideTime=120;wobDepth=0.0;ampA=8" },
    { "Sub Bass", "Sub Wide",
      "osc1Wave=4;osc1Oct=-1;osc1Level=0.3;osc2Wave=4;osc2Level=0.3;osc2Fine=8;subLevel=0.7;"
      "fltCutoff=200;fltType=0;chOn=1;chMix=0.15;chWidth=0.5;wobDepth=0.0;cpOn=0" },

    // ---------------------------------------------------------------- Experimental
    { "Experimental", "Random Machine",
      "osc1Wave=6;osc1Bank=3;osc1Pos=0.3;fltType=3;fltCutoff=1100;wobShape=8;wobDiv=15;"
      "wobDepth=0.7;wobWt=0.8;distType=7;distDrive=0.5;dlOn=1;dlMix=0.3;rvOn=1;rvMix=0.2" },
    { "Experimental", "Wavetable Drift",
      "osc1Wave=6;osc1Bank=1;osc1Pos=0.5;lfo1Depth=1;lfo1Rate=0.15;mat1Src=1;mat1Dst=4;"
      "mat1Amt=0.7;lfo2Depth=1;lfo2Rate=0.07;mat2Src=2;mat2Dst=3;mat2Amt=0.3;subLevel=0.45" },
    { "Experimental", "Granular Sub",
      "osc1Wave=1;osc1Oct=-1;subLevel=0.8;subDrive=0.6;fltCutoff=180;fltType=2;wobShape=8;"
      "wobDiv=5;wobDepth=0.8;wobSmooth=0.1;distType=8;distDrive=0.5;polyMode=1" },
    { "Experimental", "Broken Radio",
      "osc1Wave=6;osc1Bank=2;osc1Pos=0.8;fltType=4;fltCutoff=1300;wobShape=9;wobDiv=11;"
      "wobDepth=0.6;distType=7;distDrive=0.6;distMix=0.7;phOn=1;phMix=0.3;rvOn=1;rvMix=0.25" },

    // ---------------------------------------------------------------- Signature
    // The six EASY-page characters as finished patches: pick one, press play,
    // turn BASS / GRIT / WOBBLE to taste.
    { "Signature", "WM Wobble",
      "bassChar=0;morphChar=2;wobDiv=13;wobDepth=0.8;wobCut=0.9;fltCutoff=420;fltType=1;subLevel=0.55" },
    { "Signature", "WM Growl",
      "bassChar=1;morphChar=4;wobDiv=3;osc1Wave=0;fltCutoff=520;fltType=8;subLevel=0.5" },
    { "Signature", "WM Reese",
      "bassChar=2;osc1Wave=0;osc1Uni=4;osc1Detune=28;wobDiv=2;fltCutoff=480;fltType=1;subLevel=0.55" },
    { "Signature", "WM Neuro",
      "bassChar=3;osc1Wave=6;osc1Bank=3;osc1Pos=0.55;wobDiv=14;fltCutoff=700;fltType=9;subLevel=0.45" },
    { "Signature", "WM Scream",
      "bassChar=4;osc1Wave=1;wobDiv=9;fltCutoff=900;fltType=9;subLevel=0.4" },
    { "Signature", "WM Talking",
      "bassChar=5;osc1Wave=6;osc1Bank=6;osc1Pos=0.3;wobDiv=9;fltCutoff=600;formantMix=0.5;subLevel=0.5" },
};

constexpr int kPresetCount = (int) (sizeof (kPresets) / sizeof (kPresets[0]));

int indexOfParam (const char* name) noexcept
{
    for (int i = 0; i < kParamCount; ++i)
        if (std::strcmp (paramId (i), name) == 0)
            return i;
    return -1;
}

struct SimpleRng
{
    uint32_t state;

    uint32_t next() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    float unit() noexcept { return (float) (next() >> 8) * (1.0f / 16777216.0f); }
    float range (float lo, float hi) noexcept { return lo + (hi - lo) * unit(); }
    int range (int lo, int hi) noexcept { return lo + (int) (next() % (uint32_t) (hi - lo + 1)); }
    bool chance (float p) noexcept { return unit() < p; }
};

} // namespace

int factoryPresetCount() noexcept
{
    return kPresetCount;
}

const FactoryPreset& factoryPreset (int index) noexcept
{
    return kPresets[clampValue (index, 0, kPresetCount - 1)];
}

const char* const* presetCategories (int& count) noexcept
{
    static const char* names[16] {};
    static int built = -1;
    if (built < 0)
    {
        int unique = 0;
        for (int i = 0; i < kPresetCount && unique < 16; ++i)
        {
            bool seen = false;
            for (int j = 0; j < unique; ++j)
                if (std::strcmp (names[j], kPresets[i].category) == 0)
                    seen = true;
            if (! seen)
                names[unique++] = kPresets[i].category;
        }
        built = unique;
    }
    count = built;
    return names;
}

bool applyPresetString (const char* settings, ParamSnapshot& snapshot) noexcept
{
    if (settings == nullptr)
        return false;

    const char* cursor = settings;
    while (*cursor != '\0')
    {
        const char* end = cursor;
        while (*end != '\0' && *end != ';')
            ++end;

        const char* equals = cursor;
        while (equals < end && *equals != '=')
            ++equals;

        if (equals > cursor && equals < end)
        {
            char key[32] {};
            const size_t length = std::min ((size_t) (equals - cursor), sizeof (key) - 1);
            std::memcpy (key, cursor, length);

            const int index = indexOfParam (key);
            if (index >= 0)
            {
                char valueText[32] {};
                const size_t valueLength = std::min ((size_t) (end - equals - 1), sizeof (valueText) - 1);
                std::memcpy (valueText, equals + 1, valueLength);
                snapshot.values[index] = (float) std::atof (valueText);
            }
        }

        cursor = *end == ';' ? end + 1 : end;
    }

    return true;
}

bool applyFactoryPreset (int index, ParamSnapshot& snapshot) noexcept
{
    resetSnapshotToDefaults (snapshot);
    return applyPresetString (factoryPreset (index).settings, snapshot);
}

void makeRandomBass (ParamSnapshot& snapshot, uint32_t seed) noexcept
{
    resetSnapshotToDefaults (snapshot);
    SimpleRng rng { seed == 0 ? 0x9E3779B9u : seed };

    snapshot[P::bassChar] = (float) rng.range (0, kCharacterCount - 1);
    snapshot[P::morphChar] = (float) rng.range (0, kCharacterCount - 1);
    snapshot[P::bassMorph] = rng.chance (0.45f) ? 0.0f : rng.range (0.0f, 0.6f);

    static const int waves[] = { WaveSaw, WaveSquare, WavePulse, WaveTriangle,
                                 WaveSupersaw, WaveWavetable, WaveFM };
    snapshot[P::osc1Wave] = (float) waves[rng.range (0, 6)];
    snapshot[P::osc1Bank] = (float) rng.range (0, 7);
    snapshot[P::osc1Pos] = rng.range (0.15f, 0.8f);
    snapshot[P::osc1Uni] = (float) rng.range (1, 4);
    snapshot[P::osc1Detune] = rng.range (6.0f, 30.0f);
    snapshot[P::osc1Spread] = rng.range (0.3f, 0.9f);
    snapshot[P::osc1Pw] = rng.range (0.25f, 0.75f);

    static const int filters[] = { FilterLP24, FilterLP36, FilterLadder, FilterDiode,
                                   FilterSvf, FilterComb };
    snapshot[P::fltType] = (float) filters[rng.range (0, 5)];
    snapshot[P::fltCutoff] = 160.0f * std::exp2 (rng.range (0.6f, 2.6f));
    snapshot[P::fltReso] = rng.range (0.2f, 0.55f);
    snapshot[P::fltDrive] = rng.range (0.1f, 0.5f);
    snapshot[P::fltEnvAmt] = rng.range (0.6f, 2.8f);

    static const int divisions[] = { 2, 3, 4, 9, 10, 13, 14, 15 };
    snapshot[P::wobMode] = 0.0f;   // SMART WOBBLE: always locked to the host
    snapshot[P::wobDiv] = (float) divisions[rng.range (0, 7)];
    snapshot[P::wobDepth] = rng.range (0.55f, 0.9f);
    snapshot[P::wobShape] = (float) rng.range (0, 5);
    snapshot[P::wobCut] = rng.range (0.6f, 1.0f);
    snapshot[P::wobFm] = rng.range (0.0f, 0.6f);
    snapshot[P::wobWt] = rng.range (0.0f, 0.7f);
    snapshot[P::wobReso] = rng.range (0.0f, 0.5f);
    snapshot[P::wobDrive] = rng.range (0.0f, 0.5f);

    static const int distortionTypes[] = { DistSoftClip, DistHardClip, DistTube, DistDiode,
                                           DistFoldback, DistWavefold, DistDigital,
                                           DistMultistage };
    snapshot[P::distType] = (float) distortionTypes[rng.range (0, 7)];
    snapshot[P::distDrive] = rng.range (0.2f, 0.6f);
    snapshot[P::distMix] = rng.range (0.3f, 0.7f);
    snapshot[P::distRoute] = 1.0f;

    snapshot[P::subLevel] = rng.range (0.35f, 0.75f);
    snapshot[P::subOct] = (float) rng.range (0, 1);
    snapshot[P::subLock] = 1.0f;

    snapshot[P::easyWobble] = rng.range (0.4f, 0.75f);
    snapshot[P::easyBass] = rng.range (0.4f, 0.7f);
    snapshot[P::easyGrowl] = rng.range (0.35f, 0.7f);
    snapshot[P::easyGrit] = rng.range (0.4f, 0.7f);
    snapshot[P::easyMovement] = rng.range (0.35f, 0.65f);
    snapshot[P::easyWidth] = rng.range (0.35f, 0.65f);

    snapshot[P::ampA] = rng.range (1.0f, 8.0f);
    snapshot[P::ampD] = rng.range (120.0f, 700.0f);
    snapshot[P::ampS] = rng.range (0.5f, 0.95f);
    snapshot[P::ampR] = rng.range (60.0f, 260.0f);
    snapshot[P::outputDb] = rng.range (-6.0f, -2.0f);

    if (rng.chance (0.35f))
    {
        snapshot[P::dlOn] = 1.0f;
        snapshot[P::dlMix] = rng.range (0.12f, 0.3f);
        snapshot[P::dlDiv] = (float) rng.range (10, 15);
    }
    if (rng.chance (0.25f))
    {
        snapshot[P::rvOn] = 1.0f;
        snapshot[P::rvMix] = rng.range (0.1f, 0.25f);
    }
    if (rng.chance (0.3f))
    {
        snapshot[P::chOn] = 1.0f;
        snapshot[P::chMix] = rng.range (0.1f, 0.35f);
    }

    snapshot[P::cpOn] = 1.0f;
    snapshot[P::gateOn] = 0.0f;
}

bool presetMatches (const char* name, const char* category, const char* query) noexcept
{
    if (query == nullptr || *query == '\0')
        return true;

    const auto matches = [] (const char* haystack, const char* needle)
    {
        const size_t needleLength = std::strlen (needle);
        if (needleLength == 0)
            return true;

        for (const char* h = haystack; *h != '\0'; ++h)
        {
            size_t i = 0;
            while (i < needleLength && h[i] != '\0'
                   && std::tolower ((unsigned char) h[i]) == std::tolower ((unsigned char) needle[i]))
                ++i;
            if (i == needleLength)
                return true;
        }
        return false;
    };

    return matches (name, query) || matches (category, query);
}

} // namespace mk
