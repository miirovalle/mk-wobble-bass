#pragma once

// Graphical, touch-first controls. Nothing in this plug-in is a knob: every
// parameter is either a picture you drag on (XY pads, wobble curve, LFO slots),
// a tap tile, or a slim bar in the ADVANCED drawer.

#include "DSP/Engine.h"
#include "DSP/Sequencer.h"
#include "UI/Theme.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace mk
{

// Slim parameter bar: label above, filled bar with the value on the right.
class ParamSlider final : public juce::Component
{
public:
    ParamSlider (juce::AudioProcessorValueTreeState& state, int paramIndex,
                 const juce::String& labelOverride = {});
    ~ParamSlider() override;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    void applyDrag (juce::Point<float> position);

    juce::RangedAudioParameter* parameter = nullptr;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    juce::String labelText;
    int index = 0;
    float plainValue = 0.0f;
    float normalised = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamSlider)
};

// Choice parameter rendered as a compact drop-down with its label above.
class ParamCombo final : public juce::Component
{
public:
    ParamCombo (juce::AudioProcessorValueTreeState& state, int paramIndex,
                const juce::String& labelOverride = {});
    ~ParamCombo() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::ComboBox combo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    juce::String labelText;
    int index = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamCombo)
};

// Rounded panel with a title, drawn in the plug-in palette.
class Section final : public juce::Component
{
public:
    explicit Section (juce::String title);

    void paint (juce::Graphics&) override;

private:
    juce::String title;
};

// Filled, scrolling view of the engine's output ring.
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

// Two-parameter pad. X/Y drag writes straight to the parameters and the handle
// follows host automation, so the picture never lies.
class XyPad : public juce::Component, private juce::Timer
{
public:
    XyPad (juce::AudioProcessorValueTreeState& state, int xParam, int yParam,
           juce::String title);
    ~XyPad() override;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

protected:
    virtual void paintField (juce::Graphics&, juce::Rectangle<float>) = 0;

    juce::Rectangle<float> fieldArea() const;
    juce::AudioProcessorValueTreeState& apvts;
    int xIndex = 0, yIndex = 0;
    juce::String title;
    juce::Point<float> handle { 0.5f, 0.5f };

private:
    void timerCallback() override;
    juce::RangedAudioParameter* xParameter = nullptr;
    juce::RangedAudioParameter* yParameter = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XyPad)
};

// FILTER pad: cutoff on X, resonance on Y, drawn as a response curve.
class FilterPad final : public XyPad
{
public:
    explicit FilterPad (juce::AudioProcessorValueTreeState&);

protected:
    void paintField (juce::Graphics&, juce::Rectangle<float>) override;
};

// GROWL pad: distortion drive on X, wobble depth on Y, drawn as a wave that
// folds harder as you push right.
class GrowlPad final : public XyPad
{
public:
    explicit GrowlPad (juce::AudioProcessorValueTreeState&);

protected:
    void paintField (juce::Graphics&, juce::Rectangle<float>) override;
};

// The wobble engine as a big scrolling curve: drag X for rate, Y for depth,
// tap a shape icon to change the curve.
class WobblePanel final : public juce::Component, private juce::Timer
{
public:
    WobblePanel (juce::AudioProcessorValueTreeState&, const Engine&);
    ~WobblePanel() override;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    juce::Rectangle<int> shapeStripArea() const;
    int shapeAt (juce::Point<int>) const;

    juce::AudioProcessorValueTreeState& apvts;
    const Engine& engine;
    bool draggingCurve = false;
    bool draggingShape = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WobblePanel)
};

// "Different ways to wobble", as eight tiles you just tap: tempo sync, filter,
// resonance, pitch, wavetable, drive, width, formant.
class WobbleTargets final : public juce::Component, private juce::Timer
{
public:
    explicit WobbleTargets (juce::AudioProcessorValueTreeState&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    struct Tile
    {
        const char* label;
        int paramIndex;     // -1 for the tempo-sync tile
        float onAmount;
    };

    void timerCallback() override;
    juce::Rectangle<int> tileBounds (int index) const;

    juce::AudioProcessorValueTreeState& apvts;
    std::array<Tile, 8> tiles;
    std::array<bool, 8> active {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WobbleTargets)
};

// One LFO as a picture: drag X for rate, Y for depth, tap the shape chip to
// cycle the curve, tap SYNC to lock it to the host tempo.
class LfoSlot final : public juce::Component, private juce::Timer
{
public:
    LfoSlot (juce::AudioProcessorValueTreeState&, int lfoIndex);
    ~LfoSlot() override;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    juce::Rectangle<int> shapeArea() const;
    juce::Rectangle<int> syncArea() const;
    juce::Rectangle<int> curveArea() const;

    juce::AudioProcessorValueTreeState& apvts;
    int lfo = 0;             // 0-based LFO number
    float displayPhase = 0.0f;
    bool draggingCurve = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LfoSlot)
};

// Shared waveform-picture helpers.
float lfoShapeValue (int shape, float phase01) noexcept;
void drawWavePicture (juce::Graphics&, juce::Rectangle<float> area, int shape,
                      float cycles, float depth, juce::Colour colour);

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
