#include "Plugin/PluginEditor.h"

namespace mk
{
namespace
{

const char* pageName (int index)
{
    static const char* const names[] = { "MAIN", "VOICE", "OSC", "FILTER", "WOBBLE",
                                         "MOD", "DIST", "FX", "MATRIX" };
    return names[juce::jlimit (0, 8, index)];
}

} // namespace

WobbleAudioProcessorEditor::WobbleAudioProcessorEditor (WobbleAudioProcessor& processorToUse)
    : AudioProcessorEditor (processorToUse),
      processor (processorToUse),
      meter (processorToUse.engine),
      keyboard (processorToUse.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setText ("MK / WOBBLE BASS", juce::dontSendNotification);
    titleLabel.setFont (theme::labelFont (20.0f));
    titleLabel.setColour (juce::Label::textColourId, theme::text());
    addAndMakeVisible (titleLabel);

    presetLabel.setFont (theme::valueFont (15.0f));
    presetLabel.setColour (juce::Label::textColourId, theme::accentBright());
    presetLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (presetLabel);

    infoLabel.setFont (theme::valueFont (11.5f));
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

    addAndMakeVisible (meter);

    for (int i = 0; i < (int) Page::kCount; ++i)
    {
        auto button = std::make_unique<juce::TextButton> (pageName (i));
        button->setClickingTogglesState (true);
        button->setRadioGroupId (4711);
        button->onClick = [this, i] { showPage ((Page) i); };
        addAndMakeVisible (*button);
        pageButtons[(size_t) i] = std::move (button);
    }

    const auto learn = [this] (int paramIndex) { processor.startMidiLearn (paramIndex); };

    pageComponents[(size_t) Page::Main]
        = std::make_unique<MainPage> (processor.apvts, processor.engine, learn);
    pageComponents[(size_t) Page::Voice]
        = std::make_unique<ParamGridPage> (processor.apvts, "Voice, Glide, Quality",
                                           [this]
                                           {
                                               std::vector<int> list;
                                               for (int i = 0; i < kParamCount; ++i)
                                                   if (paramMeta (i).group == Group::Glob
                                                       || paramMeta (i).group == Group::Misc)
                                                       list.push_back (i);
                                               return list;
                                           }(),
                                           learn);
    pageComponents[(size_t) Page::Osc]
        = std::make_unique<ParamGridPage> (processor.apvts, "Oscillators, Sub, FM",
                                           [this]
                                           {
                                               std::vector<int> list;
                                               for (int i = 0; i < kParamCount; ++i)
                                               {
                                                   const auto group = paramMeta (i).group;
                                                   if (group == Group::Osc1 || group == Group::Osc2
                                                       || group == Group::Osc3 || group == Group::Sub
                                                       || group == Group::Fm)
                                                       list.push_back (i);
                                               }
                                               return list;
                                           }(),
                                           learn);
    pageComponents[(size_t) Page::Filter]
        = std::make_unique<ParamGridPage> (processor.apvts, "Filter",
                                           [this]
                                           {
                                               std::vector<int> list;
                                               for (int i = 0; i < kParamCount; ++i)
                                                   if (paramMeta (i).group == Group::Filt)
                                                       list.push_back (i);
                                               return list;
                                           }(),
                                           learn);
    pageComponents[(size_t) Page::WobblePage]
        = std::make_unique<ParamGridPage> (processor.apvts, "Wobble Engine",
                                           [this]
                                           {
                                               std::vector<int> list;
                                               for (int i = 0; i < kParamCount; ++i)
                                                   if (paramMeta (i).group == Group::Wob)
                                                       list.push_back (i);
                                               return list;
                                           }(),
                                           learn);
    pageComponents[(size_t) Page::Mod]
        = std::make_unique<ParamGridPage> (processor.apvts, "LFOs and Envelopes",
                                           [this]
                                           {
                                               std::vector<int> list;
                                               for (int i = 0; i < kParamCount; ++i)
                                               {
                                                   const auto group = paramMeta (i).group;
                                                   if (group == Group::Lfo || group == Group::Env)
                                                       list.push_back (i);
                                               }
                                               return list;
                                           }(),
                                           learn);
    pageComponents[(size_t) Page::Dist]
        = std::make_unique<ParamGridPage> (processor.apvts, "Distortion",
                                           [this]
                                           {
                                               std::vector<int> list;
                                               for (int i = 0; i < kParamCount; ++i)
                                                   if (paramMeta (i).group == Group::Dist)
                                                       list.push_back (i);
                                               return list;
                                           }(),
                                           learn);
    pageComponents[(size_t) Page::Fx]
        = std::make_unique<ParamGridPage> (processor.apvts, "Effects",
                                           [this]
                                           {
                                               std::vector<int> list;
                                               for (int i = 0; i < kParamCount; ++i)
                                                   if (paramMeta (i).group == Group::Fx)
                                                       list.push_back (i);
                                               return list;
                                           }(),
                                           learn);
    pageComponents[(size_t) Page::Matrix]
        = std::make_unique<MatrixPage> (processor.apvts, learn);

    for (int i = 0; i < (int) Page::kCount; ++i)
    {
        auto* component = pageComponents[(size_t) i].get();

        if (i == (int) Page::Main)
        {
            addAndMakeVisible (*component);
            continue;
        }

        auto viewport = std::make_unique<juce::Viewport>();
        viewport->setViewedComponent (component, false);
        viewport->setScrollBarsShown (true, false);
        addAndMakeVisible (*viewport);
        pageViewports[(size_t) i] = std::move (viewport);
    }

    keyboard.setAvailableRange (0, 60);   // sub-bass to C4: the instrument's range
    // Keep focus enabled so the computer keyboard plays notes once the strip is
    // clicked - the fastest way to audition a patch inside the plug-in window.
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
    setResizeLimits (820, 520, 1800, 1100);
    setSize (1180, 760);

    showPage (Page::Main);
    updateHeader();
    startTimerHz (8);
}

WobbleAudioProcessorEditor::~WobbleAudioProcessorEditor()
{
    stopTimer();
    if (browser != nullptr)
        browser.reset();
    setLookAndFeel (nullptr);
}

void WobbleAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::background());

    juce::ColourGradient glow (theme::accentDeep().withAlpha (0.18f), 0.0f, 0.0f,
                               theme::background(), 0.0f, (float) getHeight() * 0.8f, false);
    g.setGradientFill (glow);
    g.fillAll();

    auto header = getLocalBounds().removeFromTop (52).toFloat();
    g.setColour (theme::panel().withAlpha (0.85f));
    g.fillRect (header);

    g.setColour (theme::outline());
    g.drawHorizontalLine (52, 0.0f, (float) getWidth());
    g.drawHorizontalLine (52 + 38, 0.0f, (float) getWidth());

    // Recessed panel behind the playable keyboard strip.
    auto keys = getLocalBounds().removeFromBottom (keyboardHeight()).toFloat().reduced (6.0f, 2.0f);
    g.setColour (juce::Colour (0xff0d0a14));
    g.fillRoundedRectangle (keys, 8.0f);
    g.setColour (theme::outline());
    g.drawRoundedRectangle (keys, 8.0f, 1.0f);
}

void WobbleAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    auto header = bounds.removeFromTop (52).reduced (14, 8);
    titleLabel.setBounds (header.removeFromLeft (250));
    auto transport = header.removeFromLeft (260);
    previousButton.setBounds (transport.removeFromLeft (36).reduced (0, 2));
    transport.removeFromLeft (4);
    presetLabel.setBounds (transport.removeFromLeft (176));
    transport.removeFromLeft (4);
    nextButton.setBounds (transport.removeFromLeft (36).reduced (0, 2));

    meter.setBounds (header.removeFromRight (110).reduced (0, 10));
    header.removeFromRight (8);
    infoLabel.setBounds (header.removeFromRight (150));
    header.removeFromRight (8);
    browserButton.setBounds (header.removeFromRight (110).reduced (0, 2));

    auto tabs = bounds.removeFromTop (38).reduced (12, 4);
    const int tabWidth = tabs.getWidth() / (int) Page::kCount;
    for (int i = 0; i < (int) Page::kCount; ++i)
        pageButtons[(size_t) i]->setBounds (tabs.removeFromLeft (tabWidth).reduced (2, 0));

    auto keyboardArea = bounds.removeFromBottom (keyboardHeight()).reduced (10, 5);
    // 36 white keys across the strip so it fills the width at any window size.
    keyboard.setKeyWidth (juce::jmax (12.0f, (float) keyboardArea.getWidth() / 36.0f));
    keyboard.setBounds (keyboardArea);

    layoutPage();
}

int WobbleAudioProcessorEditor::keyboardHeight() const
{
    return juce::jlimit (38, 58, getHeight() / 13);
}

void WobbleAudioProcessorEditor::layoutPage()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (52 + 38);
    bounds.removeFromBottom (keyboardHeight());
    bounds.reduce (10, 6);

    for (int i = 0; i < (int) Page::kCount; ++i)
    {
        auto* viewport = pageViewports[(size_t) i].get();
        auto* page = pageComponents[(size_t) i].get();

        if (viewport == nullptr || page == nullptr)
        {
            if (page != nullptr && i == (int) Page::Main)
                page->setBounds (bounds);
            continue;
        }

        viewport->setBounds (bounds);
        const int visibleWidth = juce::jmax (200, bounds.getWidth() - 14);
        int contentHeight = bounds.getHeight();

        if (auto* grid = dynamic_cast<ParamGridPage*> (page))
        {
            const int columns = juce::jlimit (2, 10, visibleWidth / 118);
            grid->setColumns (columns);
            const int rows = (grid->itemCount() + columns - 1) / columns;
            contentHeight = juce::jmax (bounds.getHeight(), rows * 112);
        }
        else
        {
            contentHeight = juce::jmax (bounds.getHeight(), kMatrixSlots * 48 + 24);
        }

        page->setBounds (0, 0, visibleWidth, contentHeight);
    }
}

void WobbleAudioProcessorEditor::showPage (Page page)
{
    currentPage = page;

    for (int i = 0; i < (int) Page::kCount; ++i)
    {
        const bool visible = (i == (int) page);
        pageButtons[(size_t) i]->setToggleState (visible, juce::dontSendNotification);

        if (auto* viewport = pageViewports[(size_t) i].get())
        {
            viewport->setVisible (visible);
        }
        else if (auto* component = pageComponents[(size_t) i].get())
        {
            component->setVisible (visible);
        }
    }

    layoutPage();
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

void WobbleAudioProcessorEditor::updateHeader()
{
    presetLabel.setText (processor.presets.currentName(), juce::dontSendNotification);
    resized();
}

void WobbleAudioProcessorEditor::timerCallback()
{
    const float blockMs = processor.cpuLoad();
    const int voices = processor.engine.activeVoiceCount();
    const int limit = processor.engine.effectiveVoiceLimit();
    infoLabel.setText (juce::String (voices) + "/" + juce::String (limit) + " voices  |  "
                           + juce::String (blockMs, 2) + " ms",
                       juce::dontSendNotification);

    if (browser == nullptr)
        return;

    browser->setBounds (getLocalBounds());
}

} // namespace mk
