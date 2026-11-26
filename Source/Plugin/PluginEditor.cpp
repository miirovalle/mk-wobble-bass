#include "Plugin/PluginEditor.h"

namespace mk
{

WobbleAudioProcessorEditor::WobbleAudioProcessorEditor (WobbleAudioProcessor& processorToUse)
    : AudioProcessorEditor (processorToUse),
      processor (processorToUse),
      meter (processorToUse.engine),
      scope (processorToUse.engine),
      pianoRoll (processorToUse.sequencer),
      wobblePanel (processorToUse.apvts, processorToUse.engine),
      wobbleTargets (processorToUse.apvts),
      filterPad (processorToUse.apvts),
      growlPad (processorToUse.apvts),
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

    for (auto* button : { &previousButton, &nextButton, &browserButton, &advancedButton })
    {
        addAndMakeVisible (*button);
        button->setColour (juce::TextButton::textColourOffId, theme::text());
        button->setColour (juce::TextButton::textColourOnId, theme::text());
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

    advancedButton.setClickingTogglesState (true);
    advancedButton.onClick = [this] { showAdvanced (! advancedVisible); };

    addAndMakeVisible (meter);
    addAndMakeVisible (scope);
    addAndMakeVisible (pianoRoll);
    addAndMakeVisible (wobblePanel);
    addAndMakeVisible (wobbleTargets);
    addAndMakeVisible (filterPad);
    addAndMakeVisible (growlPad);

    for (int i = 0; i < 3; ++i)
    {
        lfoSlots[(size_t) i] = std::make_unique<LfoSlot> (processor.apvts, i);
        addAndMakeVisible (*lfoSlots[(size_t) i]);
    }

    advancedPage = std::make_unique<AdvancedPage> (processor.apvts);
    advancedViewport.setViewedComponent (advancedPage.get(), false);
    advancedViewport.setScrollBarsShown (true, false);
    addChildComponent (advancedViewport);

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
    setResizeLimits (820, 560, 1800, 1100);
    setSize (1180, 760);

    updateHeader();
    startTimerHz (4);
}

WobbleAudioProcessorEditor::~WobbleAudioProcessorEditor()
{
    stopTimer();
    browser.reset();
    advancedViewport.setViewedComponent (nullptr, false);
    advancedPage.reset();
    setLookAndFeel (nullptr);
}

int WobbleAudioProcessorEditor::keyboardHeight() const
{
    return juce::jlimit (38, 58, getHeight() / 14);
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
    header.removeFromLeft (10);

    meter.setBounds (header.removeFromRight (84).reduced (0, 8));
    header.removeFromRight (8);
    infoLabel.setBounds (header.removeFromRight (132));
    header.removeFromRight (8);
    advancedButton.setBounds (header.removeFromRight (102).reduced (0, 2));
    header.removeFromRight (6);
    browserButton.setBounds (header.removeFromRight (92).reduced (0, 2));

    auto keyboardArea = bounds.removeFromBottom (keyboardHeight()).reduced (10, 5);
    keyboard.setKeyWidth (juce::jmax (12.0f, (float) keyboardArea.getWidth() / 36.0f));
    keyboard.setBounds (keyboardArea);

    auto content = bounds.reduced (8, 6);

    // The waveform is always on screen, including behind the ADVANCED drawer.
    const int scopeHeight = juce::jlimit (58, 86, content.getHeight() / 8);
    scope.setBounds (content.removeFromTop (scopeHeight));
    content.removeFromTop (8);

    if (advancedVisible)
    {
        advancedViewport.setBounds (content);

        const int pageWidth = juce::jmax (240, content.getWidth() - 16);
        advancedPage->setSize (pageWidth, advancedPage->preferredHeight (pageWidth));
        return;
    }

    auto left = content.removeFromLeft (juce::jmax (320, content.getWidth() * 54 / 100));
    content.removeFromLeft (8);
    auto right = content;

    pianoRoll.setBounds (left);

    // Right: wobble curve, wobble targets, LFO slots, then the two pads.
    const int wobbleHeight = juce::jlimit (104, 150, right.getHeight() * 24 / 100);
    wobblePanel.setBounds (right.removeFromTop (wobbleHeight));
    right.removeFromTop (6);

    const int targetHeight = juce::jlimit (74, 100, right.getHeight() / 6);
    wobbleTargets.setBounds (right.removeFromTop (targetHeight));
    right.removeFromTop (6);

    const int lfoHeight = juce::jlimit (84, 118, right.getHeight() / 3);
    auto lfoRow = right.removeFromTop (lfoHeight);
    const int slotWidth = lfoRow.getWidth() / 3;

    for (int i = 0; i < 3; ++i)
        lfoSlots[(size_t) i]->setBounds (lfoRow.removeFromLeft (slotWidth).reduced (i == 0 ? 0 : 3, 0));

    right.removeFromTop (6);

    if (right.getHeight() > 60)
    {
        auto padRow = right;
        const int padWidth = padRow.getWidth() / 2;
        filterPad.setBounds (padRow.removeFromLeft (padWidth).reduced (0, 0));
        growlPad.setBounds (padRow.reduced (6, 0));
    }
}

void WobbleAudioProcessorEditor::updateHeader()
{
    presetLabel.setText (processor.presets.currentName(), juce::dontSendNotification);
}

void WobbleAudioProcessorEditor::showAdvanced (bool shouldShow)
{
    advancedVisible = shouldShow;
    advancedButton.setToggleState (shouldShow, juce::dontSendNotification);

    pianoRoll.setVisible (! shouldShow);
    wobblePanel.setVisible (! shouldShow);
    wobbleTargets.setVisible (! shouldShow);
    filterPad.setVisible (! shouldShow);
    growlPad.setVisible (! shouldShow);

    for (auto& slot : lfoSlots)
        slot->setVisible (! shouldShow);

    advancedViewport.setVisible (shouldShow);

    resized();
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

    if (browser != nullptr)
        browser->setBounds (getLocalBounds());
}

} // namespace mk
