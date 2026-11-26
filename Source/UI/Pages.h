#pragma once

// The two tabs: WOBBLE (drawable LFO + three knobs) and GATE (trance gate).

#include "DSP/Engine.h"
#include "DSP/GatePresets.h"
#include "UI/GatePattern.h"
#include "UI/GraphControls.h"
#include "UI/Theme.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace mk
{

class WobblePage final : public juce::Component
{
public:
    WobblePage (juce::AudioProcessorValueTreeState&, Engine&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    DrawableLfo canvas;
    RateKnob rate;
    ParamKnob depth;
    TypeKnob type;
    juce::TextButton sineButton { "SINE" };
    juce::TextButton clearButton { "CLEAR" };

    juce::AudioProcessorValueTreeState& apvts;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WobblePage)
};

class GatePage final : public juce::Component
{
public:
    GatePage (juce::AudioProcessorValueTreeState&, Engine&);

    void paint (juce::Graphics&) override;
    void resized() override;

    void refresh();

private:
    void applyPreset (int index);

    GatePattern pattern;
    ParamKnob rate;
    ParamKnob depth;
    ParamKnob smooth;
    juce::TextButton gateButton { "GATE ON" };
    juce::TextButton presetPrev { "<" };
    juce::TextButton presetNext { ">" };
    juce::Label presetName;

    juce::AudioProcessorValueTreeState& apvts;
    Engine& engine;
    int presetIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GatePage)
};

class LfoPage final : public juce::Component
{
public:
    explicit LfoPage (juce::AudioProcessorValueTreeState&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    std::array<std::unique_ptr<LfoSlot>, 3> slots;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LfoPage)
};

} // namespace mk
