#pragma once

// Master effect chain. Every stage is bypassable and each buffer is allocated
// exactly once in prepare(), so the audio thread never touches the allocator.
//
// RAM budget at 48 kHz: chorus 24 KB, flanger 16 KB, phaser ~0 (state only),
// delay 400 KB, reverb 48 KB. In Eco quality the delay halves its buffer.

#include "DSP/MkCommon.h"

#include <vector>

namespace mk
{

struct EffectSettings
{
    bool chorusOn = false;
    float chorusRate = 0.6f, chorusDepth = 0.4f, chorusMix = 0.3f, chorusWidth = 0.7f;

    bool phaserOn = false;
    float phaserRate = 0.3f, phaserDepth = 0.6f, phaserFb = 0.4f, phaserMix = 0.3f;
    int phaserStages = 4;

    bool flangerOn = false;
    float flangerRate = 0.25f, flangerDepth = 0.5f, flangerFb = 0.4f;
    float flangerDelay = 0.004f, flangerMix = 0.3f;

    bool delayOn = false;
    float delaySeconds = 0.375f, delayFeedback = 0.35f, delayMix = 0.25f, delayTone = 0.35f;
    bool delayPingPong = true;

    bool reverbOn = false;
    float reverbSize = 0.5f, reverbDecay = 0.5f, reverbDamp = 0.5f, reverbWidth = 0.8f, reverbMix = 0.2f;

    bool compressorOn = true;
    float compThresholdDb = -12.0f, compRatio = 4.0f, compAttackMs = 6.0f, compReleaseMs = 120.0f;
    float compMakeupDb = 2.0f;

    bool multibandOn = false;
    float multibandDrive = 0.3f, multibandMix = 0.5f;

    bool ottOn = false;
    float ottAmount = 0.7f, ottMix = 1.0f;

    // Modulated by the wobble LFO when routed there.
    float wobbleValue = 0.0f;
};

class MasterEffects
{
public:
    void prepare (double sampleRate, bool ecoMode);
    void reset();
    void setSettings (const EffectSettings&) noexcept;
    void process (float* left, float* right, int numSamples) noexcept;

    float gainReductionDb() const noexcept { return lastGainReduction; }

    size_t memoryBytes() const noexcept
    {
        size_t total = sizeof (MasterEffects);
        total += (chorusLine.capacity() + flangerLine.capacity() + delayLine.capacity()) * sizeof (float);
        for (const auto& b : combBuffer)
            total += b.capacity() * sizeof (float);
        for (const auto& b : allpassBuffer)
            total += b.capacity() * sizeof (float);
        return total;
    }

private:
    void processChorus (float* l, float* r, int n) noexcept;
    void processPhaser (float* l, float* r, int n) noexcept;
    void processFlanger (float* l, float* r, int n) noexcept;
    void processDelay (float* l, float* r, int n) noexcept;
    void processReverb (float* l, float* r, int n) noexcept;
    void processCompressor (float* l, float* r, int n) noexcept;
    void processMultiband (float* l, float* r, int n) noexcept;
    void processOtt (float* l, float* r, int n) noexcept;

    double sr = 48000.0;
    EffectSettings settings;

    // Chorus / flanger shared topology, separate lines.
    std::vector<float> chorusLine, flangerLine;
    int chorusWrite = 0, flangerWrite = 0;
    float chorusPhase = 0.0f, flangerPhase = 0.0f;
    float chorusLp[2] {}, flangerLp[2] {};

    // Phaser allpass state.
    float phaserZ[2][12] {};
    float phaserPhase = 0.0f;

    // Delay.
    std::vector<float> delayLine;
    int delayWrite = 0, delayMax = 1;
    float delayDamp[2] {};

    // Reverb (Freeverb tuning).
    static constexpr int kCombs = 8;
    static constexpr int kAllpasses = 4;
    std::vector<float> combBuffer[kCombs];
    std::vector<float> allpassBuffer[kAllpasses];
    int combIndex[kCombs] {};
    int allpassIndex[kAllpasses] {};
    float combStore[kCombs] {};
    float allpassStore[kAllpasses] {};
    float reverbLp[2] {};

    // Compressor.
    float compEnv = 0.0f, compAttackCoeff = 0.0f, compReleaseCoeff = 0.0f;
    float lastGainReduction = 0.0f;

    // Multiband crossover state.
    float bandLow[2] {}, bandHigh[2] {};
    float xoverCoeffLo = 0.0f, xoverCoeffHi = 0.0f;

    // OTT-style band compressor: three bands, fast attack, slow release.
    float ottLow[2] {}, ottHigh[2] {};
    float ottEnv[3][2] {};
    float ottAttackCoeff = 0.0f, ottReleaseCoeff = 0.0f;

    float dcState[2] {};
};

} // namespace mk
