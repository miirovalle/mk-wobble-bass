#pragma once

// Page components. Pages are generated from the parameter table, so every
// parameter in the engine has a real, working control and nothing is a
// placeholder.

#include "DSP/Params.h"
#include "UI/Knob.h"
#include "UI/WobbleDisplay.h"
#include "UI/Theme.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace mk
{

class WobbleAudioProcessor;

using LearnCallback = std::function<void (int paramIndex)>;

// Grid of knobs / combos for a list of parameter indices.
class ParamGridPage final : public juce::Component
{
public:
    ParamGridPage (juce::AudioProcessorValueTreeState& state, juce::String title,
                   std::vector<int> paramIndices, LearnCallback learnCallback);

    void paint (juce::Graphics&) override;
    void resized() override;

    void setColumns (int columns);
    int itemCount() const noexcept { return (int) controls.size(); }

private:
    juce::String title;
    std::vector<std::unique_ptr<juce::Component>> controls;
    int columns = 6;
};

class MainPage final : public juce::Component
{
public:
    MainPage (juce::AudioProcessorValueTreeState& state, const Engine& engine, LearnCallback learnCallback);

    void paint (juce::Graphics&) override;
    void resized() override;

    WobbleDisplay& display() noexcept { return wobbleDisplay; }

private:
    WobbleDisplay wobbleDisplay;
    ParamCombo wobbleRate;
    ParamKnob wobbleDepth;
    std::array<std::unique_ptr<ParamKnob>, kMacroCount> macros;
    juce::String title { "MAIN" };
};

class MatrixPage final : public juce::Component
{
public:
    MatrixPage (juce::AudioProcessorValueTreeState& state, LearnCallback learnCallback);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Row
    {
        std::unique_ptr<ParamCombo> source;
        std::unique_ptr<ParamCombo> destination;
        std::unique_ptr<ParamKnob> amount;
    };

    std::array<Row, (size_t) kMatrixSlots> rows;
};

} // namespace mk
