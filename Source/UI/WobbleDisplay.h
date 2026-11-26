#pragma once

// Animated wobble + output display. Pulls data from the engine's lock-free
// rings at 24 Hz; it never touches the audio thread directly and costs a few
// hundred microseconds per repaint.

#include "DSP/Engine.h"
#include "UI/Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace mk
{

class WobbleDisplay final : public juce::Component, private juce::Timer
{
public:
    explicit WobbleDisplay (const Engine& engineToUse);
    ~WobbleDisplay() override;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    void refreshData();

    const Engine& engine;
    std::array<float, 256> wobbleTrace {};
    std::array<float, 256> scope {};
    float phase = 0.0f;
    float value = 0.0f;
    float rate = 2.0f;
    float peak = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WobbleDisplay)
};

// Compact output meter used in the top bar.
class LevelMeter final : public juce::Component, private juce::Timer
{
public:
    explicit LevelMeter (const Engine& engineToUse);

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    void refreshData();

    const Engine& engine;
    float level = 0.0f;
    float peak = 0.0f;
};

} // namespace mk
