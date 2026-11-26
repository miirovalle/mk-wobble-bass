#pragma once

// A single bass voice: three oscillator slots plus sub, FM/ring routing, two
// multimode filters, per-voice distortion, four envelopes and stereo spread.
//
// Modulated quantities are evaluated once per 32-sample control block (filters,
// pitch, wavetable position); only the envelopes, oscillators, filters and gain
// run per sample. That split is what keeps the cost per voice in the low
// single-digit percentage of one A10X core.

#include "DSP/Distortion.h"
#include "DSP/Envelope.h"
#include "DSP/Filters.h"
#include "DSP/Oscillator.h"
#include "DSP/Params.h"

namespace mk
{

struct ModValues
{
    float v[kModDestCount] {};
    inline float& operator[] (int dest) noexcept { return v[dest]; }
    inline float operator[] (int dest) const noexcept { return v[dest]; }
};

struct VoiceParams
{
    int osc1Wave = WaveSaw, osc1Bank = 0;
    int osc2Wave = WaveSaw, osc2Bank = 0;
    int osc3Wave = WaveNoise, osc3Bank = 0;
    int osc2Mode = 0;
    int osc1Uni = 1, osc2Uni = 1;
    float osc1Semis = 0.0f, osc2Semis = 0.0f, osc3Semis = 0.0f;
    float osc1Pos = 0.25f, osc2Pos = 0.4f, osc3Pos = 0.5f;
    float osc1Level = 0.85f, osc2Level = 0.0f, osc3Level = 0.0f;
    float osc1Detune = 12.0f, osc2Detune = 10.0f;
    float osc1Spread = 0.6f, osc2Spread = 0.5f;
    float osc1Pw = 0.5f, osc2Pw = 0.5f;
    float osc2Ratio = 1.0f, osc2Fm = 0.0f, osc2Ring = 0.0f;
    float fmFeedback = 0.0f;
    int fmModWave = 0;

    int subWave = 0;
    float subSemis = -12.0f, subLevel = 0.4f, subDrive = 0.0f;

    int fltType = FilterLP24, flt2Type = FilterLP12, fltRoute = 0, fltVowel = 0;
    float fltCutoffSemis = 0.0f, fltReso = 0.2f, fltDrive = 0.1f;
    float fltKeyTrack = 0.35f, fltEnvAmt = 1.6f, fltMix = 1.0f;
    float flt2CutoffSemis = 0.0f, flt2Reso = 0.1f;

    int distType = DistOff, distRoute = 0;
    float distDrive = 0.3f, distTone = 0.0f, distMix = 0.5f, distOutDb = 0.0f;
    bool distPre = false;
    bool oversample = false;

    float ampA = 2.0f, ampD = 300.0f, ampS = 0.8f, ampR = 150.0f, ampCurve = 0.35f;
    float fltA = 1.0f, fltD = 260.0f, fltS = 0.25f, fltR = 160.0f;
    float pitA = 1.0f, pitD = 90.0f, pitS = 0.0f, pitR = 100.0f, pitAmt = 0.0f;
    float modA = 6.0f, modD = 500.0f, modS = 0.3f, modR = 300.0f;
    float velSens = 0.6f;
    float width = 1.0f;

    // Built once per block by the engine and shared by all voices.
    AdsrCoefficients ampCoeffs, fltCoeffs, pitCoeffs, modCoeffs;
};

class Voice
{
public:
    void prepare (double sampleRate, uint32_t seed)
    {
        sr = sampleRate > 0.0 ? sampleRate : 48000.0;
        combSize = ChannelFilter::requiredCombSamples (sr);
        if ((int) combBuffers[0].size() != combSize)
        {
            for (auto& buffer : combBuffers)
                buffer.assign ((size_t) combSize, 0.0f);
        }

        osc1.prepare (sr);
        osc2.prepare (sr);
        osc3.prepare (sr);
        ampEnv.prepare (sr);
        filterEnv.prepare (sr);
        pitchEnv.prepare (sr);
        modEnv.prepare (sr);
        filterL.prepare (sr, combBuffers[0].data(), combSize);
        filterR.prepare (sr, combBuffers[1].data(), combSize);
        filter2L.prepare (sr, combBuffers[2].data(), combSize);
        filter2R.prepare (sr, combBuffers[3].data(), combSize);
        distortion.prepare (sr);
        dcL.prepare (sr);
        dcR.prepare (sr);

        seedRng (seed);
        reset();
    }

