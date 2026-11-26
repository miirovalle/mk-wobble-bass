#pragma once

// Touch-friendly parameter widgets. Every control is bound directly to an
// APVTS parameter, so nothing in the UI can drift from the engine state, and
// host automation moves the on-screen control.

#include "DSP/Params.h"
#include "UI/Theme.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace mk
{

class ParamKnob final : public juce::Component
{
public:
    ParamKnob (juce::AudioProcessorValueTreeState& state, int paramIndex,
               const juce::String& labelOverride = {});
    ~ParamKnob() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void setCompact (bool shouldBeCompact);

    // Right-click (or a two-finger tap on iPad) arms MIDI learn for this knob.
    std::function<void (int paramIndex)> onLearnRequest;

    int paramIndex() const noexcept { return index; }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    juce::String labelText;
    int index = 0;
    bool compact = false;
    float cachedValue = 0.0f;

    void refreshValueText();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamKnob)
};

class ParamCombo final : public juce::Component
{
public:
    ParamCombo (juce::AudioProcessorValueTreeState& state, int paramIndex,
                const juce::String& labelOverride = {});
    ~ParamCombo() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    int paramIndex() const noexcept { return index; }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::ComboBox combo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    juce::String labelText;
    int index = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamCombo)
};

// A titled panel that groups controls, drawn with the plug-in palette.
class SectionPanel final : public juce::Component
{
public:
    explicit SectionPanel (juce::String title);

    void paint (juce::Graphics&) override;

private:
    juce::String title;
};

} // namespace mk
