#pragma once

// Top-level synthesizer. Owns the parameter store, the voice pool, the global
// modulation sources, the master effect chain and the metering.
//
// Everything the wrapper (VST3 / AUv3 / Standalone) has to do is: push host
// tempo, forward MIDI, call process(). No allocation, locking or file I/O ever
// happens on the audio path.

#include "DSP/Effects.h"
#include "DSP/Lfo.h"
#include "DSP/ParamStore.h"
#include "DSP/Params.h"
#include "DSP/Voice.h"

#include <atomic>

namespace mk
{

class Engine
{
public:
    static constexpr int kMaxVoices = 16;
    static constexpr int kControlBlock = 32;
    static constexpr int kScopeSize = 512;

    Engine();

    void prepare (double sampleRate, int maximumBlockSize);
    void reset();

    ParamStore& params() noexcept { return store; }
    const ParamStore& params() const noexcept { return store; }

    void noteOn (int midiNote, float velocity);
    void noteOff (int midiNote);
    void allNotesOff (bool immediate = false);
    void pitchBend (float normalised);          // -1..1
    void setModWheel (float value);             // 0..1
    void setAftertouch (float value);           // 0..1
    void setSustainPedal (bool down);

    void setTempo (double bpm) noexcept { hostBpm = bpm > 1.0 ? bpm : 120.0; }
    void setTransportPlaying (bool playing) noexcept { transportPlaying = playing; }
    double tempo() const noexcept { return hostBpm; }

    void process (float* left, float* right, int numSamples);

    int activeVoiceCount() const noexcept;
    int effectiveVoiceLimit() const noexcept { return std::min (voiceLimit, adaptiveLimit); }

    float outputMeter() const noexcept { return meterOut.load (std::memory_order_relaxed); }
    float outputPeak() const noexcept { return peakOut.load (std::memory_order_relaxed); }
    float wobbleValue() const noexcept { return wobbleDisplay.load (std::memory_order_relaxed); }
    float wobblePhase() const noexcept { return wobblePhaseDisplay.load (std::memory_order_relaxed); }
    float wobbleRateHz() const noexcept { return wobbleRateDisplay.load (std::memory_order_relaxed); }
    int wobbleShapeForDisplay() const noexcept { return wobbleShapeDisplay.load (std::memory_order_relaxed); }
    float wobbleDepthForDisplay() const noexcept { return wobbleDepthDisplay.load (std::memory_order_relaxed); }
    float filterDisplay() const noexcept { return filterDisplayValue.load (std::memory_order_relaxed); }
    float lastBlockMs() const noexcept { return lastBlockMsValue; }

    void copyScope (float* destination, int count) const noexcept;
    void copyWobbleTrace (float* destination, int count) const noexcept;

    // Actual resident footprint of the DSP: object state plus every buffer that
    // prepare() allocates. Reported by the benchmark and shown in the UI.
    size_t memoryFootprintBytes() const noexcept;

    double sampleRate() const noexcept { return sr; }
    int maximumBlockSize() const noexcept { return maxBlock; }

    // Host-side helper: the delay time implied by the current sync setting.
    float delayTimeSeconds() const noexcept;

private:
    void updateModulators (int numSamples, const ParamSnapshot& p) noexcept;
    void buildVoiceParams (const ParamSnapshot& p) noexcept;
    void applyMatrix (const ParamSnapshot& p, const Voice* voice, float randomValue,
                      ModValues& mod) const noexcept;
    void applyWobbleRouting (const ParamSnapshot& p, ModValues& mod) const noexcept;
    ModValues computeGlobalMod (const ParamSnapshot& p) const noexcept;
    EffectSettings makeEffectSettings (const ParamSnapshot& p, const ModValues& globalMod) const noexcept;
    void updateGlide (const ParamSnapshot& p, int chunk) noexcept;
    static float softLimit (float x) noexcept;
    void handleMonoNoteOn (int note, float velocity) noexcept;
    void handleMonoNoteOff (int note) noexcept;
    Voice* allocateVoice() noexcept;
    void updateAdaptiveLimit (int numSamples) noexcept;

    std::array<Voice, (size_t) kMaxVoices> voices;
    std::array<ModLfo, (size_t) kLfoCount> lfos;
    WobbleEngine wobble;
    MasterEffects effects;
    ParamStore store;

    VoiceParams voiceParams;
    ParamSnapshot workingParams;

    double sr = 48000.0;
    int maxBlock = 512;
    double hostBpm = 120.0;
    bool transportPlaying = false;

    int voiceLimit = 8;
    int adaptiveLimit = 8;
    int overloadBlocks = 0;
    int calmBlocks = 0;

    float bend = 0.0f;
    float modWheel = 0.0f;
    float aftertouch = 0.0f;
    bool sustainDown = false;
    std::array<bool, 128> heldNotes {};
    std::array<bool, 128> sustainedNotes {};

    int monoStack[16] {};
    int monoCount = 0;
    int lastPlayedNote = 60;

    float lfoValues[kLfoCount] {};
    float macroValues[kMacroCount] {};
    float wobbleSmoothed = 0.0f;
    float glideCoeff = 1.0f;
    std::array<float, (size_t) kMaxVoices> driftOffsets {};
    std::array<float, (size_t) kMaxVoices> voiceRandom {};
    uint32_t randomTick = 0;

    double lastBlockMsValue = 0.0;

    std::atomic<float> meterOut { 0.0f };
    std::atomic<float> peakOut { 0.0f };
    std::atomic<float> wobbleDisplay { 0.0f };
    std::atomic<float> wobblePhaseDisplay { 0.0f };
    std::atomic<float> wobbleRateDisplay { 2.0f };
    std::atomic<int> wobbleShapeDisplay { 0 };
    std::atomic<float> wobbleDepthDisplay { 0.0f };
    std::atomic<float> filterDisplayValue { 0.0f };

    std::array<std::atomic<float>, (size_t) kScopeSize> scope {};
    std::atomic<int> scopeWrite { 0 };
    std::array<std::atomic<float>, (size_t) kScopeSize> wobbleTrace {};
    std::atomic<int> wobbleWrite { 0 };

    XorShiftRng uiRandom;
};

} // namespace mk
