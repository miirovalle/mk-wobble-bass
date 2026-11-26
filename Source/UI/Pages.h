#pragma once

// The interface: EASY (six macro knobs, bass characters, wobble rate) and
// ADVANCED (oscillators, filters, envelopes, LFOs, matrix, distortion, effects,
// gate). Advanced sections are built from ParamPanel grids driven by the
// parameter metadata, so a new parameter only needs a line in a section list to
// appear in the UI.

#include "DSP/Engine.h"
#include "DSP/GatePresets.h"
#include "UI/GatePattern.h"
#include "UI/GraphControls.h"
#include "UI/Theme.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <vector>

namespace mk
{

struct ParamSpec
{
    int index;
    juce::String label;
};

inline ParamSpec paramSpec (int index, const juce::String& label = {})
{
    return { index, label };
}

// A titled panel holding a grid of knobs and drop-downs. Choice parameters
// automatically become drop-downs; everything else becomes a rotary.
class ParamPanel final : public juce::Component
{
public:
    ParamPanel (juce::AudioProcessorValueTreeState&, juce::String title,
                std::vector<ParamSpec> params, int columns);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::String titleText;
    int columnCount;
    juce::OwnedArray<juce::Component> widgets;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamPanel)
};

struct PanelSpec
{
    juce::String title;
    std::vector<ParamSpec> params;
    int columns = 4;
    float weight = 1.0f;
};

// A page made of side-by-side panels.
class SectionPage final : public juce::Component
{
public:
    SectionPage (juce::AudioProcessorValueTreeState&, std::vector<PanelSpec>);

    void resized() override;

private:
    juce::OwnedArray<ParamPanel> panels;
    std::vector<float> weights;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SectionPage)
};

// Row of six bass-character buttons bound to one choice parameter.
class CharacterSelector final : public juce::Component
{
public:
    CharacterSelector (juce::AudioProcessorValueTreeState&, int paramIndex, const juce::String& title);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void select (int index);
    void refresh (int index);

    juce::AudioProcessorValueTreeState& apvts;
    int parameterIndex = 0;
    std::array<juce::TextButton, kCharacterCount> buttons;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    juce::String caption;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CharacterSelector)
};

// Tempo-locked wobble rate: every straight, triplet and dotted division.
class RateSelector final : public juce::Component
{
public:
    explicit RateSelector (juce::AudioProcessorValueTreeState&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void refresh (float division);

    juce::AudioProcessorValueTreeState& apvts;
    juce::ComboBox combo;
    std::unique_ptr<juce::ParameterAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RateSelector)
};

class EasyPage final : public juce::Component
{
public:
    EasyPage (juce::AudioProcessorValueTreeState&, Engine&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    CharacterSelector character;
    ParamChoice morphTo;
    RateSelector rate;
    ParamKnob wobble, bass, growl, grit, movement, width;
    ParamKnob morph, subLevel, master;
    WobbleStrip wobbleStrip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EasyPage)
};

// Twelve-slot modulation matrix plus the macro knobs.
class MatrixPage final : public juce::Component
{
public:
    explicit MatrixPage (juce::AudioProcessorValueTreeState&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Row
    {
        juce::ComboBox source, destination;
        juce::Slider amount;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> destinationAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttachment;
    };

    juce::AudioProcessorValueTreeState& apvts;
    std::array<std::unique_ptr<Row>, kMatrixSlots> rows;
    ParamPanel macros;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MatrixPage)
};

// The original drawable wobble, kept as the ADVANCED editing view.
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

class AdvancedPage final : public juce::Component
{
public:
    AdvancedPage (juce::AudioProcessorValueTreeState&, Engine&);

    void paint (juce::Graphics&) override;
    void resized() override;

    void refresh();

private:
    void showSection (int index);

    static constexpr int kSectionCount = 9;

    juce::AudioProcessorValueTreeState& apvts;
    Engine& engine;
    std::array<juce::TextButton, kSectionCount> tabs;
    std::array<juce::Component*, kSectionCount> sections {};
    juce::OwnedArray<juce::Component> owned;
    GatePage* gatePage = nullptr;
    int current = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdvancedPage)
};

} // namespace mk

