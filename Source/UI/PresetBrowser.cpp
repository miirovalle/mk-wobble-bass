#include "UI/PresetBrowser.h"

namespace mk
{

PresetBrowser::PresetBrowser (WobbleAudioProcessor& processorToUse)
    : processor (processorToUse)
{
    search.setTextToShowWhenEmpty ("Search presets", theme::textDim());
    search.setFont (theme::valueFont (15.0f));
    search.onTextChange = [this] { rebuild(); };
    addAndMakeVisible (search);

    category.addItem ("All categories", 1);
    int categoryCount = 0;
    const auto* categories = presetCategories (categoryCount);
    for (int i = 0; i < categoryCount; ++i)
        category.addItem (categories[i], i + 2);
    category.setSelectedId (1);
    category.onChange = [this] { rebuild(); };
    addAndMakeVisible (category);

    list.setRowHeight (30);
    list.setColour (juce::ListBox::backgroundColourId, theme::panel());
    addAndMakeVisible (list);

    nameEditor.setTextToShowWhenEmpty ("Preset name", theme::textDim());
    nameEditor.setFont (theme::valueFont (15.0f));
    addAndMakeVisible (nameEditor);

    categoryEditor.addItem ("User", 1);
    for (int i = 0; i < categoryCount; ++i)
        categoryEditor.addItem (categories[i], i + 2);
    categoryEditor.setSelectedId (1);
    addAndMakeVisible (categoryEditor);

    const auto setup = [this] (juce::TextButton& button, std::function<void()> action)
    {
        addAndMakeVisible (button);
        button.onClick = std::move (action);
    };

    setup (loadButton, [this]
    {
        if (selectedRow >= 0 && selectedRow < (int) filtered.size())
        {
            processor.presets.load (filtered[(size_t) selectedRow]);
            if (onPresetLoaded != nullptr)
                onPresetLoaded();
            updateStatus();
        }
    });

    setup (saveButton, [this]
    {
        processor.presets.saveAs (nameEditor.getText().trim(), categoryEditor.getText());
        processor.engine.params().requestSnap();
        rebuild();
        updateStatus();
    });

    setup (deleteButton, [this]
    {
        if (selectedRow >= 0 && selectedRow < (int) filtered.size())
        {
            processor.presets.remove (filtered[(size_t) selectedRow]);
            rebuild();
            updateStatus();
        }
    });

    setup (importButton, [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Import preset",
                                                      processor.presets.presetFolder(),
                                                      "*.mkpreset");
        chooser->launchAsync (juce::FileBrowserComponent::openMode, [this] (const juce::FileChooser& fc)
        {
            const auto file = fc.getResult();
            if (file.existsAsFile())
            {
                processor.presets.importFile (file);
                rebuild();
                updateStatus();
            }
        });
    });

    setup (exportButton, [this]
    {
        if (selectedRow < 0 || selectedRow >= (int) filtered.size())
            return;

        chooser = std::make_unique<juce::FileChooser> ("Export preset",
                                                      juce::File::getSpecialLocation (juce::File::userDesktopDirectory),
                                                      "*.mkpreset");
        const int index = filtered[(size_t) selectedRow];
        chooser->launchAsync (juce::FileBrowserComponent::saveMode, [this, index] (const juce::FileChooser& fc)
        {
            const auto file = fc.getResult();
            if (file != juce::File())
                processor.presets.exportFile (index, file.withFileExtension ("mkpreset"));
        });
    });

    setup (favouriteButton, [this]
    {
        if (selectedRow >= 0 && selectedRow < (int) filtered.size())
        {
            const auto& entry = processor.presets.entries()[(size_t) filtered[(size_t) selectedRow]];
            processor.presets.toggleFavorite (entry.name);
            rebuild();
        }
    });

    setup (learnButton, [this]
    {
        // CC learning is armed by right-clicking any knob; this button cancels
        // an armed learn or clears the mapping summary.
        processor.stopMidiLearn();
        updateStatus();
    });

    setup (closeButton, [this]
    {
        if (onClose != nullptr)
            onClose();
    });

    status.setFont (theme::valueFont (13.0f));
    status.setColour (juce::Label::textColourId, theme::textDim());
    addAndMakeVisible (status);

    learnStatus.setFont (theme::valueFont (12.0f));
    learnStatus.setColour (juce::Label::textColourId, theme::textDim());
    addAndMakeVisible (learnStatus);

    rebuild();
    updateStatus();
    startTimerHz (4);
}

PresetBrowser::~PresetBrowser() { stopTimer(); }

void PresetBrowser::paint (juce::Graphics& g)
{
    g.fillAll (theme::background().withAlpha (0.96f));

    auto bounds = getLocalBounds().reduced (18);

    g.setColour (theme::panel());
    g.fillRoundedRectangle (bounds.toFloat(), 14.0f);
    g.setColour (theme::outline());
    g.drawRoundedRectangle (bounds.toFloat(), 14.0f, 1.0f);

    g.setColour (theme::text());
    g.setFont (theme::labelFont (20.0f));
    g.drawFittedText ("PRESETS", bounds.removeFromTop (44).reduced (18, 0),
                      juce::Justification::centredLeft, 1);
}