    void seedRng (uint32_t seed) noexcept
    {
        rng.seed (seed);
        const float p = rng.nextUnipolar() * 0.01f;
        osc1.reset (p, rng.nextUnipolar());
        osc2.reset (p * 0.5f, rng.nextUnipolar());
        osc3.reset (p * 0.25f, rng.nextUnipolar());
        subPhase = (rng.nextUnipolar() < 0.5f) ? 0.0f : 0.5f;
    }

    void reset() noexcept
    {
        ampEnv.reset();
        filterEnv.reset();
        pitchEnv.reset();
        modEnv.reset();
        filterL.reset();
        filterR.reset();
        filter2L.reset();
        filter2R.reset();
        distortion.reset();
        dcL.reset();
        dcR.reset();
        subPhase = 0.0f;
        fmFeedbackState = 0.0f;
        held = false;
        ageCounter = 0.0f;
        glideNote = 60.0f;
        targetNote = 60.0f;
        velocity = 0.0f;
    }

    void noteOn (int midiNote, float noteVelocity, bool legato, bool resetEnvelopes) noexcept
    {
        targetNote = (float) midiNote;
        if (! legato)
            glideNote = targetNote;
        velocity = clampf (noteVelocity, 0.0f, 1.0f);
        held = true;
        ageCounter = 0.0f;

        if (! legato)
        {
            if (resetEnvelopes)
            {
                ampEnv.noteOn (true);
                filterEnv.noteOn (true);
                pitchEnv.noteOn (true);
                modEnv.noteOn (true);
                subPhase = 0.0f;
                fmFeedbackState = 0.0f;
            }
        }
    }

    void noteOff() noexcept
    {
        held = false;
        ampEnv.noteOff();
        filterEnv.noteOff();
        pitchEnv.noteOff();
        modEnv.noteOff();
    }

    void quickRelease() noexcept
    {
        held = false;
        ampEnv.quickRelease();
        filterEnv.noteOff();
        pitchEnv.noteOff();
        modEnv.noteOff();
    }

    void setGlide (float semitoneTarget, float coeff) noexcept
    {
        glideNote += (semitoneTarget - glideNote) * coeff;
    }

