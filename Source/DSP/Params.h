#pragma once

// Single source of truth for every plug-in parameter. The X-macro list builds
// the enum, the metadata table, the default snapshot and the host automation
// layout, so index order can never drift between the engine, the UI and the
// JUCE wrapper.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace mk
{

#define MK_GROUPS(E)                                                                              \
    E(Glob)                                                                                       \
    E(Osc1)                                                                                       \
    E(Osc2)                                                                                       \
    E(Osc3)                                                                                       \
    E(Sub)                                                                                        \
    E(Fm)                                                                                         \
    E(Filt)                                                                                       \
    E(Wob)                                                                                        \
    E(Lfo)                                                                                        \
    E(Env)                                                                                        \
    E(Dist)                                                                                       \
    E(Fx)                                                                                         \
    E(Mac)                                                                                        \
    E(Mat)                                                                                        \
    E(Misc)

enum class Group : int
{
#define MK_GROUP_ENUM(name) name,
    MK_GROUPS (MK_GROUP_ENUM)
#undef MK_GROUP_ENUM
        kGroupCount
};

enum class Unit : int { Raw, Pct, Hz, Db, Ms, Semi, Cents, Sec, Ratio, Beats };

using U = Unit;

// clang-format off
#define MK_PARAMS(X)                                                                              \
    X(polyMode,     Glob, 0.0f,    2.0f,     0.0f,   Raw)   /* Poly / Mono / Legato */            \
    X(polyphony,    Glob, 1.0f,    16.0f,    8.0f,   Raw)                                          \
    X(glideTime,    Glob, 0.0f,    1500.0f,  40.0f,  Ms)                                           \
    X(quality,      Glob, 0.0f,    2.0f,     1.0f,   Raw)   /* Eco / Normal / High */              \
    X(oversample,   Glob, 0.0f,    1.0f,     0.0f,   Raw)   /* Off / 2x */                         \
    X(outputDb,     Glob, -24.0f,  6.0f,    -3.0f,   Db)                                           \
    X(bendRange,    Glob, 1.0f,    24.0f,    2.0f,   Semi)                                         \
    X(adaptive,     Glob, 0.0f,    1.0f,     1.0f,   Raw)                                          \
    X(velSens,      Glob, 0.0f,    1.0f,     0.6f,   Pct)                                          \
    X(mix,          Glob, 0.0f,    1.0f,     0.3f,   Pct)                                          \
                                                                                                   \
    X(osc1Wave,     Osc1, 0.0f,    8.0f,     0.0f,   Raw)                                          \
    X(osc1Bank,     Osc1, 0.0f,    7.0f,     2.0f,   Raw)                                          \
    X(osc1Pos,      Osc1, 0.0f,    1.0f,     0.25f,  Pct)                                          \
    X(osc1Oct,      Osc1, -3.0f,   3.0f,     0.0f,   Raw)                                          \
    X(osc1Semi,     Osc1, -12.0f,  12.0f,    0.0f,   Semi)                                         \
    X(osc1Fine,     Osc1, -50.0f,  50.0f,    0.0f,   Cents)                                        \
    X(osc1Level,    Osc1, 0.0f,    1.0f,     0.85f,  Pct)                                          \
    X(osc1Uni,      Osc1, 1.0f,    7.0f,     1.0f,   Raw)                                          \
    X(osc1Detune,   Osc1, 0.0f,    60.0f,    12.0f,  Cents)                                        \
    X(osc1Spread,   Osc1, 0.0f,    1.0f,     0.6f,   Pct)                                          \
    X(osc1Phase,    Osc1, 0.0f,    1.0f,     0.0f,   Pct)                                          \
    X(osc1Pw,       Osc1, 0.05f,   0.95f,    0.5f,   Pct)                                          \
                                                                                                   \
    X(osc2Mode,     Osc2, 0.0f,    3.0f,     0.0f,   Raw)   /* Layer/FM/Ring/Pitch */              \
    X(osc2Wave,     Osc2, 0.0f,    8.0f,     0.0f,   Raw)                                          \
    X(osc2Bank,     Osc2, 0.0f,    7.0f,     3.0f,   Raw)                                          \
    X(osc2Pos,      Osc2, 0.0f,    1.0f,     0.4f,   Pct)                                          \
    X(osc2Oct,      Osc2, -3.0f,   3.0f,     0.0f,   Raw)                                          \
    X(osc2Semi,     Osc2, -12.0f,  12.0f,    0.0f,   Semi)                                         \
    X(osc2Fine,     Osc2, -50.0f,  50.0f,    0.0f,   Cents)                                        \
    X(osc2Level,    Osc2, 0.0f,    1.0f,     0.0f,   Pct)                                          \
    X(osc2Uni,      Osc2, 1.0f,    7.0f,     1.0f,   Raw)                                          \
    X(osc2Detune,   Osc2, 0.0f,    60.0f,    10.0f,  Cents)                                        \
    X(osc2Spread,   Osc2, 0.0f,    1.0f,     0.5f,   Pct)                                          \
    X(osc2Pw,       Osc2, 0.05f,   0.95f,    0.5f,   Pct)                                          \
    X(osc2Ratio,    Osc2, 0.25f,   8.0f,     1.0f,   Ratio)                                        \
    X(osc2Fm,       Osc2, 0.0f,    1.0f,     0.0f,   Pct)                                          \
    X(osc2Ring,     Osc2, 0.0f,    1.0f,     0.0f,   Pct)                                          \
                                                                                                   \
    X(osc3Wave,     Osc3, 0.0f,    4.0f,     4.0f,   Raw)   /* Sine/Saw/Square/WT/Noise */         \
    X(osc3Bank,     Osc3, 0.0f,    7.0f,     5.0f,   Raw)                                          \
    X(osc3Pos,      Osc3, 0.0f,    1.0f,     0.5f,   Pct)                                          \
    X(osc3Oct,      Osc3, -3.0f,   3.0f,     0.0f,   Raw)                                          \
    X(osc3Semi,     Osc3, -12.0f,  12.0f,    0.0f,   Semi)                                         \
    X(osc3Level,    Osc3, 0.0f,    1.0f,     0.0f,   Pct)                                          \
                                                                                                   \
    X(subWave,      Sub,  0.0f,    2.0f,     0.0f,   Raw)   /* Sine/Tri/Square */                  \
    X(subOct,       Sub,  0.0f,    2.0f,     0.0f,   Raw)   /* -1 / -2 / 0 */                      \
    X(subLevel,     Sub,  0.0f,    1.0f,     0.4f,   Pct)                                          \
    X(subDrive,     Sub,  0.0f,    1.0f,     0.0f,   Pct)                                          \
                                                                                                   \
    X(fmFeedback,   Fm,   0.0f,    1.0f,     0.0f,   Pct)                                          \
    X(fmModWave,    Fm,   0.0f,    3.0f,     0.0f,   Raw)   /* Sine/Tri/Saw/Square */              \
                                                                                                   \
    X(fltType,      Filt, 0.0f,    10.0f,    1.0f,   Raw)                                          \
    X(fltCutoff,    Filt, 20.0f,   18000.0f, 320.0f, Hz)                                           \
    X(fltReso,      Filt, 0.0f,    1.0f,     0.22f,  Pct)                                          \
    X(fltDrive,     Filt, 0.0f,    1.0f,     0.15f,  Pct)                                          \
    X(fltKeyTrack,  Filt, 0.0f,    1.0f,     0.35f,  Pct)                                          \
    X(fltEnvAmt,    Filt, -4.0f,   4.0f,     1.6f,   Semi)                                         \
    X(fltMix,       Filt, 0.0f,    1.0f,     1.0f,   Pct)                                          \
    X(fltRoute,     Filt, 0.0f,    2.0f,     0.0f,   Raw)   /* Off / Serial / Parallel */          \
    X(flt2Type,     Filt, 0.0f,    10.0f,    0.0f,   Raw)                                          \
    X(flt2Cutoff,   Filt, 20.0f,   18000.0f, 1400.0f, Hz)                                          \
    X(flt2Reso,     Filt, 0.0f,    1.0f,     0.1f,   Pct)                                          \
    X(fltFormant,   Filt, 0.0f,    4.0f,     0.0f,   Raw)   /* A E I O U */                        \
                                                                                                   \
    X(wobMode,      Wob,  0.0f,    1.0f,     0.0f,   Raw)   /* Sync / Free */                      \
    X(wobDiv,       Wob,  0.0f,    15.0f,    2.0f,   Raw)                                          \
    X(wobRate,      Wob,  0.02f,   32.0f,    2.0f,   Hz)                                           \
    X(wobDepth,     Wob,  0.0f,    1.0f,     0.55f,  Pct)                                          \
    X(wobShape,     Wob,  0.0f,    9.0f,     0.0f,   Raw)                                          \
    X(wobPhase,     Wob,  0.0f,    1.0f,     0.0f,   Pct)                                          \
    X(wobOffset,    Wob, -1.0f,    1.0f,     0.0f,   Raw)                                          \
    X(wobSmooth,    Wob,  0.0f,    1.0f,     0.25f,  Pct)                                          \
    X(wobCut,       Wob, -1.0f,    1.0f,     0.85f,  Raw)                                          \
    X(wobWt,        Wob, -1.0f,    1.0f,     0.35f,  Raw)                                          \
    X(wobFm,        Wob, -1.0f,    1.0f,     0.3f,   Raw)                                          \
    X(wobDrive,     Wob, -1.0f,    1.0f,     0.4f,   Raw)                                          \
    X(wobPitch,     Wob, -1.0f,    1.0f,     0.0f,   Raw)                                          \
    X(wobPw,        Wob, -1.0f,    1.0f,     0.0f,   Raw)                                          \
    X(wobReso,      Wob, -1.0f,    1.0f,     0.15f,  Raw)                                          \
    X(wobWidth,     Wob, -1.0f,    1.0f,     0.2f,   Raw)                                          \
    X(wobFormant,   Wob, -1.0f,    1.0f,     0.0f,   Raw)                                          \
                                                                                                   \
    X(lfo1Sync,     Lfo,  0.0f,    1.0f,     1.0f,   Raw)                                          \
    X(lfo1Div,      Lfo,  0.0f,    15.0f,    2.0f,   Raw)                                          \
    X(lfo1Rate,     Lfo,  0.01f,   30.0f,    1.0f,   Hz)                                           \
    X(lfo1Shape,    Lfo,  0.0f,    5.0f,     0.0f,   Raw)                                          \
    X(lfo1Depth,    Lfo,  0.0f,    1.0f,     0.0f,   Pct)                                          \
    X(lfo1Phase,    Lfo,  0.0f,    1.0f,     0.0f,   Pct)                                          \
    X(lfo1Offset,   Lfo, -1.0f,    1.0f,     0.0f,   Raw)                                          \
    X(lfo1Fade,     Lfo,  0.0f,    5.0f,     0.0f,   Sec)                                          \
    X(lfo1Retrig,   Lfo,  0.0f,    1.0f,     1.0f,   Raw)                                          \
    X(lfo2Sync,     Lfo,  0.0f,    1.0f,     1.0f,   Raw)                                          \
    X(lfo2Div,      Lfo,  0.0f,    15.0f,    9.0f,   Raw)                                          \
    X(lfo2Rate,     Lfo,  0.01f,   30.0f,    0.5f,   Hz)                                           \
    X(lfo2Shape,    Lfo,  0.0f,    5.0f,     1.0f,   Raw)                                          \
    X(lfo2Depth,    Lfo,  0.0f,    1.0f,     0.0f,   Pct)                                          \
    X(lfo2Phase,    Lfo,  0.0f,    1.0f,     0.25f,  Pct)                                          \
    X(lfo2Offset,   Lfo, -1.0f,    1.0f,     0.0f,   Raw)                                          \
    X(lfo2Fade,     Lfo,  0.0f,    5.0f,     0.5f,   Sec)                                          \
    X(lfo2Retrig,   Lfo,  0.0f,    1.0f,     0.0f,   Raw)                                          \
    X(lfo3Sync,     Lfo,  0.0f,    1.0f,     0.0f,   Raw)                                          \
    X(lfo3Div,      Lfo,  0.0f,    15.0f,    4.0f,  Raw)                                          \
    X(lfo3Rate,     Lfo,  0.01f,   30.0f,    4.5f,   Hz)                                           \
    X(lfo3Shape,    Lfo,  0.0f,    5.0f,     3.0f,   Raw)                                          \
    X(lfo3Depth,    Lfo,  0.0f,    1.0f,     0.0f,   Pct)                                          \
    X(lfo3Phase,    Lfo,  0.0f,    1.0f,     0.5f,   Pct)                                          \
    X(lfo3Offset,   Lfo, -1.0f,    1.0f,     0.0f,   Raw)                                          \
    X(lfo3Fade,     Lfo,  0.0f,    5.0f,     1.0f,   Sec)                                          \
    X(lfo3Retrig,   Lfo,  0.0f,    1.0f,     0.0f,   Raw)                                          \
    X(lfo4Sync,     Lfo,  0.0f,    1.0f,     1.0f,   Raw)                                          \
    X(lfo4Div,      Lfo,  0.0f,    15.0f,    15.0f,  Raw)                                          \
    X(lfo4Rate,     Lfo,  0.01f,   30.0f,    0.2f,   Hz)                                           \
    X(lfo4Shape,    Lfo,  0.0f,    5.0f,     5.0f,   Raw)                                          \
    X(lfo4Depth,    Lfo,  0.0f,    1.0f,     0.0f,   Pct)                                          \
    X(lfo4Phase,    Lfo,  0.0f,    1.0f,     0.0f,   Pct)                                          \
    X(lfo4Offset,   Lfo, -1.0f,    1.0f,     0.0f,   Raw)                                          \
    X(lfo4Fade,     Lfo,  0.0f,    5.0f,     2.0f,   Sec)                                          \
    X(lfo4Retrig,   Lfo,  0.0f,    1.0f,     0.0f,   Raw)                                          \
                                                                                                   \
    X(ampA,         Env,  0.3f,    4000.0f,  2.0f,   Ms)                                           \
    X(ampD,         Env,  2.0f,    8000.0f,  320.0f, Ms)                                           \
    X(ampS,         Env,  0.0f,    1.0f,     0.85f,  Pct)                                          \
    X(ampR,         Env,  1.0f,    8000.0f,  140.0f, Ms)                                           \
    X(ampCurve,     Env,  0.0f,    1.0f,     0.35f,  Pct)                                          \
    X(fltA,         Env,  0.3f,    4000.0f,  1.0f,   Ms)                                           \
    X(fltD,         Env,  2.0f,    8000.0f,  260.0f, Ms)                                           \
    X(fltS,         Env,  0.0f,    1.0f,     0.25f,  Pct)                                          \
    X(fltR,         Env,  1.0f,    8000.0f,  160.0f, Ms)                                           \
    X(pitA,         Env,  0.3f,    4000.0f,  1.0f,   Ms)                                           \
    X(pitD,         Env,  2.0f,    8000.0f,  90.0f,  Ms)                                           \
    X(pitS,         Env,  0.0f,    1.0f,     0.0f,   Pct)                                          \
    X(pitR,         Env,  1.0f,    8000.0f,  100.0f, Ms)                                           \
    X(pitAmt,       Env, -24.0f,   24.0f,    0.0f,   Semi)                                         \
    X(modA,         Env,  0.3f,    4000.0f,  6.0f,   Ms)                                           \
    X(modD,         Env,  2.0f,    8000.0f,  500.0f, Ms)                                           \
    X(modS,         Env,  0.0f,    1.0f,     0.3f,   Pct)                                          \
    X(modR,         Env,  1.0f,    8000.0f,  300.0f, Ms)                                           \
                                                                                                   \
    X(distType,     Dist, 0.0f,    11.0f,    0.0f,   Raw)                                          \
    X(distDrive,    Dist, 0.0f,    1.0f,     0.3f,   Pct)                                          \
    X(distTone,     Dist, -1.0f,   1.0f,     0.0f,   Raw)                                          \
    X(distMix,      Dist, 0.0f,    1.0f,     0.5f,   Pct)                                          \
    X(distOut,      Dist, -24.0f,  6.0f,     0.0f,   Db)                                           \
    X(distPre,      Dist, 0.0f,    1.0f,     0.0f,   Raw)   /* 0 = post filter, 1 = pre */         \
    X(distRoute,    Dist, 0.0f,    2.0f,     0.0f,   Raw)   /* Off / Serial / Parallel */          \
                                                                                                   \
    X(chOn,         Fx,   0.0f,    1.0f,     0.0f,   Raw)                                          \
    X(chRate,       Fx,   0.02f,   8.0f,     0.6f,   Hz)                                           \
    X(chDepth,      Fx,   0.0f,    1.0f,     0.4f,   Pct)                                          \
    X(chMix,        Fx,   0.0f,    1.0f,     0.3f,   Pct)                                          \
    X(chWidth,      Fx,   0.0f,    1.0f,     0.7f,   Pct)                                          \
    X(phOn,         Fx,   0.0f,    1.0f,     0.0f,   Raw)                                          \
    X(phRate,       Fx,   0.02f,   8.0f,     0.3f,   Hz)                                           \
    X(phDepth,      Fx,   0.0f,    1.0f,     0.6f,   Pct)                                          \
    X(phFb,         Fx,   0.0f,    0.95f,    0.4f,   Pct)                                          \
    X(phStages,     Fx,   2.0f,    12.0f,    4.0f,   Raw)                                          \
    X(phMix,        Fx,   0.0f,    1.0f,     0.3f,   Pct)                                          \
    X(flOn,         Fx,   0.0f,    1.0f,     0.0f,   Raw)                                          \
    X(flRate,       Fx,   0.02f,   8.0f,     0.25f,  Hz)                                           \
    X(flDepth,      Fx,   0.0f,    1.0f,     0.5f,   Pct)                                          \
    X(flFb,         Fx,   0.0f,    0.95f,    0.4f,   Pct)                                          \
    X(flDelay,      Fx,   0.0005f, 0.02f,    0.004f, Sec)                                          \
    X(flMix,        Fx,   0.0f,    1.0f,     0.3f,   Pct)                                          \
    X(dlOn,         Fx,   0.0f,    1.0f,     0.0f,   Raw)                                          \
    X(dlSync,       Fx,   0.0f,    1.0f,     1.0f,   Raw)                                          \
    X(dlDiv,        Fx,   0.0f,    15.0f,    14.0f,  Raw)                                          \
    X(dlTime,       Fx,   0.02f,   1.5f,     0.375f, Sec)                                          \
    X(dlFb,         Fx,   0.0f,    0.95f,    0.35f,  Pct)                                          \
    X(dlPing,       Fx,   0.0f,    1.0f,     1.0f,   Raw)                                          \
    X(dlFilter,     Fx,   0.0f,    1.0f,     0.35f,  Pct)                                          \
    X(dlMix,        Fx,   0.0f,    1.0f,     0.25f,  Pct)                                          \
    X(rvOn,         Fx,   0.0f,    1.0f,     0.0f,   Raw)                                          \
    X(rvSize,       Fx,   0.0f,    1.0f,     0.5f,   Pct)                                          \
    X(rvDecay,      Fx,   0.0f,    1.0f,     0.5f,   Pct)                                          \
    X(rvDamp,       Fx,   0.0f,    1.0f,     0.5f,   Pct)                                          \
    X(rvWidth,      Fx,   0.0f,    1.0f,     0.8f,   Pct)                                          \
    X(rvMix,        Fx,   0.0f,    1.0f,     0.2f,   Pct)                                          \
    X(cpOn,         Fx,   0.0f,    1.0f,     1.0f,   Raw)                                          \
    X(cpThresh,     Fx,   -40.0f,  0.0f,    -12.0f,  Db)                                           \
    X(cpRatio,      Fx,   1.0f,    20.0f,    4.0f,   Ratio)                                        \
    X(cpAttack,     Fx,   0.1f,    100.0f,   6.0f,   Ms)                                           \
    X(cpRelease,    Fx,   5.0f,    1000.0f,  120.0f, Ms)                                           \
    X(cpMakeup,     Fx,   0.0f,    18.0f,    2.0f,   Db)                                           \
    X(mbOn,         Fx,   0.0f,    1.0f,     0.0f,   Raw)                                          \
    X(mbDrive,      Fx,   0.0f,    1.0f,     0.3f,   Pct)                                          \
    X(mbMix,        Fx,   0.0f,    1.0f,     0.5f,   Pct)                                          \
                                                                                                   \
    X(macBass,      Mac,  0.0f,    1.0f,     0.0f,   Pct)                                          \
    X(macGrowl,     Mac,  0.0f,    1.0f,     0.0f,   Pct)                                          \
    X(macMovement,  Mac,  0.0f,    1.0f,     0.0f,   Pct)                                          \
    X(macDrive,     Mac,  0.0f,    1.0f,     0.0f,   Pct)                                          \
    X(macFilter,    Mac,  0.0f,    1.0f,     0.0f,   Pct)                                          \
    X(macSub,       Mac,  0.0f,    1.0f,     0.0f,   Pct)                                          \
    X(macWidth,     Mac,  0.0f,    1.0f,     0.0f,   Pct)                                          \
    X(macMix,       Mac,  0.0f,    1.0f,     0.0f,   Pct)                                          \
                                                                                                   \
    X(mat1Src,      Mat,  0.0f,    21.0f,    0.0f,   Raw)                                          \
    X(mat1Dst,      Mat,  0.0f,    20.0f,    0.0f,   Raw)                                          \
    X(mat1Amt,      Mat, -1.0f,    1.0f,     0.0f,   Raw)                                          \
    X(mat2Src,      Mat,  0.0f,    21.0f,    0.0f,   Raw)                                          \
    X(mat2Dst,      Mat,  0.0f,    20.0f,    0.0f,   Raw)                                          \
    X(mat2Amt,      Mat, -1.0f,    1.0f,     0.0f,   Raw)                                          \
    X(mat3Src,      Mat,  0.0f,    21.0f,    0.0f,   Raw)                                          \
    X(mat3Dst,      Mat,  0.0f,    20.0f,    0.0f,   Raw)                                          \
    X(mat3Amt,      Mat, -1.0f,    1.0f,     0.0f,   Raw)                                          \
    X(mat4Src,      Mat,  0.0f,    21.0f,    0.0f,   Raw)                                          \
    X(mat4Dst,      Mat,  0.0f,    20.0f,    0.0f,   Raw)                                          \
    X(mat4Amt,      Mat, -1.0f,    1.0f,     0.0f,   Raw)                                          \
    X(mat5Src,      Mat,  0.0f,    21.0f,    0.0f,   Raw)                                          \
    X(mat5Dst,      Mat,  0.0f,    20.0f,    0.0f,   Raw)                                          \
    X(mat5Amt,      Mat, -1.0f,    1.0f,     0.0f,   Raw)                                          \
    X(mat6Src,      Mat,  0.0f,    21.0f,    0.0f,   Raw)                                          \
    X(mat6Dst,      Mat,  0.0f,    20.0f,    0.0f,   Raw)                                          \
    X(mat6Amt,      Mat, -1.0f,    1.0f,     0.0f,   Raw)                                          \
    X(mat7Src,      Mat,  0.0f,    21.0f,    0.0f,   Raw)                                          \
    X(mat7Dst,      Mat,  0.0f,    20.0f,    0.0f,   Raw)                                          \
    X(mat7Amt,      Mat, -1.0f,    1.0f,     0.0f,   Raw)                                          \
    X(mat8Src,      Mat,  0.0f,    21.0f,    0.0f,   Raw)                                          \
    X(mat8Dst,      Mat,  0.0f,    20.0f,    0.0f,   Raw)                                          \
    X(mat8Amt,      Mat, -1.0f,    1.0f,     0.0f,   Raw)                                          \
    X(mat9Src,      Mat,  0.0f,    21.0f,    0.0f,   Raw)                                          \
    X(mat9Dst,      Mat,  0.0f,    20.0f,    0.0f,   Raw)                                          \
    X(mat9Amt,      Mat, -1.0f,    1.0f,     0.0f,   Raw)                                          \
    X(mat10Src,     Mat,  0.0f,    21.0f,    0.0f,   Raw)                                          \
    X(mat10Dst,     Mat,  0.0f,    20.0f,    0.0f,   Raw)                                          \
    X(mat10Amt,     Mat, -1.0f,    1.0f,     0.0f,   Raw)                                          \
    X(mat11Src,     Mat,  0.0f,    21.0f,    0.0f,   Raw)                                          \
    X(mat11Dst,     Mat,  0.0f,    20.0f,    0.0f,   Raw)                                          \
    X(mat11Amt,     Mat, -1.0f,    1.0f,     0.0f,   Raw)                                          \
    X(mat12Src,     Mat,  0.0f,    21.0f,    0.0f,   Raw)                                          \
    X(mat12Dst,     Mat,  0.0f,    20.0f,    0.0f,   Raw)                                          \
    X(mat12Amt,     Mat, -1.0f,    1.0f,     0.0f,   Raw)                                          \
                                                                                                   \
    X(drift,        Misc, 0.0f,    1.0f,     0.15f,  Pct)
// clang-format on

enum class P : int
{
#define MK_PARAM_ENUM(id, group, lo, hi, def, unit) id,
    MK_PARAMS (MK_PARAM_ENUM)
#undef MK_PARAM_ENUM
        kCount
};

constexpr int kParamCount = (int) P::kCount;
constexpr int kMatrixSlots = 12;
constexpr int kLfoCount = 4;
constexpr int kMacroCount = 8;

struct ParamSnapshot
{
    float values[kParamCount] {};

    inline float operator[] (P p) const noexcept { return values[(int) p]; }
    inline float& operator[] (P p) noexcept { return values[(int) p]; }
    inline int asInt (P p) const noexcept { return (int) values[(int) p]; }
    inline bool asBool (P p) const noexcept { return values[(int) p] > 0.5f; }
};

struct ParamMeta
{
    const char* id;
    Group group;
    float lo, hi, def;
    Unit unit;
};

const ParamMeta& paramMeta (int index) noexcept;
const char* paramId (int index) noexcept;
const char* paramName (int index) noexcept;        // "wobbleRate" -> "Wobble Rate"
const char* paramGroupName (Group g) noexcept;
const char* const* paramChoices (int index, int& count) noexcept;
const char* formatParamValue (int index, float value, char* buffer, int bufferSize) noexcept;
float normaliseParam (int index, float plain) noexcept;
float denormaliseParam (int index, float normalised) noexcept;

void resetSnapshotToDefaults (ParamSnapshot& snapshot) noexcept;

// Tempo divisions: 16 entries used by the wobble engine, LFOs and the delay.
struct SyncDivision
{
    const char* name;
    float beats;
};
const SyncDivision* syncDivisions (int& count) noexcept;

enum WobbleShape : int
{
    ShapeSine = 0, ShapeTriangle, ShapeSawUp, ShapeSawDown, ShapeSquare,
    ShapePulse, ShapeRamp, ShapeRandom, ShapeSampleHold, ShapeCustom
};

enum LfoShape : int
{
    LfoSine = 0, LfoTriangle, LfoSawDown, LfoSawUp, LfoSquare, LfoRandom
};

enum FilterType : int
{
    FilterLP12 = 0, FilterLP24, FilterLP36, FilterHP, FilterBP, FilterNotch,
    FilterComb, FilterFormant, FilterLadder, FilterDiode, FilterSvf
};

enum DistortionType : int
{
    DistOff = 0, DistSoftClip, DistHardClip, DistTube, DistDiode, DistFoldback,
    DistWavefold, DistBitcrush, DistDownsample, DistDigital, DistAnalog, DistMultistage
};

enum OscWave : int
{
    WaveSaw = 0, WaveSquare, WavePulse, WaveTriangle, WaveSine, WaveSupersaw,
    WaveWavetable, WaveFM, WaveNoise
};

enum ModSource : int
{
    SrcNone = 0, SrcLfo1, SrcLfo2, SrcLfo3, SrcLfo4, SrcWobble,
    SrcAmpEnv, SrcFiltEnv, SrcPitchEnv, SrcModEnv,
    SrcVelocity, SrcModWheel, SrcAftertouch, SrcKeyTrack, SrcRandom,
    SrcMacro1, SrcMacro2, SrcMacro3, SrcMacro4, SrcMacro5, SrcMacro6, SrcMacro7, SrcMacro8
};

enum ModDest : int
{
    DstNone = 0, DstPitch, DstCutoff, DstReso, DstWtPos1, DstWtPos2, DstWtPos3,
    DstFm, DstPw, DstDrive, DstWidth, DstSubLevel, DstRing, DstFormant,
    DstOsc2Pitch, DstLfoDepth, DstFxMix, DstDistMix, DstDelayMix, DstRevMix, DstChorusMix
};

constexpr int kModSourceCount = 23;
constexpr int kModDestCount = 21;

// Pitch that the engine reaches at full scale; ranges mirror the mod matrix UI.
constexpr float kCutoffModSemitones = 60.0f;
constexpr float kPitchModSemitones = 24.0f;

} // namespace mk
