#pragma once

#include "Plugin/PluginProcessor.h"
#include "UI/Pages.h"
#include "UI/PresetBrowser.h"
#include "UI/Theme.h"
#include "UI/WobbleDisplay.h"

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
    enum class Page : int { Main = 0, Voice, Osc, Filter, WobblePage, Mod, Dist, Fx, Matrix, kCount };

    void timerCallback() override;
    void showPage (Page page);
    void openBrowser();
    void closeBrowser();
    void updateHeader();
    void layoutPage();
    int keyboardHeight() const;

    WobbleAudioProcessor& processor;
    MkLookAndFeel lookAndFeel;

    juce::Label titleLabel;
    juce::Label presetLabel;
    juce::Label infoLabel;
    juce::TextButton previousButton { "<" };
    juce::TextButton nextButton { ">" };
    juce::TextButton browserButton { "PRESETS" };
    LevelMeter meter;

    // Compact playable keyboard strip.

    std::array<std::unique_ptr<juce::TextButton>, (size_t) Page::kCount> pageButtons;
    std::array<std::unique_ptr<juce::Component>, (size_t) Page::kCount> pageComponents;
    std::array<std::unique_ptr<juce::Viewport>, (size_t) Page::kCount> pageViewports;

    std::unique_ptr<PresetBrowser> browser;
    juce::MidiKeyboardComponent keyboard;
    Page currentPage = Page::Main;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WobbleAudioProcessorEditor)
};

} // namespace mk
