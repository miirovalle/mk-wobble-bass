#pragma once

// The plug-in shell: header (title, preset, RANDOM, PRESETS), live output
// scope, the EASY / ADVANCED tab switch and the playable keyboard.
//   EASY      - six macro knobs, bass character, morph, wobble rate
//   ADVANCED  - oscillators, filters, envelopes, LFOs, matrix, distortion,
//               effects and the trance gate

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
    int headerHeight() const;

    WobbleAudioProcessor& processor;
    MkLookAndFeel lookAndFeel;

    juce::Label titleLabel;
    juce::Label presetLabel;
    juce::TextButton randomButton { "RANDOM" };
    juce::TextButton browserButton { "PRESETS" };
    juce::TextButton easyTab { "EASY" };
    juce::TextButton advancedTab { "ADVANCED" };

    WaveScope scope;
    LevelMeter meter;
    EasyPage easyPage;
    AdvancedPage advancedPage;

    std::unique_ptr<PresetBrowser> browser;
    juce::MidiKeyboardComponent keyboard;

    int currentTab = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WobbleAudioProcessorEditor)
};

} // namespace mk
