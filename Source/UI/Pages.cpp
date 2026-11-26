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
      rate (state),
      depth (state, (int) P::wobDepth, "DEPTH"),
      type (state),
      apvts (state)
{
    addAndMakeVisible (canvas);
    addAndMakeVisible (rate);
    addAndMakeVisible (depth);
    addAndMakeVisible (type);

    for (auto* button : { &sineButton, &clearButton })
    {
        styleChip (*button, theme::text());
        addAndMakeVisible (*button);
    }

    sineButton.onClick = [this] { canvas.loadShape (ShapeSine); };
    clearButton.onClick = [this] { canvas.loadShape (-1); };
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

    auto chipRow = area.removeFromBottom (28);
    area.removeFromBottom (6);

    canvas.setBounds (area);

    sineButton.setBounds (chipRow.removeFromLeft (86).reduced (0, 1));
    chipRow.removeFromLeft (6);
    clearButton.setBounds (chipRow.removeFromLeft (86).reduced (0, 1));

    const int knobWidth = knobRow.getWidth() / 3;
    rate.setBounds (knobRow.removeFromLeft (knobWidth).reduced (6, 0));
    depth.setBounds (knobRow.removeFromLeft (knobWidth).reduced (6, 0));
    type.setBounds (knobRow.reduced (6, 0));
}

//==============================================================================
GatePage::GatePage (juce::AudioProcessorValueTreeState& state, Engine& engineToUse)
    : pattern (engineToUse),
      rate (state, (int) P::gateDiv, "RATE"),
      depth (state, (int) P::gateDepth, "DEPTH"),
      smooth (state, (int) P::gateSmooth, "SMOOTH"),
      apvts (state),
      engine (engineToUse)
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

    for (auto* button : { &presetPrev, &presetNext })
    {
        styleChip (*button, theme::text());
        addAndMakeVisible (*button);
    }

    presetPrev.onClick = [this] { applyPreset (presetIndex - 1); };
    presetNext.onClick = [this] { applyPreset (presetIndex + 1); };

    presetName.setFont (theme::valueFont (13.0f));
    presetName.setColour (juce::Label::textColourId, theme::accentBright());
    presetName.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (presetName);

    refresh();
}

void GatePage::applyPreset (int index)
{
    const int count = gatePresetCount();
    presetIndex = ((index % count) + count) % count;

    const auto& preset = gatePreset (presetIndex);

    // The library is written as 16-step rhythms; repeat them over the full grid.
    uint64_t mask = 0;
    const uint16_t base = gatePresetMask (presetIndex);

    for (int offset = 0; offset < TranceGate::kSteps; offset += 16)
        mask |= (uint64_t) base << offset;

    engine.tranceGate().setPattern (mask);
    writePlain (apvts, (int) P::gateDiv, (float) preset.division);
    writePlain (apvts, (int) P::gateDepth, preset.depth);
    writePlain (apvts, (int) P::gateSmooth, preset.smooth);

    // Stored on the plug-in state tree so the label survives a reload.
    apvts.state.setProperty ("gatePreset", presetIndex, nullptr);

    refresh();
    pattern.repaint();
}

void GatePage::refresh()
{
    const bool on = readPlain (apvts, (int) P::gateOn) > 0.5f;
    gateButton.setToggleState (on, juce::dontSendNotification);
    gateButton.setButtonText (on ? "GATE ON" : "GATE OFF");

    presetIndex = juce::jlimit (0, gatePresetCount() - 1,
                                (int) apvts.state.getProperty ("gatePreset", 0));

    const auto& preset = gatePreset (presetIndex);
    presetName.setText (juce::String (presetIndex + 1) + "/" + juce::String (gatePresetCount())
                            + "  " + preset.name,
                        juce::dontSendNotification);
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

    auto presetBar = area.removeFromBottom (26);
    area.removeFromBottom (6);

    pattern.setBounds (area);

    presetPrev.setBounds (presetBar.removeFromLeft (34).reduced (0, 1));
    presetNext.setBounds (presetBar.removeFromRight (34).reduced (0, 1));
    presetName.setBounds (presetBar.reduced (6, 0));

    const int cellWidth = knobRow.getWidth() / 4;
    rate.setBounds (knobRow.removeFromLeft (cellWidth).reduced (6, 0));
    depth.setBounds (knobRow.removeFromLeft (cellWidth).reduced (6, 0));
    smooth.setBounds (knobRow.removeFromLeft (cellWidth).reduced (6, 0));

    auto buttonCell = knobRow.reduced (10, 0);
    gateButton.setBounds (buttonCell.withSizeKeepingCentre (buttonCell.getWidth(),
                                                            juce::jmin (56, buttonCell.getHeight())));
}

} // namespace mk
