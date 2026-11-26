#pragma once

// Preset browser overlay: search, category filter, favourites, factory and user
// presets, save / delete / import / export, plus the MIDI learn row.

#include "Plugin/PluginProcessor.h"
#include "UI/Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace mk
{

class PresetBrowser final : public juce::Component,
                            private juce::ListBoxModel,
                            private juce::Timer
{
public:
    explicit PresetBrowser (WobbleAudioProcessor& processor);
    ~PresetBrowser() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void refreshList();
    std::function<void()> onClose;
    std::function<void()> onPresetLoaded;

private:
    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override;
    void returnKeyPressed (int row) override;
    void timerCallback() override { updateStatus(); }

    void rebuild();
    void updateStatus();
    void saveCurrent (bool saveAs);

    WobbleAudioProcessor& processor;
    juce::TextEditor search;
    juce::ComboBox category;
    juce::ListBox list { "presets", this };
    juce::TextEditor nameEditor;
    juce::ComboBox categoryEditor;
    juce::TextButton loadButton { "LOAD" };
    juce::TextButton saveButton { "SAVE" };
    juce::TextButton deleteButton { "DELETE" };
    juce::TextButton importButton { "IMPORT" };
    juce::TextButton exportButton { "EXPORT" };
    juce::TextButton favouriteButton { "FAV" };
    juce::TextButton learnButton { "MIDI LEARN" };
    juce::TextButton closeButton { "CLOSE" };
    juce::Label status;
    juce::Label learnStatus;

    std::vector<int> filtered;
    int selectedRow = 0;

    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBrowser)
};

} // namespace mk
