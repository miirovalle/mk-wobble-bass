#pragma once

// One screen, no knobs:
//   left  - live waveform + the piano roll sequencer
//   right - wobble curve, wobble-target tiles, three LFO slots, filter/growl pads
// The ADVANCED drawer holds every remaining parameter as slim bars.

#include "Plugin/PluginProcessor.h"
#include "UI/GraphControls.h"
#include "UI/Pages.h"
#include "UI/PianoRoll.h"
#include "UI/PresetBrowser.h"
#include "UI/Theme.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <array>
#include <memory>

namespace mk
{

class WobbleAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                         private juce::Timer
{
public:
    explicit WobbleAudioProcessorEditor (WobbleAudioProcessor&);
    ~WobbleAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateHeader();
    void openBrowser();
    void closeBrowser();
    void showAdvanced (bool shouldShow);

    int keyboardHeight() const;

    WobbleAudioProcessor& processor;
    MkLookAndFeel lookAndFeel;

    juce::Label titleLabel;
    juce::Label presetLabel;
    juce::Label infoLabel;
    juce::TextButton previousButton { "<" };
    juce::TextButton nextButton { ">" };
    juce::TextButton browserButton { "PRESETS" };
    juce::TextButton advancedButton { "ADVANCED" };
    LevelMeter meter;

    WaveScope scope;
    PianoRoll pianoRoll;
    WobblePanel wobblePanel;
    WobbleTargets wobbleTargets;
    std::array<std::unique_ptr<LfoSlot>, 3> lfoSlots;
    FilterPad filterPad;
    GrowlPad growlPad;

    juce::Viewport advancedViewport;
    std::unique_ptr<AdvancedPage> advancedPage;
    bool advancedVisible = false;

    std::unique_ptr<PresetBrowser> browser;

    juce::MidiKeyboardComponent keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WobbleAudioProcessorEditor)
};

} // namespace mk
