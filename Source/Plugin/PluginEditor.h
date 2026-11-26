#pragma once

// Two tabs, one tool each:
//   WOBBLE - draw the wobble shape, then RATE / DEPTH / TYPE
//   GATE   - a 64-step trance gate with 100 rhythms
// The output waveform sits under the header on both tabs.

#include "Plugin/PluginProcessor.h"
#include "UI/GraphControls.h"
#include "UI/Pages.h"
#include "UI/PresetBrowser.h"
#include "UI/Theme.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

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
    void showTab (int index);
    void openBrowser();
    void closeBrowser();

    int keyboardHeight() const;

    WobbleAudioProcessor& processor;
    MkLookAndFeel lookAndFeel;

    juce::Label titleLabel;
    juce::Label presetLabel;
    juce::TextButton browserButton { "PRESETS" };
    juce::TextButton wobbleTab { "WOBBLE" };
    juce::TextButton gateTab { "GATE" };

    WaveScope scope;
    WobblePage wobblePage;
    GatePage gatePage;

    std::unique_ptr<PresetBrowser> browser;
    juce::MidiKeyboardComponent keyboard;

    int currentTab = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WobbleAudioProcessorEditor)
};

} // namespace mk
