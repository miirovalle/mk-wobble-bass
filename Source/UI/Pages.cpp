#include "UI/Pages.h"

namespace mk
{
namespace
{

juce::RangedAudioParameter* rangedParam (juce::AudioProcessorValueTreeState& state, int index)
{
    return dynamic_cast<juce::RangedAudioParameter*> (state.getParameter (paramId (index)));
}

void writePlain (juce::AudioProcessorValueTreeState& state, int index, float plain)
{
    if (auto* parameter = rangedParam (state, index))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (plain));
}

float readPlain (juce::AudioProcessorValueTreeState& state, int index)
{
    auto* parameter = rangedParam (state, index);
    return parameter != nullptr ? parameter->convertFrom0to1 (parameter->getValue()) : 0.0f;
}

void styleChip (juce::TextButton& button, juce::Colour textColour)
{
    button.setColour (juce::TextButton::buttonColourId, theme::panelLight());
    button.setColour (juce::TextButton::buttonOnColourId, theme::accentDeep());
    button.setColour (juce::TextButton::textColourOffId, textColour);
    button.setColour (juce::TextButton::textColourOnId, theme::text());
}

} // namespace

//==============================================================================
WobblePage::WobblePage (juce::AudioProcessorValueTreeState& state, Engine& engine)
    : canvas (state, engine),
      rate (state, (int) P::wobRate, "RATE"),
      depth (state, (int) P::wobDepth, "DEPTH"),
      type (state),
      apvts (state)
{
    addAndMakeVisible (canvas);
    addAndMakeVisible (rate);
    addAndMakeVisible (depth);
    addAndMakeVisible (type);

    for (auto* button : { &syncButton, &sineButton, &triButton, &sawButton, &squareButton, &clearButton })
    {
        styleChip (*button, theme::text());
        addAndMakeVisible (*button);
        button->setColour (juce::TextButton::textColourOffId, theme::text());
    }

    syncButton.setClickingTogglesState (true);
    syncButton.onClick = [this]
    {
        // wobMode: 0 = host sync, 1 = free running.
        writePlain (apvts, (int) P::wobMode, syncButton.getToggleState() ? 0.0f : 1.0f);
    };

    sineButton.onClick = [this] { canvas.loadShape (ShapeSine); };
    triButton.onClick = [this] { canvas.loadShape (ShapeTriangle); };
    sawButton.onClick = [this] { canvas.loadShape (ShapeSawUp); };
    squareButton.onClick = [this] { canvas.loadShape (ShapeSquare); };
    clearButton.onClick = [this] { canvas.loadShape (-1); };

    refresh();
}

void WobblePage::refresh()
{
    const bool synced = readPlain (apvts, (int) P::wobMode) < 0.5f;
    syncButton.setToggleState (synced, juce::dontSendNotification);
}

void WobblePage::paint (juce::Graphics& g)
{
    juce::ignoreUnused (g);
}

void WobblePage::resized()
{
    auto area = getLocalBounds().reduced (2);

    auto knobRow = area.removeFromBottom (juce::jlimit (120, 180, area.getHeight() * 36 / 100));
    area.removeFromBottom (8);

    auto right = area.removeFromRight (juce::jmax (190, area.getWidth() * 32 / 100));
    area.removeFromRight (8);
    canvas.setBounds (area);

    const int chipHeight = juce::jlimit (26, 38, right.getHeight() / 6);

    auto row1 = right.removeFromTop (chipHeight);
    const int quarter = row1.getWidth() / 4;
    sineButton.setBounds (row1.removeFromLeft (quarter).reduced (2, 0));
    triButton.setBounds (row1.removeFromLeft (quarter).reduced (2, 0));
    sawButton.setBounds (row1.removeFromLeft (quarter).reduced (2, 0));
    squareButton.setBounds (row1.reduced (2, 0));

    right.removeFromTop (6);

    auto row2 = right.removeFromTop (chipHeight);
    const int half = row2.getWidth() / 2;
    syncButton.setBounds (row2.removeFromLeft (half).reduced (2, 0));
    clearButton.setBounds (row2.reduced (2, 0));

    const int knobWidth = knobRow.getWidth() / 3;
    rate.setBounds (knobRow.removeFromLeft (knobWidth).reduced (6, 0));
    depth.setBounds (knobRow.removeFromLeft (knobWidth).reduced (6, 0));
    type.setBounds (knobRow.reduced (6, 0));
}

//==============================================================================
GatePage::GatePage (juce::AudioProcessorValueTreeState& state, Engine& engine)
    : pattern (engine),
      rate (state, (int) P::gateDiv, "RATE"),
      depth (state, (int) P::gateDepth, "DEPTH"),
      smooth (state, (int) P::gateSmooth, "SMOOTH"),
      apvts (state)
{
    addAndMakeVisible (pattern);
    addAndMakeVisible (rate);
    addAndMakeVisible (depth);
    addAndMakeVisible (smooth);

    styleChip (gateButton, theme::text());
    gateButton.setClickingTogglesState (true);
    gateButton.onClick = [this]
    {
        writePlain (apvts, (int) P::gateOn, gateButton.getToggleState() ? 1.0f : 0.0f);
    };
    addAndMakeVisible (gateButton);

    refresh();
}

void GatePage::refresh()
{
    const bool on = readPlain (apvts, (int) P::gateOn) > 0.5f;
    gateButton.setToggleState (on, juce::dontSendNotification);
    gateButton.setButtonText (on ? "GATE ON" : "GATE OFF");
}

void GatePage::paint (juce::Graphics& g)
{
    juce::ignoreUnused (g);
}

void GatePage::resized()
{
    auto area = getLocalBounds().reduced (2);

    auto knobRow = area.removeFromBottom (juce::jlimit (120, 180, area.getHeight() * 36 / 100));
    area.removeFromBottom (8);

    pattern.setBounds (area);

    const int cellWidth = knobRow.getWidth() / 4;
    rate.setBounds (knobRow.removeFromLeft (cellWidth).reduced (6, 0));
    depth.setBounds (knobRow.removeFromLeft (cellWidth).reduced (6, 0));
    smooth.setBounds (knobRow.removeFromLeft (cellWidth).reduced (6, 0));

    auto buttonCell = knobRow.reduced (10, 0);
    gateButton.setBounds (buttonCell.withSizeKeepingCentre (buttonCell.getWidth(),
                                                            juce::jmin (56, buttonCell.getHeight())));
}

} // namespace mk
