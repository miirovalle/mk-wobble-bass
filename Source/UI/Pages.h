#pragma once

// The two tabs: WOBBLE (drawable LFO + three knobs) and GATE (trance gate).

#include "DSP/Engine.h"
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

    // Keeps the SYNC chip in step with host automation of the wobble mode.
    void refresh();

private:
    DrawableLfo canvas;
    ParamKnob rate;
    ParamKnob depth;
    TypeKnob type;
    juce::TextButton syncButton { "SYNC" };
    juce::TextButton sineButton { "SINE" };
    juce::TextButton triButton { "TRI" };
    juce::TextButton sawButton { "SAW" };
    juce::TextButton squareButton { "SQUARE" };
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
    GatePattern pattern;
    ParamKnob rate;
    ParamKnob depth;
    ParamKnob smooth;
    juce::TextButton gateButton { "GATE ON" };

    juce::AudioProcessorValueTreeState& apvts;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GatePage)
};

} // namespace mk
