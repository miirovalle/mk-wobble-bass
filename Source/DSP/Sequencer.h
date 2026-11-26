#pragma once

// Built-in 16-step sequencer for the on-screen piano roll.
//
// The pattern is stored as one bitmask per step (bit = semitone row), so the UI
// can paint and toggle cells from the message thread while the audio thread
// reads the same words atomically. No locks, no allocation, no JUCE.

#include "DSP/MkCommon.h"

#include <array>
#include <atomic>

namespace mk
{

class Sequencer
{
public:
    static constexpr int kSteps = 16;
    static constexpr int kRows = 25;        // C0 (MIDI 24) .. C2 (MIDI 48)
    static constexpr int kBaseNote = 24;
    static constexpr float kGate = 0.85f;   // note length as a share of the step
    static constexpr int kMaxEvents = 64;   // 25 note-ons + 25 note-offs + slack

    struct Event
    {
        int sampleOffset = 0;
        int note = 0;
        bool noteOn = false;
    };

    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate > 0.0 ? sampleRate : 48000.0;
        reset();
    }

    void reset() noexcept
    {
        stepIndex = kSteps - 1;
        stepPhase = 0.0;
        gatePending = false;
        activeMask = 0;
        restartPending = true;
        stepDisplay.store (0, std::memory_order_relaxed);
    }

    void setPlaying (bool shouldPlay) noexcept
    {
        const bool wasPlaying = playing.exchange (shouldPlay, std::memory_order_relaxed);
        if (shouldPlay && ! wasPlaying)
            restartPending = true;
    }

    bool isPlaying() const noexcept { return playing.load (std::memory_order_relaxed); }

    void setBpm (double newBpm) noexcept
    {
        tempoValue.store (clampValue (newBpm, 20.0, 300.0), std::memory_order_relaxed);
    }

    double bpm() const noexcept { return tempoValue.load (std::memory_order_relaxed); }

    // 0 = 1/8, 1 = 1/16, 2 = 1/32
    void setDivision (int value) noexcept
    {
        divisionValue.store (clampValue (value, 0, 2), std::memory_order_relaxed);
    }

    int division() const noexcept { return divisionValue.load (std::memory_order_relaxed); }

    static const char* divisionName (int value) noexcept
    {
        static const char* const names[] = { "1/8", "1/16", "1/32" };
        return names[clampValue (value, 0, 2)];
    }

    //--------------------------------------------------------------------------
    // Pattern access (message thread). All of these are safe while playing.
    bool isSet (int step, int row) const noexcept
    {
        if (! inRange (step, row))
            return false;
        return (pattern[(size_t) step].load (std::memory_order_relaxed) & (1u << row)) != 0;
    }

    void set (int step, int row, bool on) noexcept
    {
        if (! inRange (step, row))
            return;

        auto& word = pattern[(size_t) step];
        const uint32_t bit = 1u << row;
        uint32_t current = word.load (std::memory_order_relaxed);

        for (;;)
        {
            const uint32_t next = on ? (current | bit) : (current & ~bit);
            if (next == current || word.compare_exchange_weak (current, next,
                                                               std::memory_order_relaxed))
                return;
        }
    }

    void toggle (int step, int row) noexcept { set (step, row, ! isSet (step, row)); }

    void clear() noexcept
    {
        for (auto& word : pattern)
            word.store (0u, std::memory_order_relaxed);
    }

    // Transposes the whole pattern by whole octaves, clamped to the grid.
    void transposeOctaves (int octaves) noexcept
    {
        for (auto& word : pattern)
        {
            const uint32_t current = word.load (std::memory_order_relaxed);
            uint32_t moved = 0;
            for (int row = 0; row < kRows; ++row)
                if ((current & (1u << row)) != 0)
                    moved |= 1u << clampValue (row + octaves * 12, 0, kRows - 1);
            word.store (moved, std::memory_order_relaxed);
        }
    }

    uint32_t stepWord (int step) const noexcept
    {
        return pattern[(size_t) clampValue (step, 0, kSteps - 1)].load (std::memory_order_relaxed);
    }

    void setStepWord (int step, uint32_t word) noexcept
    {
        pattern[(size_t) clampValue (step, 0, kSteps - 1)].store (word & ((1u << kRows) - 1u),
                                                                 std::memory_order_relaxed);
    }

    int currentStep() const noexcept { return stepDisplay.load (std::memory_order_relaxed); }

    static int rowToNote (int row) noexcept { return kBaseNote + clampValue (row, 0, kRows - 1); }

    //--------------------------------------------------------------------------
    // Audio thread: advance the clock and report the note events of this block.
    int advance (int numSamples, double hostBpm, bool hostPlaying,
                 Event* events, int maxEvents) noexcept
    {
        int count = 0;

        if (! isPlaying())
        {
            if (activeMask != 0)
            {
                emitAllOffs (events, count, maxEvents, 0);
                gatePending = false;
            }
            return count;
        }

        const double tempo = (hostPlaying && hostBpm > 1.0) ? hostBpm : bpm();
        const int stepsPerBeat = division() == 0 ? 2 : (division() == 1 ? 4 : 8);
        const double stepSamples = std::max (4.0, sr * 60.0 / tempo / (double) stepsPerBeat);
        const double gateLength = stepSamples * (double) kGate;

        if (restartPending)
        {
            restartPending = false;
            if (activeMask != 0)
                emitAllOffs (events, count, maxEvents, 0);
            stepIndex = kSteps - 1;
            stepPhase = stepSamples;   // the first sample lands on step 0
            gatePending = false;
        }

        for (int i = 0; i < numSamples; ++i)
        {
            stepPhase += 1.0;

            if (gatePending && stepPhase >= gateLength)
            {
                emitAllOffs (events, count, maxEvents, i);
                gatePending = false;
            }

            if (stepPhase >= stepSamples)
            {
                if (gatePending)
                {
                    emitAllOffs (events, count, maxEvents, i);
                    gatePending = false;
                }

                stepPhase -= stepSamples;
                stepIndex = (stepIndex + 1) % kSteps;

                const uint32_t mask = pattern[(size_t) stepIndex].load (std::memory_order_relaxed);
                activeMask = mask;

                for (int row = 0; row < kRows; ++row)
                {
                    if ((mask & (1u << row)) == 0)
                        continue;

                    if (count < maxEvents)
                        events[count++] = { i, rowToNote (row), true };
                }

                gatePending = mask != 0;
                stepDisplay.store (stepIndex, std::memory_order_relaxed);
            }
        }

        return count;
    }

private:
    static bool inRange (int step, int row) noexcept
    {
        return step >= 0 && step < kSteps && row >= 0 && row < kRows;
    }

    void emitAllOffs (Event* events, int& count, int maxEvents, int offset) noexcept
    {
        for (int row = 0; row < kRows; ++row)
        {
            if ((activeMask & (1u << row)) == 0)
                continue;
            if (count < maxEvents)
                events[count++] = { offset, rowToNote (row), false };
        }

        activeMask = 0;
    }

    std::array<std::atomic<uint32_t>, (size_t) kSteps> pattern {};
    std::atomic<bool> playing { false };
    std::atomic<double> tempoValue { 120.0 };
    std::atomic<int> divisionValue { 1 };
    std::atomic<int> stepDisplay { 0 };

    double sr = 48000.0;
    double stepPhase = 0.0;
    int stepIndex = kSteps - 1;
    uint32_t activeMask = 0;
    bool gatePending = false;
    bool restartPending = true;
};

} // namespace mk
