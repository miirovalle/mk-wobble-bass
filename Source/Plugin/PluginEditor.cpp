#include "Plugin/PluginEditor.h"

namespace mk
{

WobbleAudioProcessorEditor::WobbleAudioProcessorEditor (WobbleAudioProcessor& processorToUse)
    : AudioProcessorEditor (processorToUse),
      processor (processorToUse),
      meter (processorToUse.engine),
      scope (processorToUse.engine),
      wobblePage (processorToUse.apvts, processorToUse.engine),
      gatePage (processorToUse.apvts, processorToUse.engine),
      keyboard (processorToUse.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setText ("MK / WOBBLE BASS", juce::dontSendNotification);
    titleLabel.setFont (theme::labelFont (19.0f));
    titleLabel.setColour (juce::Label::textColourId, theme::text());
    addAndMakeVisible (titleLabel);

    presetLabel.setFont (theme::valueFont (14.0f));
    presetLabel.setColour (juce::Label::textColourId, theme::accentBright());
    presetLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (presetLabel);

    infoLabel.setFont (theme::valueFont (10.5f));
    infoLabel.setColour (juce::Label::textColourId, theme::textDim());
    infoLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (infoLabel);

    for (auto* button : { &previousButton, &nextButton, &browserButton })
    {
        addAndMakeVisible (*button);
        button->setColour (juce::TextButton::textColourOffId, theme::text());
    }

    previousButton.onClick = [this]
    {
        processor.presets.loadNext (-1);
        updateHeader();
    };

    nextButton.onClick = [this]
    {
        processor.presets.loadNext (1);
        updateHeader();
    };

    browserButton.onClick = [this] { openBrowser(); };

    for (int i = 0; i < 2; ++i)
    {
        auto& tab = i == 0 ? wobbleTab : gateTab;
        tab.setClickingTogglesState (true);
        tab.setRadioGroupId (9001);
        tab.setColour (juce::TextButton::buttonOnColourId, theme::accentDeep());
        tab.onClick = [this, i] { showTab (i); };
        addAndMakeVisible (tab);
    }

    addAndMakeVisible (meter);
    addAndMakeVisible (scope);
    addChildComponent (wobblePage);
    addChildComponent (gatePage);

    keyboard.setAvailableRange (0, 60);   // sub-bass to C4: the instrument's range
    keyboard.setWantsKeyboardFocus (true);
    keyboard.setMouseClickGrabsKeyboardFocus (true);
    keyboard.setScrollButtonsVisible (false);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffd9d2ea));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff14101d));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, theme::outline());
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
                        theme::accent().withAlpha (0.35f));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId,
                        theme::accent().withAlpha (0.75f));
    keyboard.setColour (juce::MidiKeyboardComponent::textLabelColourId, theme::textDim());
    addAndMakeVisible (keyboard);

    setResizable (true, true);
    setResizeLimits (760, 520, 1600, 1000);
    setSize (980, 640);

    updateHeader();
    showTab (0);
    startTimerHz (5);
}

WobbleAudioProcessorEditor::~WobbleAudioProcessorEditor()
{
    stopTimer();
    browser.reset();
    setLookAndFeel (nullptr);
}

int WobbleAudioProcessorEditor::keyboardHeight() const
{
    return juce::jlimit (38, 56, getHeight() / 14);
}

void WobbleAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::background());

    juce::ColourGradient glow (theme::accentDeep().withAlpha (0.16f), 0.0f, 0.0f,
                               theme::background(), 0.0f, (float) getHeight() * 0.8f, false);
    g.setGradientFill (glow);
    g.fillAll();

    auto header = getLocalBounds().removeFromTop (54).toFloat();
    g.setColour (theme::panel().withAlpha (0.9f));
    g.fillRect (header);

    g.setColour (theme::outline());
    g.drawHorizontalLine (54, 0.0f, (float) getWidth());

    auto keys = getLocalBounds().removeFromBottom (keyboardHeight()).toFloat().reduced (6.0f, 2.0f);
    g.setColour (juce::Colour (0xff0d0a14));
    g.fillRoundedRectangle (keys, 8.0f);
    g.setColour (theme::outline());
    g.drawRoundedRectangle (keys, 8.0f, 1.0f);
}

void WobbleAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    auto header = bounds.removeFromTop (54).reduced (12, 9);
    titleLabel.setBounds (header.removeFromLeft (210));

    auto transport = header.removeFromLeft (276);
    previousButton.setBounds (transport.removeFromLeft (32).reduced (0, 2));
    transport.removeFromLeft (4);
    presetLabel.setBounds (transport.removeFromLeft (196));
    transport.removeFromLeft (4);
    nextButton.setBounds (transport.removeFromLeft (32).reduced (0, 2));

    meter.setBounds (header.removeFromRight (84).reduced (0, 8));
    header.removeFromRight (8);
    infoLabel.setBounds (header.removeFromRight (132));
    header.removeFromRight (8);
    browserButton.setBounds (header.removeFromRight (92).reduced (0, 2));

    auto keyboardArea = bounds.removeFromBottom (keyboardHeight()).reduced (10, 5);
    keyboard.setKeyWidth (juce::jmax (12.0f, (float) keyboardArea.getWidth() / 36.0f));
    keyboard.setBounds (keyboardArea);

    auto content = bounds.reduced (8, 6);

    // The waveform is always visible, on every tab.
    const int scopeHeight = juce::jlimit (56, 78, content.getHeight() / 8);
    scope.setBounds (content.removeFromTop (scopeHeight));
    content.removeFromTop (6);

    auto tabs = content.removeFromTop (30);
    wobbleTab.setBounds (tabs.removeFromLeft (110).reduced (0, 1));
    tabs.removeFromLeft (6);
    gateTab.setBounds (tabs.removeFromLeft (110).reduced (0, 1));
    content.removeFromTop (6);

    wobblePage.setBounds (content);
    gatePage.setBounds (content);
}

void WobbleAudioProcessorEditor::showTab (int index)
{
    currentTab = juce::jlimit (0, 1, index);

    wobbleTab.setToggleState (currentTab == 0, juce::dontSendNotification);
    gateTab.setToggleState (currentTab == 1, juce::dontSendNotification);

    wobblePage.setVisible (currentTab == 0);
    gatePage.setVisible (currentTab == 1);
}

void WobbleAudioProcessorEditor::updateHeader()
{
    presetLabel.setText (processor.presets.currentName(), juce::dontSendNotification);
}

void WobbleAudioProcessorEditor::openBrowser()
{
    if (browser != nullptr)
        return;

    browser = std::make_unique<PresetBrowser> (processor);
    browser->onClose = [this] { closeBrowser(); };
    browser->onPresetLoaded = [this] { updateHeader(); };
    addAndMakeVisible (*browser);
    browser->setBounds (getLocalBounds());
    browser->grabKeyboardFocus();
}

void WobbleAudioProcessorEditor::closeBrowser()
{
    if (browser == nullptr)
        return;

    browser.reset();
    updateHeader();
}

void WobbleAudioProcessorEditor::timerCallback()
{
    const int voices = processor.engine.activeVoiceCount();
    const float blockMs = processor.cpuLoad();

    infoLabel.setText (juce::String (voices) + " voices  |  " + juce::String (blockMs, 2) + " ms",
                       juce::dontSendNotification);

    wobblePage.refresh();
    gatePage.refresh();

    if (browser != nullptr)
        browser->setBounds (getLocalBounds());
}

} // namespace mk