    void setParams (const VoiceParams& params, const ModValues& mod, int quality) noexcept
    {
        p = params;

        const float pitchBase = glideNote
                              + mod[DstPitch] * kPitchModSemitones
                              + pitchEnv.getValue() * p.pitAmt;

        const int unisonCap = quality == 0 ? 3 : (quality == 1 ? 5 : 7);
        const bool cubic = quality == 2;

        const float pos1 = clampf (p.osc1Pos + mod[DstWtPos1], 0.0f, 1.0f);
        const float pos2 = clampf (p.osc2Pos + mod[DstWtPos2], 0.0f, 1.0f);
        const float pos3 = clampf (p.osc3Pos + mod[DstWtPos3], 0.0f, 1.0f);
        const float pw1 = clampf (p.osc1Pw + mod[DstPw] * 0.45f, 0.05f, 0.95f);
        const float pw2 = clampf (p.osc2Pw + mod[DstPw] * 0.45f, 0.05f, 0.95f);

        osc1Active = p.osc1Level > 0.0008f;
        osc2Active = p.osc2Level > 0.0008f || (p.osc2Mode == 1 && p.osc2Fm > 0.0008f);
        osc3Active = p.osc3Level > 0.0008f;
        subActive = p.subLevel > 0.0008f;

        if (osc1Active)
        {
            osc1.configure (p.osc1Wave, p.osc1Bank, pos1, std::min ((int) p.osc1Uni, unisonCap),
                            p.osc1Detune, p.osc1Spread, pw1, p.osc1Level, cubic);
            osc1.setPitch (pitchBase + p.osc1Semis);
        }

        if (osc2Active)
        {
            const float osc2Pitch = pitchBase + p.osc2Semis
                                  + mod[DstOsc2Pitch] * kPitchModSemitones;
            osc2.configure (p.osc2Wave, p.osc2Bank, pos2, std::min ((int) p.osc2Uni, unisonCap),
                            p.osc2Detune, p.osc2Spread, pw2,
                            p.osc2Mode == 1 ? std::max (p.osc2Level, 0.0f) : p.osc2Level, cubic);
            osc2.setPitch (osc2Pitch + (p.osc2Mode == 1 ? 12.0f * std::log2 (std::max (p.osc2Ratio, 0.01f)) : 0.0f));
        }

        if (osc3Active)
        {
            osc3.configure (p.osc3Wave, p.osc3Bank, pos3, 1, 0.0f, 0.0f, 0.5f, p.osc3Level, cubic);
            osc3.setPitch (pitchBase + p.osc3Semis);
        }

        fmAmount = clampf (p.osc2Fm + mod[DstFm], 0.0f, 1.0f);
        ringAmount = clampf (p.osc2Ring + mod[DstRing], 0.0f, 1.0f);
        fmFeedbackAmount = clampf (p.fmFeedback, 0.0f, 1.0f);
        fmIsActive = p.osc2Mode == 1 && fmAmount > 0.0008f && osc2Active;

        const float filtEnvValue = filterEnv.getValue();
        const float cutoffSemis = p.fltCutoffSemis
                                + p.fltKeyTrack * (glideNote - 36.0f)
                                + p.fltEnvAmt * 12.0f * filtEnvValue
                                + mod[DstCutoff] * kCutoffModSemitones;

        const float cutoffHz = 20.0f * fastSemitoneRatio (clampf (cutoffSemis, 0.0f, 165.0f));
        const float cutReso = clampf (p.fltReso + mod[DstReso], 0.0f, 0.995f);
        driveLevel = clampf (p.fltDrive + mod[DstDrive] * 0.6f, 0.0f, 1.0f);
        formantShift = mod[DstFormant] * 24.0f;
        fltMixActive = p.fltMix > 0.0005f || p.fltRoute != 0;

        filterL.setCoefficients (p.fltType, cutoffHz, cutReso, formantShift, p.fltVowel, quality);
        filterR.setCoefficients (p.fltType, cutoffHz, cutReso, formantShift, p.fltVowel, quality);

        if (p.fltRoute != 0)
        {
            const float cutoff2Semis = p.flt2CutoffSemis
                                     + p.fltKeyTrack * (glideNote - 36.0f)
                                     + p.fltEnvAmt * 12.0f * filtEnvValue
                                     + mod[DstCutoff] * kCutoffModSemitones;
            const float cutoff2Hz = 20.0f * fastSemitoneRatio (clampf (cutoff2Semis, 0.0f, 165.0f));
            const float reso2 = clampf (p.flt2Reso + mod[DstReso] * 0.5f, 0.0f, 0.995f);
            filter2L.setCoefficients (p.flt2Type, cutoff2Hz, reso2, formantShift, p.fltVowel, quality);
            filter2R.setCoefficients (p.flt2Type, cutoff2Hz, reso2, formantShift, p.fltVowel, quality);
        }

        distortionActive = p.distRoute != 0 && p.distType != DistOff
                           && (p.distMix + mod[DstDistMix]) > 0.0008f;
        if (distortionActive)
        {
            distortion.setParameters (p.distType, clampf (p.distDrive + mod[DstDrive] * 0.4f, 0.0f, 1.0f),
                                      p.distTone, clampf (p.distMix + mod[DstDistMix], 0.0f, 1.0f),
                                      p.distOutDb, p.oversample);
        }

        subIncrement = std::max (0.0f, midiToHz (p.subSemis + pitchBase) / (float) sr);

        ampEnv.setCoefficients (p.ampCoeffs);
        filterEnv.setCoefficients (p.fltCoeffs);
        pitchEnv.setCoefficients (p.pitCoeffs);
        modEnv.setCoefficients (p.modCoeffs);

        widthGain = clampf (p.width + mod[DstWidth] * 0.6f, 0.0f, 1.6f);
        velocityGain = 1.0f - p.velSens + p.velSens * velocity;
    }

