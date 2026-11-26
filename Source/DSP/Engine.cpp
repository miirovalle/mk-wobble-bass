#include "DSP/Engine.h"

#include <chrono>

namespace mk
{
namespace
{

inline float cutoffSemisFromHz (float hz) noexcept
{
    return 12.0f * std::log2 (std::max (hz, 20.0f) / 20.0f);
}

// Macro offsets are applied to the smoothed snapshot copy every control block.
// Keeping them here (rather than in the matrix) means the eight macros always do
// something musical, while the matrix stays free for user routing.
void applyMacros (ParamSnapshot& p) noexcept
{
    const float bass = p[P::macBass];
    const float growl = p[P::macGrowl];
    const float movement = p[P::macMovement];
    const float drive = p[P::macDrive];
    const float filter = p[P::macFilter];
    const float sub = p[P::macSub];
    const float width = p[P::macWidth];
    const float mix = p[P::macMix];

    if (bass > 0.0001f)
    {
        p[P::subLevel] = clampf (p[P::subLevel] + bass * 0.35f, 0.0f, 1.0f);
        p[P::fltCutoff] = clampf (p[P::fltCutoff] * std::exp2 (-bass * 0.25f), 20.0f, 18000.0f);
    }
    if (growl > 0.0001f)
    {
        p[P::osc2Fm] = clampf (p[P::osc2Fm] + growl * 0.55f, 0.0f, 1.0f);
        p[P::fltReso] = clampf (p[P::fltReso] + growl * 0.22f, 0.0f, 1.0f);
        p[P::osc2Level] = clampf (p[P::osc2Level] + growl * 0.3f, 0.0f, 1.0f);
    }
    if (movement > 0.0001f)
    {
        p[P::wobDepth] = clampf (p[P::wobDepth] + movement * 0.35f, 0.0f, 1.0f);
        p[P::osc1Pos] = clampf (p[P::osc1Pos] + movement * 0.3f, 0.0f, 1.0f);
        p[P::fltEnvAmt] = clampf (p[P::fltEnvAmt] + movement * 1.2f, -4.0f, 4.0f);
    }
    if (drive > 0.0001f)
    {
        p[P::distDrive] = clampf (p[P::distDrive] + drive * 0.45f, 0.0f, 1.0f);
        p[P::distMix] = clampf (p[P::distMix] + drive * 0.25f, 0.0f, 1.0f);
        p[P::fltDrive] = clampf (p[P::fltDrive] + drive * 0.3f, 0.0f, 1.0f);
    }
    if (filter > 0.0001f)
    {
        p[P::fltCutoff] = clampf (p[P::fltCutoff] * std::exp2 (filter * 1.6f), 20.0f, 18000.0f);
        p[P::fltReso] = clampf (p[P::fltReso] + filter * 0.12f, 0.0f, 1.0f);
    }
    if (sub > 0.0001f)
    {
        p[P::subLevel] = clampf (p[P::subLevel] + sub * 0.4f, 0.0f, 1.0f);
        p[P::subDrive] = clampf (p[P::subDrive] + sub * 0.3f, 0.0f, 1.0f);
    }
    if (width > 0.0001f)
    {
        p[P::osc1Spread] = clampf (p[P::osc1Spread] + width * 0.3f, 0.0f, 1.0f);
        p[P::chWidth] = clampf (p[P::chWidth] + width * 0.25f, 0.0f, 1.0f);
    }
    if (mix > 0.0001f)
    {
        p[P::dlMix] = clampf (p[P::dlMix] + mix * 0.35f, 0.0f, 1.0f);
        p[P::rvMix] = clampf (p[P::rvMix] + mix * 0.35f, 0.0f, 1.0f);
        p[P::chMix] = clampf (p[P::chMix] + mix * 0.25f, 0.0f, 1.0f);
    }
}

} // namespace

Engine::Engine()
{
    uiRandom.seed (0x51ED2701u);
    for (int i = 0; i < kMaxVoices; ++i)
    {
        voices[(size_t) i].prepare (48000.0, (uint32_t) (i * 2654435761u + 12345u));
        driftOffsets[(size_t) i] = 0.0f;
        voiceRandom[(size_t) i] = 0.0f;
    }
    resetSnapshotToDefaults (workingParams);
    voiceLimit = 8;
    adaptiveLimit = 8;
}

void Engine::prepare (double sampleRate, int maximumBlockSize)
{
    sr = sampleRate > 0.0 ? sampleRate : 48000.0;
    maxBlock = std::max (maximumBlockSize, kControlBlock);

    for (int i = 0; i < kMaxVoices; ++i)
        voices[(size_t) i].prepare (sr, (uint32_t) (i * 2654435761u + 12345u));

    for (auto& lfo : lfos)
        lfo.prepare (sr);

    wobble.prepare (sr);
    effects.prepare (sr, false);
    store.prepare (sr);
    reset();
}

void Engine::reset()
{
    for (auto& v : voices)
        v.reset();

    for (auto& lfo : lfos)
        lfo.prepare (sr);

    wobble.prepare (sr);
    effects.reset();

    heldNotes.fill (false);
    sustainedNotes.fill (false);
    sustainDown = false;
    monoCount = 0;
    bend = 0.0f;
    modWheel = 0.0f;
    aftertouch = 0.0f;
    wobbleSmoothed = 0.0f;
    glideCoeff = 1.0f;
    for (auto& value : lfoValues)
        value = 0.0f;
    for (auto& value : voiceRandom)
        value = 0.0f;
    peakOut.store (0.0f, std::memory_order_relaxed);
    meterOut.store (0.0f, std::memory_order_relaxed);
}

float Engine::delayTimeSeconds() const noexcept
{
    const auto& p = store.smoothed();
    if (p[P::dlSync] > 0.5f)
    {
        int count = 0;
        const auto* divisions = syncDivisions (count);
        const int index = clampValue ((int) p[P::dlDiv], 0, count - 1);
        return (float) (60.0 / hostBpm) * divisions[index].beats;
    }
    return p[P::dlTime];
}

void Engine::noteOn (int midiNote, float velocity)
{
    if (midiNote < 0 || midiNote > 127)
        return;

    const int mode = store.smoothed().asInt (P::polyMode);
    heldNotes[(size_t) midiNote] = true;
    sustainedNotes[(size_t) midiNote] = false;
    lastPlayedNote = midiNote;

    if (mode != 0)
    {
        handleMonoNoteOn (midiNote, velocity);
        return;
    }

    // Re-use a voice already playing this note so repeated notes retrigger
    // instead of stacking.
    for (auto& v : voices)
    {
        if (v.isActive() && v.isHeld() && v.note() == midiNote)
        {
            v.noteOn (midiNote, velocity, false, true);
            return;
        }
    }

    if (auto* voice = allocateVoice())
        voice->noteOn (midiNote, velocity, false, true);
}

void Engine::noteOff (int midiNote)
{
    if (midiNote < 0 || midiNote > 127)
        return;

    heldNotes[(size_t) midiNote] = false;

    if (sustainDown)
    {
        sustainedNotes[(size_t) midiNote] = true;
        return;
    }

    if (store.smoothed().asInt (P::polyMode) != 0)
    {
        handleMonoNoteOff (midiNote);
        return;
    }

    for (auto& v : voices)
        if (v.isActive() && v.note() == midiNote)
            v.noteOff();
}

void Engine::handleMonoNoteOn (int note, float velocity) noexcept
{
    const bool legatoMode = store.smoothed().asInt (P::polyMode) == 2;

    if (monoCount < 16)
        monoStack[monoCount++] = note;
    else
    {
        for (int i = 1; i < 16; ++i)
            monoStack[i - 1] = monoStack[i];
        monoStack[15] = note;
    }

    auto& voice = voices[0];
    const bool legatoHold = legatoMode && voice.isActive() && voice.isHeld();
    voice.noteOn (note, velocity, legatoHold, ! legatoHold);

    // Mono owns one voice: release the rest so a mode change never leaves a drone.
    for (int i = 1; i < kMaxVoices; ++i)
        if (voices[(size_t) i].isActive())
            voices[(size_t) i].quickRelease();
}

void Engine::handleMonoNoteOff (int note) noexcept
{
    int found = -1;
    for (int i = 0; i < monoCount; ++i)
        if (monoStack[i] == note)
        {
            found = i;
            break;
        }

    if (found >= 0)
    {
        for (int i = found; i < monoCount - 1; ++i)
            monoStack[i] = monoStack[i + 1];
        --monoCount;
    }

    auto& voice = voices[0];
    if (monoCount > 0)
    {
        const int fallback = monoStack[monoCount - 1];
        const bool legatoMode = store.smoothed().asInt (P::polyMode) == 2;
        voice.noteOn (fallback, voice.velocityValue() > 0.0f ? voice.velocityValue() : 1.0f,
                      legatoMode, ! legatoMode);
        return;
    }

    if (sustainDown)
        return;

    voice.noteOff();
}

void Engine::allNotesOff (bool immediate)
{
    for (auto& v : voices)
    {
        if (immediate)
            v.reset();
        else if (v.isActive())
            v.quickRelease();
    }

    heldNotes.fill (false);
    sustainedNotes.fill (false);
    monoCount = 0;
}

void Engine::pitchBend (float normalised)
{
    bend = clampf (normalised, -1.0f, 1.0f);
}

void Engine::setModWheel (float value)
{
    modWheel = clampf (value, 0.0f, 1.0f);
}

void Engine::setAftertouch (float value)
{
    aftertouch = clampf (value, 0.0f, 1.0f);
}

void Engine::setSustainPedal (bool down)
{
    sustainDown = down;
    if (down)
        return;

    for (int note = 0; note < 128; ++note)
    {
        if (! sustainedNotes[(size_t) note])
            continue;

        sustainedNotes[(size_t) note] = false;
        if (heldNotes[(size_t) note])
            continue;

        if (store.smoothed().asInt (P::polyMode) != 0)
        {
            handleMonoNoteOff (note);
            return;
        }

        for (auto& v : voices)
            if (v.isActive() && v.note() == note)
                v.noteOff();
    }
}

Voice* Engine::allocateVoice() noexcept
{
    const int limit = std::max (1, effectiveVoiceLimit());

    for (int i = 0; i < limit; ++i)
        if (! voices[(size_t) i].isActive())
            return &voices[(size_t) i];

    Voice* oldestReleased = nullptr;
    Voice* oldestHeld = nullptr;

    for (int i = 0; i < limit; ++i)
    {
        auto& v = voices[(size_t) i];
        if (v.isReleasing())
        {
            if (oldestReleased == nullptr || v.age() > oldestReleased->age())
                oldestReleased = &v;
        }
        else if (oldestHeld == nullptr || v.age() > oldestHeld->age())
        {
            oldestHeld = &v;
        }
    }

    Voice* victim = oldestReleased != nullptr ? oldestReleased : oldestHeld;
    if (victim != nullptr)
        victim->quickRelease();

    return victim;
}

void Engine::updateAdaptiveLimit (int numSamples) noexcept
{
    const bool adaptiveOn = store.smoothed().asBool (P::adaptive);
    const int requested = clampValue ((int) store.smoothed()[P::polyphony], 1, kMaxVoices);
    voiceLimit = requested;

    if (! adaptiveOn)
    {
        adaptiveLimit = requested;
        return;
    }

    const double budgetMs = (double) numSamples * 1000.0 / sr;

    if (lastBlockMsValue > budgetMs * 0.8)
    {
        if (++overloadBlocks > 6)
        {
            adaptiveLimit = std::max (2, adaptiveLimit - 1);
            overloadBlocks = 0;
        }
        calmBlocks = 0;
    }
    else if (lastBlockMsValue < budgetMs * 0.45)
    {
        if (++calmBlocks > 240)
        {
            adaptiveLimit = std::min (voiceLimit, adaptiveLimit + 1);
            calmBlocks = 0;
        }
        overloadBlocks = 0;
    }

    adaptiveLimit = clampValue (adaptiveLimit, 1, voiceLimit);
}

void Engine::updateModulators (int numSamples, const ParamSnapshot& p) noexcept
{
    for (int i = 0; i < kLfoCount; ++i)
    {
        const int base = (int) P::lfo1Sync + i * 9;
        lfos[(size_t) i].setParameters ((int) p.values[base + 3], p.values[base + 2],
                                        p.values[base + 4], p.values[base + 5],
                                        p.values[base + 6], p.values[base + 7],
                                        p.values[base + 8] > 0.5f, hostBpm,
                                        p.values[base] > 0.5f, (int) p.values[base + 1]);
        lfos[(size_t) i].advance (numSamples);
        lfoValues[i] = lfos[(size_t) i].value();
    }

    wobble.setParameters (p[P::wobMode] > 0.5f, (int) p[P::wobDiv], p[P::wobRate],
                          (int) p[P::wobShape], p[P::wobDepth], p[P::wobPhase],
                          p[P::wobOffset], p[P::wobSmooth], hostBpm);
    wobble.advance (numSamples);
    wobbleSmoothed = wobble.value();

    wobbleDisplay.store (wobbleSmoothed, std::memory_order_relaxed);
    wobbleShapeDisplay.store ((int) p[P::wobShape], std::memory_order_relaxed);
    wobbleDepthDisplay.store (p[P::wobDepth], std::memory_order_relaxed);
    wobblePhaseDisplay.store (wobble.rawPhase(), std::memory_order_relaxed);
    wobbleRateDisplay.store (WobbleEngine::rateInHz (p[P::wobMode] > 0.5f, (int) p[P::wobDiv],
                                                     p[P::wobRate], hostBpm),
                             std::memory_order_relaxed);

    for (int i = 0; i < kMacroCount; ++i)
        macroValues[i] = p[(P) ((int) P::macBass + i)];

    if ((randomTick++ & 7u) == 0u)
        for (auto& value : voiceRandom)
            value = uiRandom.nextBipolar();

    // Wobble trace ring for the animated display (once per control block).
    int writeIndex = wobbleWrite.load (std::memory_order_relaxed) + 1;
    if (writeIndex >= kScopeSize)
        writeIndex = 0;
    wobbleTrace[(size_t) writeIndex].store (wobbleSmoothed, std::memory_order_relaxed);
    wobbleWrite.store (writeIndex, std::memory_order_relaxed);
}

void Engine::buildVoiceParams (const ParamSnapshot& p) noexcept
{
    voiceParams.osc1Wave = (int) p[P::osc1Wave];
    voiceParams.osc1Bank = (int) p[P::osc1Bank];
    voiceParams.osc1Pos = p[P::osc1Pos];
    voiceParams.osc1Level = p[P::osc1Level];
    voiceParams.osc1Semis = p[P::osc1Oct] * 12.0f + p[P::osc1Semi] + p[P::osc1Fine] * 0.01f;
    voiceParams.osc1Uni = (int) p[P::osc1Uni];
    voiceParams.osc1Detune = p[P::osc1Detune];
    voiceParams.osc1Spread = p[P::osc1Spread];
    voiceParams.osc1Pw = p[P::osc1Pw];

    voiceParams.osc2Mode = (int) p[P::osc2Mode];
    voiceParams.osc2Wave = (int) p[P::osc2Wave];
    voiceParams.osc2Bank = (int) p[P::osc2Bank];
    voiceParams.osc2Pos = p[P::osc2Pos];
    voiceParams.osc2Level = p[P::osc2Level];
    voiceParams.osc2Semis = p[P::osc2Oct] * 12.0f + p[P::osc2Semi] + p[P::osc2Fine] * 0.01f;
    voiceParams.osc2Uni = (int) p[P::osc2Uni];
    voiceParams.osc2Detune = p[P::osc2Detune];
    voiceParams.osc2Spread = p[P::osc2Spread];
    voiceParams.osc2Pw = p[P::osc2Pw];
    voiceParams.osc2Ratio = p[P::osc2Ratio];
    voiceParams.osc2Fm = p[P::osc2Fm];
    voiceParams.osc2Ring = p[P::osc2Ring];

    voiceParams.osc3Wave = (int) p[P::osc3Wave];
    voiceParams.osc3Bank = (int) p[P::osc3Bank];
    voiceParams.osc3Pos = p[P::osc3Pos];
    voiceParams.osc3Level = p[P::osc3Level];
    voiceParams.osc3Semis = p[P::osc3Oct] * 12.0f + p[P::osc3Semi];

    voiceParams.fmFeedback = p[P::fmFeedback];
    voiceParams.fmModWave = (int) p[P::fmModWave];

    voiceParams.subWave = (int) p[P::subWave];
    const int subOct = (int) p[P::subOct];
    voiceParams.subSemis = subOct == 0 ? -12.0f : (subOct == 1 ? -24.0f : 0.0f);
    voiceParams.subLevel = p[P::subLevel];
    voiceParams.subDrive = p[P::subDrive];

    voiceParams.fltType = (int) p[P::fltType];
    voiceParams.fltCutoffSemis = cutoffSemisFromHz (p[P::fltCutoff]);
    voiceParams.fltReso = p[P::fltReso];
    voiceParams.fltDrive = p[P::fltDrive];
    voiceParams.fltKeyTrack = p[P::fltKeyTrack];
    voiceParams.fltEnvAmt = p[P::fltEnvAmt];
    voiceParams.fltMix = p[P::fltMix];
    voiceParams.fltRoute = (int) p[P::fltRoute];
    voiceParams.flt2Type = (int) p[P::flt2Type];
    voiceParams.flt2CutoffSemis = cutoffSemisFromHz (p[P::flt2Cutoff]);
    voiceParams.flt2Reso = p[P::flt2Reso];
    voiceParams.fltVowel = (int) p[P::fltFormant];

    voiceParams.distType = (int) p[P::distType];
    voiceParams.distDrive = p[P::distDrive];
    voiceParams.distTone = p[P::distTone];
    voiceParams.distMix = p[P::distMix];
    voiceParams.distOutDb = p[P::distOut];
    voiceParams.distPre = p[P::distPre] > 0.5f;
    voiceParams.distRoute = (int) p[P::distRoute];
    voiceParams.oversample = p[P::oversample] > 0.5f;

    voiceParams.ampCoeffs = AdsrCoefficients::make (sr, p[P::ampA], p[P::ampD], p[P::ampS],
                                                    p[P::ampR], p[P::ampCurve]);
    voiceParams.fltCoeffs = AdsrCoefficients::make (sr, p[P::fltA], p[P::fltD], p[P::fltS],
                                                    p[P::fltR], 0.2f);
    voiceParams.pitCoeffs = AdsrCoefficients::make (sr, p[P::pitA], p[P::pitD], p[P::pitS],
                                                    p[P::pitR], 0.2f);
    voiceParams.modCoeffs = AdsrCoefficients::make (sr, p[P::modA], p[P::modD], p[P::modS],
                                                    p[P::modR], 0.2f);
    voiceParams.pitAmt = p[P::pitAmt];
    voiceParams.velSens = p[P::velSens];
    voiceParams.width = 1.0f;
}

void Engine::applyMatrix (const ParamSnapshot& p, const Voice* voice, float randomValue,
                          ModValues& mod) const noexcept
{
    float sources[kModSourceCount] {};
    sources[SrcLfo1] = lfoValues[0];
    sources[SrcLfo2] = lfoValues[1];
    sources[SrcLfo3] = lfoValues[2];
    sources[SrcLfo4] = lfoValues[3];
    sources[SrcWobble] = wobbleSmoothed;
    sources[SrcRandom] = randomValue;

    if (voice != nullptr)
    {
        sources[SrcAmpEnv] = voice->envelopeLevel();
        sources[SrcFiltEnv] = voice->filterLevel();
        sources[SrcPitchEnv] = voice->pitchLevel();
        sources[SrcModEnv] = voice->modEnvLevel();
        sources[SrcVelocity] = voice->velocityValue();
        sources[SrcKeyTrack] = clampf ((float) (voice->note() - 60) / 48.0f, -1.0f, 1.0f);
    }

    sources[SrcModWheel] = modWheel;
    sources[SrcAftertouch] = aftertouch;

    for (int i = 0; i < kMacroCount; ++i)
        sources[SrcMacro1 + i] = macroValues[i];

    const int matrixBase = (int) P::mat1Src;
    for (int slot = 0; slot < kMatrixSlots; ++slot)
    {
        const int index = matrixBase + slot * 3;
        const int src = (int) p.values[index];
        const int dst = (int) p.values[index + 1];
        const float amount = p.values[index + 2];

        if (src <= 0 || src >= kModSourceCount || dst <= 0 || dst >= kModDestCount
            || std::fabs (amount) < 1.0e-4f)
            continue;

        mod[dst] += sources[src] * amount;
    }
}

void Engine::applyWobbleRouting (const ParamSnapshot& p, ModValues& mod) const noexcept
{
    const float w = wobbleSmoothed;
    if (std::fabs (w) < 1.0e-4f)
        return;

    mod[DstCutoff] += w * p[P::wobCut] * 0.8f;
    mod[DstWtPos1] += w * p[P::wobWt];
    mod[DstWtPos2] += w * p[P::wobWt] * 0.7f;
    mod[DstWtPos3] += w * p[P::wobWt] * 0.5f;
    mod[DstFm] += w * p[P::wobFm];
    mod[DstDrive] += w * p[P::wobDrive];
    mod[DstPitch] += w * p[P::wobPitch] * 0.5f;
    mod[DstPw] += w * p[P::wobPw];
    mod[DstReso] += w * p[P::wobReso];
    mod[DstWidth] += w * p[P::wobWidth];
    mod[DstFormant] += w * p[P::wobFormant];
}

ModValues Engine::computeGlobalMod (const ParamSnapshot& p) const noexcept
{
    ModValues mod;
    applyMatrix (p, nullptr, 0.0f, mod);
    applyWobbleRouting (p, mod);
    return mod;
}

void Engine::updateGlide (const ParamSnapshot& p, int chunk) noexcept
{
    const float glideMs = p[P::glideTime];
    if (glideMs < 0.5f)
        glideCoeff = 1.0f;
    else
        glideCoeff = 1.0f - std::exp (-(float) chunk / (glideMs * 0.001f * (float) sr));

    const float bendSemis = bend * p[P::bendRange];

    for (int i = 0; i < kMaxVoices; ++i)
    {
        auto& voice = voices[(size_t) i];
        if (! voice.isActive())
            continue;

        const float target = (float) voice.note() + bendSemis + driftOffsets[(size_t) i];
        voice.setGlide (target, glideCoeff);
    }
}

EffectSettings Engine::makeEffectSettings (const ParamSnapshot& p, const ModValues& globalMod) const noexcept
{
    EffectSettings fx;
    fx.chorusOn = p[P::chOn] > 0.5f;
    fx.chorusRate = p[P::chRate];
    fx.chorusDepth = p[P::chDepth];
    fx.chorusMix = clampf (p[P::chMix] + globalMod[DstChorusMix] + globalMod[DstFxMix] * 0.5f, 0.0f, 1.0f);
    fx.chorusWidth = p[P::chWidth];

    fx.phaserOn = p[P::phOn] > 0.5f;
    fx.phaserRate = p[P::phRate];
    fx.phaserDepth = p[P::phDepth];
    fx.phaserFb = p[P::phFb];
    fx.phaserStages = clampValue ((int) p[P::phStages], 2, 12);
    fx.phaserMix = clampf (p[P::phMix] + globalMod[DstFxMix] * 0.4f, 0.0f, 1.0f);

    fx.flangerOn = p[P::flOn] > 0.5f;
    fx.flangerRate = p[P::flRate];
    fx.flangerDepth = p[P::flDepth];
    fx.flangerFb = p[P::flFb];
    fx.flangerDelay = p[P::flDelay];
    fx.flangerMix = clampf (p[P::flMix] + globalMod[DstFxMix] * 0.4f, 0.0f, 1.0f);

    fx.delayOn = p[P::dlOn] > 0.5f;
    float delaySeconds = p[P::dlTime];
    if (p[P::dlSync] > 0.5f)
    {
        int count = 0;
        const auto* divisions = syncDivisions (count);
        const int index = clampValue ((int) p[P::dlDiv], 0, count - 1);
        delaySeconds = (float) (60.0 / hostBpm) * divisions[index].beats;
    }
    fx.delaySeconds = clampf (delaySeconds, 0.02f, 1.5f);
    fx.delayFeedback = p[P::dlFb];
    fx.delayMix = clampf (p[P::dlMix] + globalMod[DstDelayMix] + globalMod[DstFxMix] * 0.4f, 0.0f, 1.0f);
    fx.delayTone = p[P::dlFilter];
    fx.delayPingPong = p[P::dlPing] > 0.5f;

    fx.reverbOn = p[P::rvOn] > 0.5f;
    fx.reverbSize = p[P::rvSize];
    fx.reverbDecay = p[P::rvDecay];
    fx.reverbDamp = p[P::rvDamp];
    fx.reverbWidth = p[P::rvWidth];
    fx.reverbMix = clampf (p[P::rvMix] + globalMod[DstRevMix] + globalMod[DstFxMix] * 0.4f, 0.0f, 1.0f);

    fx.compressorOn = p[P::cpOn] > 0.5f;
    fx.compThresholdDb = p[P::cpThresh];
    fx.compRatio = p[P::cpRatio];
    fx.compAttackMs = p[P::cpAttack];
    fx.compReleaseMs = p[P::cpRelease];
    fx.compMakeupDb = p[P::cpMakeup];

    fx.multibandOn = p[P::mbOn] > 0.5f;
    fx.multibandDrive = p[P::mbDrive];
    fx.multibandMix = p[P::mbMix];

    fx.wobbleValue = wobbleSmoothed;
    return fx;
}

float Engine::softLimit (float x) noexcept
{
    constexpr float threshold = 0.94f;
    const float ax = std::fabs (x);
    if (ax <= threshold)
        return x;

    const float over = ax - threshold;
    const float limited = threshold + (1.0f - std::exp (-over * 12.0f)) * (1.0f - threshold);
    return x < 0.0f ? -limited : limited;
}

void Engine::process (float* left, float* right, int numSamples)
{
    if (numSamples <= 0 || left == nullptr || right == nullptr)
        return;

    const auto startTime = std::chrono::steady_clock::now();

    int done = 0;
    while (done < numSamples)
    {
        const int chunk = std::min (kControlBlock, numSamples - done);

        store.advance (chunk);
        workingParams = store.smoothed();
        applyMacros (workingParams);

        updateModulators (chunk, workingParams);
        buildVoiceParams (workingParams);
        updateAdaptiveLimit (chunk);
        updateGlide (workingParams, chunk);

        std::fill (left + done, left + done + chunk, 0.0f);
        std::fill (right + done, right + done + chunk, 0.0f);

        const int quality = (int) workingParams[P::quality];
        const int limit = std::max (1, effectiveVoiceLimit());

        for (int i = 0; i < limit; ++i)
        {
            auto& voice = voices[(size_t) i];
            if (! voice.isActive())
                continue;

            ModValues mod;
            applyMatrix (workingParams, &voice, voiceRandom[(size_t) i], mod);
            applyWobbleRouting (workingParams, mod);
            voice.age ((float) chunk);
            voice.setParams (voiceParams, mod, quality);
            voice.render (left + done, right + done, chunk);
        }

        done += chunk;
    }

    const ModValues globalMod = computeGlobalMod (workingParams);
    const EffectSettings fx = makeEffectSettings (workingParams, globalMod);
    effects.setSettings (fx);
    effects.process (left, right, numSamples);

    const float masterGain = dbToGain (workingParams[P::outputDb]);
    float peak = 0.0f;
    double energy = 0.0;
    int scopeCounter = 0;

    for (int i = 0; i < numSamples; ++i)
    {
        float l = softLimit (left[i] * masterGain);
        float r = softLimit (right[i] * masterGain);

        left[i] = l;
        right[i] = r;
        peak = std::max (peak, std::max (std::fabs (l), std::fabs (r)));
        energy += (double) l * l + (double) r * r;

        if ((scopeCounter++ & 7) == 0)
        {
            int index = scopeWrite.load (std::memory_order_relaxed) + 1;
            if (index >= kScopeSize)
                index = 0;
            scope[(size_t) index].store ((l + r) * 0.5f, std::memory_order_relaxed);
            scopeWrite.store (index, std::memory_order_relaxed);
        }
    }

    meterOut.store ((float) std::sqrt (energy / (double) (2 * numSamples)), std::memory_order_relaxed);
    peakOut.store (peak, std::memory_order_relaxed);
    filterDisplayValue.store (workingParams[P::fltCutoff] / 18000.0f, std::memory_order_relaxed);

    lastBlockMsValue = std::chrono::duration<double, std::milli> (
                      std::chrono::steady_clock::now() - startTime).count();
}

void Engine::copyScope (float* destination, int count) const noexcept
{
    const int write = scopeWrite.load (std::memory_order_relaxed);
    for (int i = 0; i < count; ++i)
    {
        int index = write - count + (i + 1);
        while (index < 0)
            index += kScopeSize;
        destination[i] = scope[(size_t) (index % kScopeSize)].load (std::memory_order_relaxed);
    }
}

void Engine::copyWobbleTrace (float* destination, int count) const noexcept
{
    const int write = wobbleWrite.load (std::memory_order_relaxed);
    for (int i = 0; i < count; ++i)
    {
        int index = write - count + (i + 1);
        while (index < 0)
            index += kScopeSize;
        destination[i] = wobbleTrace[(size_t) (index % kScopeSize)].load (std::memory_order_relaxed);
    }
}

int Engine::activeVoiceCount() const noexcept
{
    int count = 0;
    for (const auto& v : voices)
        if (v.isActive())
            ++count;
    return count;
}

size_t Engine::memoryFootprintBytes() const noexcept
{
    size_t total = sizeof (Engine);
    total += effects.memoryBytes();
    for (const auto& v : voices)
        total += v.memoryBytes() - sizeof (Voice);
    return total;
}

} // namespace mk
