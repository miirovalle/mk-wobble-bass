#pragma once

// The whole interface: one drawable wobble shape, three knobs and the output
// waveform. Nothing else.

#include "DSP/Engine.h"
#include "UI/Theme.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace mk
{

// Rotary knob bound to a parameter.
class ParamKnob final : public juce::Component
{
public:
    ParamKnob (juce::AudioProcessorValueTreeState& state, int paramIndex,
               const juce::String& labelOverride = {});
    ~ParamKnob() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    juce::String labelText;
    int index = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamKnob)
};

// Rotary that picks what the wobble modulates. It is not a parameter: changing
// it routes the wobble to one destination and zeroes the others, which is the
// "different types of wobble" control.
class TypeKnob final : public juce::Component
{
public:
    explicit TypeKnob (juce::AudioProcessorValueTreeState& state);

    void paint (juce::Graphics&) override;
    void resized() override;

    static int typeCount() noexcept;
    static const char* typeName (int index) noexcept;

private:
    void applyType (int type);

    juce::AudioProcessorValueTreeState& apvts;
    juce::Slider slider;
    int current = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TypeKnob)
};

// Rotary that sets the wobble rate in Hz and always leaves the engine in free
// mode, so the knob can never appear dead (the engine's other option is host
// tempo sync, which the rate knob cannot express).
class RateKnob final : public juce::Component
{
public:
    explicit RateKnob (juce::AudioProcessorValueTreeState& state);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::Slider slider;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    float value = 2.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RateKnob)
};

// One LFO: tap the header to change shape, drag the curve for rate (X) and
// depth (Y), tap SYNC to lock to the host, tap the target to choose what it
// modulates. The slot also routes itself in the mod matrix, so it is audible
// without opening anything else.
class LfoSlot final : public juce::Component, private juce::Timer
{
public:
    LfoSlot (juce::AudioProcessorValueTreeState&, int lfoIndex);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void cycleShape();
    void cycleTarget();
    void toggleSync();
    void applyRouting();

    juce::Rectangle<int> headerArea() const;
    juce::Rectangle<int> curveArea() const;
    juce::Rectangle<int> syncArea() const;
    juce::Rectangle<int> targetArea() const;

    juce::AudioProcessorValueTreeState& apvts;
    int lfo = 0;
    int target = 0;
    float displayPhase = 0.0f;
    bool dragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LfoSlot)
};

// The drawable LFO: drag on the canvas to draw the wobble shape.
class DrawableLfo final : public juce::Component, private juce::Timer
{
public:
    DrawableLfo (juce::AudioProcessorValueTreeState&, Engine&);
    ~DrawableLfo() override;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

    // Replaces the whole curve with a classic shape and switches to custom mode.
    void loadShape (int shape);

private:
    void timerCallback() override;
    void writePoint (juce::Point<float> position);

    juce::Rectangle<float> canvasArea() const;

    juce::AudioProcessorValueTreeState& apvts;
    Engine& engine;
    float phase = 0.0f;
    float value = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrawableLfo)
};

// Live output waveform.
class WaveScope final : public juce::Component, private juce::Timer
{
public:
    explicit WaveScope (const Engine&);
    ~WaveScope() override;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    const Engine& engine;
    std::array<float, 512> samples {};
    float peak = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveScope)
};

// Compact output meter for the header.
class LevelMeter final : public juce::Component, private juce::Timer
{
public:
    explicit LevelMeter (const Engine&);
    ~LevelMeter() override;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    const Engine& engine;
    float level = 0.0f;
    float peak = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeter)
};

} // namespace mk