    // Renders n samples (n <= control block size) additively into the buffers.
    void render (float* outL, float* outR, int n) noexcept
    {
        if (! ampEnv.isActive())
            return;

        const float subLevel = subActive ? p.subLevel : 0.0f;
        const float subDriveAmount = p.subDrive;

        for (int i = 0; i < n; ++i)
        {
            const float amp = ampEnv.process();
            filterEnv.process();
            pitchEnv.process();
            modEnv.process();

            float l = 0.0f;
            float r = 0.0f;

            float osc2L = 0.0f, osc2R = 0.0f;
            if (osc2Active)
                osc2.render (osc2L, osc2R);

            float pmOffset = 0.0f;
            if (fmIsActive)
            {
                const float modulator = (osc2L + osc2R) * 0.5f;
                pmOffset = modulator * fmAmount * 3.0f + fmFeedbackState * fmFeedbackAmount * 1.5f;
            }

            float osc1L = 0.0f, osc1R = 0.0f;
            if (osc1Active)
                osc1.render (osc1L, osc1R, pmOffset);

            l = osc1L + osc2L;
            r = osc1R + osc2R;

            if (fmIsActive)
                fmFeedbackState = (osc1L + osc1R) * 0.5f;

            if (ringAmount > 0.0008f && osc1Active && osc2Active)
            {
                const float ring = ((osc1L + osc1R) * (osc2L + osc2R)) * 0.5f * ringAmount;
                l += ring;
                r += ring;
            }

            if (osc3Active)
            {
                float osc3L = 0.0f, osc3R = 0.0f;
                osc3.render (osc3L, osc3R);
                l += osc3L;
                r += osc3R;
            }

            if (subLevel > 0.0f)
            {
                subPhase += subIncrement;
                if (subPhase >= 1.0f)
                    subPhase -= 1.0f;

                float sub;
                switch (p.subWave)
                {
                    case 1: sub = 1.0f - 4.0f * std::fabs (subPhase - 0.5f); break;
                    case 2: sub = subPhase < 0.5f ? 1.0f : -1.0f; break;
                    default: sub = SineTable::instance().lookup (subPhase); break;
                }

                if (subDriveAmount > 0.001f)
                {
                    const float g = 1.0f + subDriveAmount * 6.0f;
                    sub = fastTanh (sub * g) * (1.0f / (1.0f + subDriveAmount * 1.5f));
                }

                l += sub * subLevel;
                r += sub * subLevel;
            }

            if (distortionActive && p.distPre)
                distortion.process (l, r);

            if (fltMixActive)
            {
                float filteredL = filterL.process (l, driveLevel);
                float filteredR = filterR.process (r, driveLevel);

                if (p.fltRoute == 1)
                {
                    filteredL = filter2L.process (filteredL, driveLevel);
                    filteredR = filter2R.process (filteredR, driveLevel);
                }
                else if (p.fltRoute == 2)
                {
                    const float parallelL = filter2L.process (l, driveLevel);
                    const float parallelR = filter2R.process (r, driveLevel);
                    filteredL = (filteredL + parallelL) * 0.5f;
                    filteredR = (filteredR + parallelR) * 0.5f;
                }

                const float wet = p.fltMix;
                l = l + (filteredL - l) * wet;
                r = r + (filteredR - r) * wet;
            }

            if (distortionActive && ! p.distPre)
                distortion.process (l, r);

            float gain = amp * velocityGain;

            if (widthGain != 1.0f)
            {
                const float mid = (l + r) * 0.5f;
                const float side = (l - r) * 0.5f * widthGain;
                l = mid + side;
                r = mid - side;
            }

            l = dcL.process (l) * gain;
            r = dcR.process (r) * gain;

            outL[i] += l;
            outR[i] += r;
        }
    }

    bool isActive() const noexcept { return ampEnv.isActive(); }
    bool isHeld() const noexcept { return held; }
    bool isReleasing() const noexcept { return ampEnv.isReleasing(); }
    int note() const noexcept { return (int) targetNote; }
    float age() const noexcept { return ageCounter; }
    void age (float amount) noexcept { ageCounter += amount; }
    float envelopeLevel() const noexcept { return ampEnv.getValue(); }
    float filterLevel() const noexcept { return filterEnv.getValue(); }
    float pitchLevel() const noexcept { return pitchEnv.getValue(); }
    float modEnvLevel() const noexcept { return modEnv.getValue(); }
    float velocityValue() const noexcept { return velocity; }
    float currentPitch() const noexcept { return glideNote; }

    size_t memoryBytes() const noexcept
    {
        size_t total = sizeof (Voice);
        for (const auto& buffer : combBuffers)
            total += buffer.capacity() * sizeof (float);
        return total;
    }

private:
    double sr = 48000.0;
    int combSize = 512;

    OscUnit osc1, osc2, osc3;
    ChannelFilter filterL, filterR, filter2L, filter2R;
    std::array<std::vector<float>, 4> combBuffers;
    DistortionUnit distortion;
    AdsrEnvelope ampEnv, filterEnv, pitchEnv, modEnv;
    DcBlocker dcL, dcR;
    XorShiftRng rng;

    VoiceParams p;
    float glideNote = 60.0f, targetNote = 60.0f, velocity = 0.0f;
    float ageCounter = 0.0f;
    bool held = false;

    float subPhase = 0.0f, subIncrement = 0.0f;
    float fmFeedbackState = 0.0f, fmAmount = 0.0f, fmFeedbackAmount = 0.0f, ringAmount = 0.0f;
    float formantShift = 0.0f, widthGain = 1.0f, velocityGain = 1.0f;
    float driveLevel = 0.0f;
    bool osc1Active = true, osc2Active = false, osc3Active = false, subActive = false;
    bool fmIsActive = false, distortionActive = false, fltMixActive = true;
};

} // namespace mk
