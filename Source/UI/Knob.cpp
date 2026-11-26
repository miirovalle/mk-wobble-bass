#include "UI/Knob.h"

namespace mk
{

ParamKnob::ParamKnob (juce::AudioProcessorValueTreeState& state, int paramIndex,
                      const juce::String& labelOverride)
    : apvts (state), index (paramIndex)
{
    labelText = labelOverride.isNotEmpty() ? labelOverride : juce::String (paramName (paramIndex));

    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                juce::MathConstants<float>::pi * 2.75f, true);
    slider.setDoubleClickReturnValue (true, (double) paramMeta (paramIndex).def);
    slider.setTooltip (juce::String (paramName (paramIndex)));

    addAndMakeVisible (slider);
    // The attachment installs the parameter's own range (including skew), so
    // slider.getValue() is always the plain parameter value.
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, paramId (paramIndex), slider);

    slider.onValueChange = [this] { refreshValueText(); };
    refreshValueText();
}

ParamKnob::~ParamKnob()
{
    slider.onValueChange = nullptr;
}

void ParamKnob::setCompact (bool shouldBeCompact)
{
    compact = shouldBeCompact;
    repaint();
}

void ParamKnob::refreshValueText()
{
    cachedValue = (float) slider.getValue();
    repaint();
}

void ParamKnob::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    g.setColour (theme::textDim());
    g.setFont (theme::labelFont (compact ? 11.0f : 12.5f));
    g.drawFittedText (labelText, bounds.removeFromTop (compact ? 16 : 20),
                      juce::Justification::centred, 1);

    char text[32] {};
    formatParamValue (index, (float) slider.getValue(), text, (int) sizeof (text));

    g.setColour (theme::text());
    g.setFont (theme::valueFont (compact ? 12.0f : 14.0f));
    g.drawFittedText (text, bounds.removeFromBottom (compact ? 16 : 20),
                      juce::Justification::centred, 1);
}

void ParamKnob::resized()
{
    slider.setBounds (getLocalBounds().reduced (0, compact ? 16 : 20));
}

void ParamKnob::mouseDown (const juce::MouseEvent& event)
{
    if ((event.mods.isPopupMenu() || event.mods.isRightButtonDown()) && onLearnRequest != nullptr)
        onLearnRequest (index);
}

void ParamKnob::mouseDoubleClick (const juce::MouseEvent& event)
{
    if (slider.getBounds().contains (event.getPosition()))
        return;

    if (auto* parameter = apvts.getParameter (paramId (index)))
        parameter->setValueNotifyingHost (parameter->getDefaultValue());
}

//==============================================================================
ParamCombo::ParamCombo (juce::AudioProcessorValueTreeState& state, int paramIndex,
                        const juce::String& labelOverride)
    : apvts (state), index (paramIndex)
{
    labelText = labelOverride.isNotEmpty() ? labelOverride : juce::String (paramName (paramIndex));

    int count = 0;
    if (const auto* choices = paramChoices (paramIndex, count))
        for (int i = 0; i < count; ++i)
            combo.addItem (choices[i], i + 1);

    combo.setJustificationType (juce::Justification::centredLeft);
    combo.setTooltip (juce::String (paramName (paramIndex)));
    addAndMakeVisible (combo);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, paramId (paramIndex), combo);
}

ParamCombo::~ParamCombo() = default;

void ParamCombo::paint (juce::Graphics& g)
{
    g.setColour (theme::textDim());
    g.setFont (theme::labelFont (11.5f));
    g.drawFittedText (labelText, getLocalBounds().removeFromTop (16),
                      juce::Justification::centredLeft, 1);
}

void ParamCombo::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (16);
    combo.setBounds (bounds.reduced (0, 2));
}

//==============================================================================
SectionPanel::SectionPanel (juce::String panelTitle) : title (std::move (panelTitle))
{
    setInterceptsMouseClicks (false, true);
}

void SectionPanel::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (0.5f);

    g.setColour (theme::panel());
    g.fillRoundedRectangle (bounds, 10.0f);

    juce::ColourGradient glow (theme::accent().withAlpha (0.16f), bounds.getTopLeft(),
                               theme::panel(), bounds.getBottomLeft(), false);
    g.setGradientFill (glow);
    g.fillRoundedRectangle (bounds, 10.0f);

    g.setColour (theme::outline());
    g.drawRoundedRectangle (bounds, 10.0f, 1.0f);

    if (title.isNotEmpty())
    {
        g.setColour (theme::accentBright());
        g.setFont (theme::labelFont (12.0f));
        g.drawFittedText (title.toUpperCase(), getLocalBounds().removeFromTop (22).reduced (12, 0),
                          juce::Justification::centredLeft, 1);
    }
}

} // namespace mk
