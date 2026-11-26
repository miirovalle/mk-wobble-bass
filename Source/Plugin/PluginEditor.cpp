#include "Plugin/PluginEditor.h"

namespace mk
{

WobbleAudioProcessorEditor::WobbleAudioProcessorEditor (WobbleAudioProcessor& processorToUse)
    : AudioProcessorEditor (processorToUse),
      processor (processorToUse),
      scope (processorToUse.engine),
      meter (processorToUse.engine),
      easyPage (processorToUse.apvts, processorToUse.engine),
      advancedPage (processorToUse.apvts, processorToUse.engine),
      keyboard (processorToUse.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setText ("WOBBLE MACHINE", juce::dontSendNotification);
    titleLabel.setFont (theme::labelFont (21.0f));
    titleLabel.setColour (juce::Label::textColourId, theme::text());
    addAndMakeVisible (titleLabel);

    presetLabel.setFont (theme::valueFont (13.0f));
    presetLabel.setColour (juce::Label::textColourId, theme::accentBright());
    presetLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (presetLabel);

    randomButton.setColour (juce::TextButton::textColourOffId, theme::text());
    randomButton.onClick = [this]
    {
        processor.randomize();
        presetLabel.setText (processor.presets.currentName(), juce::dontSendNotification);
    };
    addAndMakeVisible (randomButton);

    browserButton.setColour (juce::TextButton::textColourOffId, theme::text());
    browserButton.onClick = [this] { openBrowser(); };
    addAndMakeVisible (browserButton);

    for (int i = 0; i < 2; ++i)
    {
        auto& tab = i == 0 ? easyTab : advancedTab;
        tab.setClickingTogglesState (true);
        tab.setRadioGroupId (9001);
        tab.setColour (juce::TextButton::buttonOnColourId, theme::accentDeep());
        tab.onClick = [this, i] { showTab (i); };
        addAndMakeVisible (tab);
    }

    addAndMakeVisible (meter);
    addAndMakeVisible (scope);
    addChildComponent (easyPage);
    addChildComponent (advancedPage);

    keyboard.setAvailableRange (0, 60);   // sub-bass to C4: the instrument's range
    keyboard.setWantsKeyboardFocus (true);
    keyboard.setMouseClickGrabsKeyboardFocus (true);
    keyboard.setScrollButtonsVisible (false);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffddd5cb));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff141210));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, theme::outline());
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
                        theme::accent().withAlpha (0.35f));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId,
                        theme::accent().withAlpha (0.75f));
    keyboard.setColour (juce::MidiKeyboardComponent::textLabelColourId, theme::textDim());
    addAndMakeVisible (keyboard);

    setResizable (true, true);
    setResizeLimits (900, 600, 1680, 1040);
    setSize (1120, 730);

    presetLabel.setText (processor.presets.currentName(), juce::dontSendNotification);

    showTab (0);
    startTimerHz (6);
}

WobbleAudioProcessorEditor::~WobbleAudioProcessorEditor()
{
    stopTimer();
    browser.reset();
    setLookAndFeel (nullptr);
}

int WobbleAudioProcessorEditor::keyboardHeight() const
{
    return juce::jlimit (38, 58, getHeight() / 16);
}

int WobbleAudioProcessorEditor::headerHeight() const
{
    return 54;
}

void WobbleAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::background());

    juce::ColourGradient glow (theme::accentDeep().withAlpha (0.28f), 0.0f, 0.0f,
                               theme::background(), 0.0f, (float) getHeight() * 0.7f, false);
    g.setGradientFill (glow);
    g.fillAll();

    auto header = getLocalBounds().removeFromTop (headerHeight()).toFloat();
    g.setColour (theme::panel().withAlpha (0.92f));
    g.fillRect (header);

    g.setColour (theme::textDim().withAlpha (0.85f));
    g.setFont (theme::labelFont (9.0f));
    g.drawFittedText ("DUBSTEP BASS SYNTHESIZER", juce::Rectangle<int> (16, 36, 280, 13),
                      juce::Justification::centredLeft, 1);

    g.setColour (theme::accent().withAlpha (0.7f));
    g.drawHorizontalLine (headerHeight() - 1, 0.0f, (float) getWidth());

    // Gradient rule under the title: the one piece of glow that also reads on
    // stage without stealing attention from the knobs.
    juce::ColourGradient titleGlow (theme::accent().withAlpha (0.9f), 16.0f, (float) headerHeight() - 2,
                                    theme::accent().withAlpha (0.0f), 300.0f, (float) headerHeight() - 2,
                                    false);
    g.setGradientFill (titleGlow);
    g.fillRect (16.0f, (float) headerHeight() - 2, 284.0f, 2.0f);

    auto keys = getLocalBounds().removeFromBottom (keyboardHeight()).toFloat().reduced (6.0f, 2.0f);
    g.setColour (juce::Colour (0xff0d0c0b));
    g.fillRoundedRectangle (keys, 8.0f);
    g.setColour (theme::outline());
    g.drawRoundedRectangle (keys, 8.0f, 1.0f);
}

void WobbleAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    auto header = bounds.removeFromTop (headerHeight()).reduced (14, 8);
    auto titleArea = header.removeFromLeft (280);
    titleLabel.setBounds (titleArea.removeFromTop (26));
    meter.setBounds (header.removeFromLeft (90).reduced (2, 7));

    browserButton.setBounds (header.removeFromRight (92));
    header.removeFromRight (6);
    randomButton.setBounds (header.removeFromRight (96));
    header.removeFromRight (10);
    presetLabel.setBounds (header);

    auto keyboardArea = bounds.removeFromBottom (keyboardHeight()).reduced (10, 5);
    keyboard.setKeyWidth (juce::jmax (12.0f, (float) keyboardArea.getWidth() / 36.0f));
    keyboard.setBounds (keyboardArea);

    auto content = bounds.reduced (10, 6);

    // The output waveform is always visible.
    const int scopeHeight = juce::jlimit (48, 66, content.getHeight() / 10);
    scope.setBounds (content.removeFromTop (scopeHeight));
    content.removeFromTop (6);

    auto tabs = content.removeFromTop (28);
    easyTab.setBounds (tabs.removeFromLeft (120).reduced (0, 1));
    tabs.removeFromLeft (6);
    advancedTab.setBounds (tabs.removeFromLeft (150).reduced (0, 1));
    content.removeFromTop (8);

    easyPage.setBounds (content);
    advancedPage.setBounds (content);
}

void WobbleAudioProcessorEditor::showTab (int index)
{
    currentTab = juce::jlimit (0, 1, index);

    easyTab.setToggleState (currentTab == 0, juce::dontSendNotification);
    advancedTab.setToggleState (currentTab == 1, juce::dontSendNotification);

    easyPage.setVisible (currentTab == 0);
    advancedPage.setVisible (currentTab == 1);
}

void WobbleAudioProcessorEditor::openBrowser()
{
    if (browser != nullptr)
        return;

    browser = std::make_unique<PresetBrowser> (processor);
    browser->onClose = [this] { closeBrowser(); };
    browser->onPresetLoaded = [this]
    {
        presetLabel.setText (processor.presets.currentName(), juce::dontSendNotification);
    };

    addAndMakeVisible (*browser);
    browser->setBounds (getLocalBounds());
    browser->grabKeyboardFocus();
}

void WobbleAudioProcessorEditor::closeBrowser()
{
    if (browser == nullptr)
        return;

    browser.reset();
    presetLabel.setText (processor.presets.currentName(), juce::dontSendNotification);
}

void WobbleAudioProcessorEditor::timerCallback()
{
    presetLabel.setText (processor.presets.currentName(), juce::dontSendNotification);
    advancedPage.refresh();

    if (browser != nullptr)
        browser->setBounds (getLocalBounds());
}

} // namespace mk