void PresetBrowser::resized()
{
    auto bounds = getLocalBounds().reduced (18);
    bounds.removeFromTop (44);

    auto topRow = bounds.removeFromTop (34).reduced (16, 0);
    search.setBounds (topRow.removeFromLeft (topRow.getWidth() / 2 - 8));
    topRow.removeFromLeft (16);
    category.setBounds (topRow);

    bounds.removeFromTop (10);
    list.setBounds (bounds.removeFromTop (bounds.getHeight() - 128).reduced (16, 0));

    bounds.removeFromTop (8);
    auto saveRow = bounds.removeFromTop (34).reduced (16, 0);
    nameEditor.setBounds (saveRow.removeFromLeft (saveRow.getWidth() / 2));
    saveRow.removeFromLeft (8);
    categoryEditor.setBounds (saveRow.removeFromLeft (150));
    saveRow.removeFromLeft (8);
    saveButton.setBounds (saveRow.removeFromLeft (90));
    saveRow.removeFromLeft (6);
    deleteButton.setBounds (saveRow.removeFromLeft (90));

    bounds.removeFromTop (6);
    auto buttonRow = bounds.removeFromTop (30).reduced (16, 0);
    const int buttonWidth = (buttonRow.getWidth() - 5 * 6) / 6;
    for (auto* button : { &loadButton, &importButton, &exportButton, &favouriteButton,
                          &learnButton, &closeButton })
    {
        button->setBounds (buttonRow.removeFromLeft (buttonWidth));
        buttonRow.removeFromLeft (6);
    }

    bounds.removeFromTop (6);
    auto statusRow = bounds.reduced (16, 0);
    status.setBounds (statusRow.removeFromTop (18));
    learnStatus.setBounds (statusRow.removeFromTop (18));
}

void PresetBrowser::rebuild()
{
    filtered.clear();
    const auto& entries = processor.presets.entries();
    const auto query = search.getText();
    const auto categoryName = category.getSelectedId() <= 1 ? juce::String()
                                                            : category.getText();

    for (int i = 0; i < (int) entries.size(); ++i)
    {
        const auto& entry = entries[(size_t) i];
        if (categoryName.isNotEmpty() && entry.category != categoryName)
            continue;
        if (! presetMatches (entry.name.toRawUTF8(), entry.category.toRawUTF8(), query.toRawUTF8()))
            continue;
        filtered.push_back (i);
    }

    if (selectedRow >= (int) filtered.size())
        selectedRow = juce::jmax (0, (int) filtered.size() - 1);

    list.updateContent();
    list.selectRow (selectedRow);
    repaint();
}

int PresetBrowser::getNumRows()
{
    return (int) filtered.size();
}

void PresetBrowser::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected)
{
    if (row < 0 || row >= (int) filtered.size())
        return;

    const auto& entry = processor.presets.entries()[(size_t) filtered[(size_t) row]];

    if (selected)
    {
        g.setColour (theme::accentDeep().withAlpha (0.55f));
        g.fillRoundedRectangle (juce::Rectangle<float> (2.0f, 2.0f, (float) width - 4.0f,
                                                       (float) height - 4.0f), 6.0f);
    }

    const bool favourite = processor.presets.isFavorite (entry.name);
    if (favourite)
    {
        g.setColour (theme::accentBright());
        g.fillEllipse (10.0f, (float) height * 0.5f - 3.0f, 6.0f, 6.0f);
    }

    g.setColour (selected ? theme::text() : theme::text().withAlpha (0.86f));
    g.setFont (theme::valueFont (14.0f));
    g.drawFittedText (entry.name, 24, 0, width - 210, height, juce::Justification::centredLeft, 1);

    g.setColour (theme::textDim());
    g.setFont (theme::valueFont (12.0f));
    g.drawFittedText (entry.isFactory ? entry.category : entry.category + " (user)",
                      width - 190, 0, 180, height, juce::Justification::centredRight, 1);
}

void PresetBrowser::listBoxItemDoubleClicked (int row, const juce::MouseEvent&)
{
    selectedRow = row;
    loadButton.onClick();
}

void PresetBrowser::returnKeyPressed (int row)
{
    selectedRow = row;
    loadButton.onClick();
}

void PresetBrowser::updateStatus()
{
    if (selectedRow >= 0 && selectedRow < (int) filtered.size())
    {
        const auto& entry = processor.presets.entries()[(size_t) filtered[(size_t) selectedRow]];
        nameEditor.setText (entry.name, false);
        status.setText ("Loaded: " + processor.presets.currentName()
                            + "  |  " + juce::String ((int) filtered.size()) + " presets",
                        juce::dontSendNotification);
    }
    else
    {
        status.setText ("No preset selected", juce::dontSendNotification);
    }

    learnStatus.setText (processor.midiLearnDescription(), juce::dontSendNotification);
}

void PresetBrowser::saveCurrent (bool saveAs)
{
    juce::ignoreUnused (saveAs);
    processor.presets.saveAs (nameEditor.getText().trim(), categoryEditor.getText());
}

} // namespace mk
