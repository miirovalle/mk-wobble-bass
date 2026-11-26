#include "UI/Pages.h"

namespace mk
{
namespace
{

juce::RangedAudioParameter* rangedParam (juce::AudioProcessorValueTreeState& state, int index)
{
    return dynamic_cast<juce::RangedAudioParameter*> (state.getParameter (paramId (index)));
}

void styleChip (juce::TextButton& button)
{
    button.setColour (juce::TextButton::buttonColourId, theme::panelLight());
    button.setColour (juce::TextButton::buttonOnColourId, theme::accentDeep());
    button.setColour (juce::TextButton::textColourOffId, theme::textDim());
    button.setColour (juce::TextButton::textColourOnId, theme::text());
}

std::vector<PanelSpec> oscPanels()
{
    return {
        { "OSCILLATOR 1",
          { paramSpec ((int) P::osc1Wave), paramSpec ((int) P::osc1Bank), paramSpec ((int) P::osc1Pos),
            paramSpec ((int) P::osc1Level), paramSpec ((int) P::osc1Oct), paramSpec ((int) P::osc1Semi),
            paramSpec ((int) P::osc1Fine), paramSpec ((int) P::osc1Uni), paramSpec ((int) P::osc1Detune),
            paramSpec ((int) P::osc1Spread), paramSpec ((int) P::osc1Pw), paramSpec ((int) P::osc1Phase) },
          4, 1.15f },
        { "OSCILLATOR 2",
          { paramSpec ((int) P::osc2Mode), paramSpec ((int) P::osc2Wave), paramSpec ((int) P::osc2Bank),
            paramSpec ((int) P::osc2Pos), paramSpec ((int) P::osc2Level), paramSpec ((int) P::osc2Oct),
            paramSpec ((int) P::osc2Semi), paramSpec ((int) P::osc2Fine), paramSpec ((int) P::osc2Uni),
            paramSpec ((int) P::osc2Detune), paramSpec ((int) P::osc2Spread), paramSpec ((int) P::osc2Pw),
            paramSpec ((int) P::osc2Ratio), paramSpec ((int) P::osc2Fm), paramSpec ((int) P::osc2Ring) },
          4, 1.15f },
        { "SUB / FM / OSC 3",
          { paramSpec ((int) P::subWave), paramSpec ((int) P::subOct), paramSpec ((int) P::subLevel),
            paramSpec ((int) P::subDrive), paramSpec ((int) P::subLock), paramSpec ((int) P::fmFeedback),
            paramSpec ((int) P::fmModWave), paramSpec ((int) P::osc3Wave), paramSpec ((int) P::osc3Level),
            paramSpec ((int) P::osc3Pos) },
          2, 0.9f },
    };
}

std::vector<PanelSpec> filterPanels()
{
    return {
        { "FILTER 1",
          { paramSpec ((int) P::fltType), paramSpec ((int) P::fltCutoff), paramSpec ((int) P::fltReso),
            paramSpec ((int) P::fltDrive), paramSpec ((int) P::fltKeyTrack), paramSpec ((int) P::fltEnvAmt),
            paramSpec ((int) P::fltMix), paramSpec ((int) P::fltRoute) },
          4, 1.3f },
        { "FILTER 2",
          { paramSpec ((int) P::flt2Type), paramSpec ((int) P::flt2Cutoff), paramSpec ((int) P::flt2Reso) },
          3, 1.0f },
        { "TALKING",
          { paramSpec ((int) P::fltFormant), paramSpec ((int) P::formantMix),
            paramSpec ((int) P::wobFormant), paramSpec ((int) P::wobCut) },
          2, 0.85f },
    };
}

std::vector<PanelSpec> envPanels()
{
    return {
        { "AMP ENVELOPE",
          { paramSpec ((int) P::ampA), paramSpec ((int) P::ampD), paramSpec ((int) P::ampS),
            paramSpec ((int) P::ampR), paramSpec ((int) P::ampCurve) },
          3, 1.0f },
        { "FILTER ENVELOPE",
          { paramSpec ((int) P::fltA), paramSpec ((int) P::fltD), paramSpec ((int) P::fltS),
            paramSpec ((int) P::fltR) },
          2, 0.9f },
        { "PITCH / MOD ENVELOPE",
          { paramSpec ((int) P::pitA), paramSpec ((int) P::pitD), paramSpec ((int) P::pitS),
            paramSpec ((int) P::pitR), paramSpec ((int) P::pitAmt), paramSpec ((int) P::modA),
            paramSpec ((int) P::modD), paramSpec ((int) P::modS), paramSpec ((int) P::modR) },
          3, 1.2f },
        { "VOICE",
          { paramSpec ((int) P::polyMode), paramSpec ((int) P::polyphony), paramSpec ((int) P::glideTime),
            paramSpec ((int) P::velSens), paramSpec ((int) P::bendRange), paramSpec ((int) P::quality),
            paramSpec ((int) P::oversample), paramSpec ((int) P::drift) },
          2, 1.0f },
    };
}

std::vector<PanelSpec> distPanels()
{
    return {
        { "DISTORTION",
          { paramSpec ((int) P::distType), paramSpec ((int) P::distDrive), paramSpec ((int) P::distTone),
            paramSpec ((int) P::distMix), paramSpec ((int) P::distOut), paramSpec ((int) P::distPre),
            paramSpec ((int) P::distRoute) },
          4, 1.3f },
        { "MULTIBAND DRIVE",
          { paramSpec ((int) P::mbOn), paramSpec ((int) P::mbDrive), paramSpec ((int) P::mbMix) },
          3, 0.9f },
    };
}

std::vector<PanelSpec> fxPanels()
{
    return {
        { "CHORUS / PHASER / FLANGER",
          { paramSpec ((int) P::chOn), paramSpec ((int) P::chRate), paramSpec ((int) P::chDepth),
            paramSpec ((int) P::chMix), paramSpec ((int) P::chWidth), paramSpec ((int) P::phOn),
            paramSpec ((int) P::phRate), paramSpec ((int) P::phDepth), paramSpec ((int) P::phFb),
            paramSpec ((int) P::phStages), paramSpec ((int) P::phMix), paramSpec ((int) P::flOn),
            paramSpec ((int) P::flRate), paramSpec ((int) P::flDepth), paramSpec ((int) P::flFb),
            paramSpec ((int) P::flDelay), paramSpec ((int) P::flMix) },
          6, 1.35f },
        { "DELAY / REVERB",
          { paramSpec ((int) P::dlOn), paramSpec ((int) P::dlSync), paramSpec ((int) P::dlDiv),
            paramSpec ((int) P::dlTime), paramSpec ((int) P::dlFb), paramSpec ((int) P::dlPing),
            paramSpec ((int) P::dlFilter), paramSpec ((int) P::dlMix), paramSpec ((int) P::rvOn),
            paramSpec ((int) P::rvSize), paramSpec ((int) P::rvDecay), paramSpec ((int) P::rvDamp),
            paramSpec ((int) P::rvWidth), paramSpec ((int) P::rvMix) },
          5, 1.15f },
        { "DYNAMICS",
          { paramSpec ((int) P::cpOn), paramSpec ((int) P::cpThresh), paramSpec ((int) P::cpRatio),
            paramSpec ((int) P::cpAttack), paramSpec ((int) P::cpRelease), paramSpec ((int) P::cpMakeup),
            paramSpec ((int) P::ottOn), paramSpec ((int) P::ottAmount), paramSpec ((int) P::ottMix) },
          3, 0.9f },
    };
}

const std::array<const char*, 9>& sectionNames()
{
    static const std::array<const char*, 9> names {
        "WOBBLE", "OSC", "FILTER", "ENV", "LFO", "MATRIX", "DIST", "FX", "GATE"
    };
    return names;
}

} // namespace

//==============================================================================
ParamPanel::ParamPanel (juce::AudioProcessorValueTreeState& state, juce::String title,
                        std::vector<ParamSpec> params, int columns)
    : titleText (std::move (title)),
      columnCount (juce::jlimit (1, 8, columns))
{
    for (const auto& spec : params)
    {
        int count = 0;
        juce::Component* widget = nullptr;

        if (paramChoices (spec.index, count) != nullptr)
            widget = new ParamChoice (state, spec.index, spec.label);
        else
            widget = new ParamKnob (state, spec.index, spec.label);

        widgets.add (widget);
        addAndMakeVisible (widget);
    }
}

void ParamPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (theme::panel());
    g.fillRoundedRectangle (bounds, 10.0f);
    g.setColour (theme::outline());
    g.drawRoundedRectangle (bounds, 10.0f, 1.0f);

    auto header = getLocalBounds().reduced (10, 0).removeFromTop (22);
    g.setColour (theme::accent());
    g.setFont (theme::labelFont (12.0f));
    g.drawFittedText (titleText, header, juce::Justification::centredLeft, 1);

    g.setColour (theme::accent().withAlpha (0.5f));
    g.fillRect (header.getX(), header.getBottom() - 2, 26, 2);
}

void ParamPanel::resized()
{
    auto area = getLocalBounds().reduced (8, 6);
    area.removeFromTop (18);

    const int count = widgets.size();
    if (count == 0 || area.isEmpty())
        return;

    const int rows = (count + columnCount - 1) / columnCount;
    const int cellWidth = area.getWidth() / columnCount;
    const int cellHeight = area.getHeight() / rows;

    for (int i = 0; i < count; ++i)
    {
        const int row = i / columnCount;
        const int column = i % columnCount;
        widgets[i]->setBounds (area.getX() + column * cellWidth,
                               area.getY() + row * cellHeight,
                               cellWidth, cellHeight);
    }
}

//==============================================================================
SectionPage::SectionPage (juce::AudioProcessorValueTreeState& state, std::vector<PanelSpec> specs)
{
    float total = 0.0f;
    for (auto& spec : specs)
    {
        panels.add (new ParamPanel (state, spec.title, std::move (spec.params), spec.columns));
        const float weight = juce::jmax (0.2f, spec.weight);
        weights.push_back (weight);
        total += weight;
    }

    for (auto* panel : panels)
    {
        addAndMakeVisible (panel);
    }

    if (total > 0.0f)
        for (auto& weight : weights)
            weight /= total;
}

void SectionPage::resized()
{
    auto area = getLocalBounds();
    const int gap = 6;
    const int usable = area.getWidth() - gap * (panels.size() - 1);
    int x = area.getX();

    for (int i = 0; i < panels.size(); ++i)
    {
        const bool last = i == panels.size() - 1;
        const int width = last ? area.getRight() - x
                               : juce::roundToInt ((float) usable * weights[(size_t) i]);
        panels[i]->setBounds (x, area.getY(), width, area.getHeight());
        x += width + gap;
    }
}

//==============================================================================
CharacterSelector::CharacterSelector (juce::AudioProcessorValueTreeState& state, int paramIndex,
                                      const juce::String& title)
    : apvts (state), parameterIndex (paramIndex), caption (title)
{
    int count = 0;
    const auto* names = paramChoices (paramIndex, count);

    for (int i = 0; i < kCharacterCount; ++i)
    {
        auto& button = buttons[(size_t) i];
        button.setButtonText (names != nullptr && i < count ? names[i] : juce::String (i + 1));
        button.setClickingTogglesState (false);
        styleChip (button);
        button.onClick = [this, i] { select (i); };
        addAndMakeVisible (button);
    }

    if (auto* parameter = rangedParam (state, paramIndex))
    {
        attachment = std::make_unique<juce::ParameterAttachment> (
            *parameter, [this] (float value) { refresh ((int) std::lround (value)); });
        attachment->sendInitialUpdate();
    }
}

void CharacterSelector::select (int index)
{
    if (attachment != nullptr)
        attachment->setValueAsCompleteGesture ((float) index);
}

void CharacterSelector::refresh (int index)
{
    for (int i = 0; i < kCharacterCount; ++i)
        buttons[(size_t) i].setToggleState (i == index, juce::dontSendNotification);
}

void CharacterSelector::paint (juce::Graphics& g)
{
    g.setColour (theme::textDim());
    g.setFont (theme::labelFont (11.0f));
    g.drawFittedText (caption, getLocalBounds().removeFromTop (16), juce::Justification::centredLeft, 1);
}

void CharacterSelector::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (16);

    const int width = area.getWidth() / kCharacterCount;
    for (int i = 0; i < kCharacterCount; ++i)
        buttons[(size_t) i].setBounds (area.removeFromLeft (width).reduced (2, 1));
}

//==============================================================================
RateSelector::RateSelector (juce::AudioProcessorValueTreeState& state) : apvts (state)
{
    int count = 0;
    const auto* divisions = syncDivisions (count);

    for (int i = 0; i < count; ++i)
        combo.addItem (divisions[i].name, i + 1);

    combo.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (combo);

    if (auto* parameter = rangedParam (state, (int) P::wobDiv))
    {
        attachment = std::make_unique<juce::ParameterAttachment> (
            *parameter, [this] (float value) { refresh (value); });
        attachment->sendInitialUpdate();
    }

    combo.onChange = [this]
    {
        if (attachment != nullptr)
            attachment->setValueAsCompleteGesture ((float) combo.getSelectedItemIndex());

        // SMART WOBBLE: touching the rate locks the wobble back to the host.
        if (auto* mode = rangedParam (apvts, (int) P::wobMode))
            mode->setValueNotifyingHost (1.0f);
    };
}

void RateSelector::refresh (float division)
{
    combo.setSelectedItemIndex (juce::jlimit (0, combo.getNumItems() - 1, (int) std::lround (division)),
                                juce::dontSendNotification);
}

void RateSelector::paint (juce::Graphics& g)
{
    g.setColour (theme::textDim());
    g.setFont (theme::labelFont (11.0f));
    g.drawFittedText ("WOBBLE RATE", getLocalBounds().removeFromTop (16),
                      juce::Justification::centredLeft, 1);
}

void RateSelector::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (16);
    combo.setBounds (area.reduced (0, 2));
}

//==============================================================================
EasyPage::EasyPage (juce::AudioProcessorValueTreeState& state, Engine& engine)
    : character (state, (int) P::bassChar, "BASS CHARACTER"),
      morphTo (state, (int) P::morphChar, "MORPH TO"),
      rate (state),
      wobble (state, (int) P::easyWobble, "WOBBLE", true),
      bass (state, (int) P::easyBass, "BASS", true),
      growl (state, (int) P::easyGrowl, "GROWL", true),
      grit (state, (int) P::easyGrit, "GRIT", true),
      movement (state, (int) P::easyMovement, "MOVEMENT", true),
      width (state, (int) P::easyWidth, "WIDTH", true),
      morph (state, (int) P::bassMorph, "BASS MORPH"),
      subLevel (state, (int) P::subLevel, "SUB"),
      master (state, (int) P::outputDb, "MASTER"),
      wobbleStrip (engine)
{
    addAndMakeVisible (character);
    addAndMakeVisible (morphTo);
    addAndMakeVisible (rate);
    addAndMakeVisible (morph);
    addAndMakeVisible (subLevel);
    addAndMakeVisible (master);
    addAndMakeVisible (wobbleStrip);

    for (auto* knob : { &wobble, &bass, &growl, &grit, &movement, &width })
        addAndMakeVisible (knob);
}

void EasyPage::paint (juce::Graphics& g)
{
    auto area = getLocalBounds();
    area.removeFromTop (68);
    area.removeFromBottom (54);

    auto panel = area.reduced (0, 4).toFloat();
    g.setColour (theme::panel().withAlpha (0.55f));
    g.fillRoundedRectangle (panel, 12.0f);
    g.setColour (theme::outline().withAlpha (0.8f));
    g.drawRoundedRectangle (panel, 12.0f, 1.0f);

    auto footer = getLocalBounds().removeFromBottom (54).withTrimmedTop (4);
    const int rateWidth = juce::jmin (240, footer.getWidth() / 3);
    g.setColour (theme::textDim());
    g.setFont (theme::valueFont (11.0f));
    g.drawFittedText ("ONE-KNOB FILTH: TURN GRIT UP",
                      footer.withTrimmedLeft (rateWidth + 16).reduced (0, 0),
                      juce::Justification::centredLeft, 1);
}

void EasyPage::resized()
{
    auto area = getLocalBounds();

    character.setBounds (area.removeFromTop (64));
    area.removeFromTop (4);

    auto footer = area.removeFromBottom (50);
    area.removeFromBottom (4);
    rate.setBounds (footer.removeFromLeft (juce::jmin (240, footer.getWidth() / 3)));

    auto right = area.removeFromRight (juce::jmin (250, area.getWidth() / 4));
    area.removeFromRight (10);

    auto rightTop = right.removeFromTop (right.getHeight() / 2);
    subLevel.setBounds (rightTop.removeFromLeft (rightTop.getWidth() / 2).reduced (4));
    master.setBounds (rightTop.reduced (4));

    morph.setBounds (right.removeFromLeft (juce::jmin (104, right.getWidth() / 2)).reduced (4));
    morphTo.setBounds (right.reduced (4));

    wobbleStrip.setBounds (area.removeFromBottom (68));
    area.removeFromBottom (6);

    const int knobWidth = area.getWidth() / 6;
    for (auto* knob : { &wobble, &bass, &growl, &grit, &movement, &width })
        knob->setBounds (area.removeFromLeft (knobWidth).reduced (4));
}

//==============================================================================
MatrixPage::MatrixPage (juce::AudioProcessorValueTreeState& state)
    : apvts (state),
      macros (state, "EASY MACROS",
              { paramSpec ((int) P::easyWobble, "WOBBLE"), paramSpec ((int) P::easyBass, "BASS"),
                paramSpec ((int) P::easyGrowl, "GROWL"), paramSpec ((int) P::easyGrit, "GRIT"),
                paramSpec ((int) P::easyMovement, "MOVE"), paramSpec ((int) P::easyWidth, "WIDTH"),
                paramSpec ((int) P::macro7, "MACRO 7"), paramSpec ((int) P::macro8, "MACRO 8") },
              2)
{
    int sourceCount = 0;
    int destinationCount = 0;
    const auto* sources = paramChoices ((int) P::mat1Src, sourceCount);
    const auto* destinations = paramChoices ((int) P::mat1Dst, destinationCount);

    for (int slot = 0; slot < kMatrixSlots; ++slot)
    {
        auto row = std::make_unique<Row>();
        const int base = (int) P::mat1Src + slot * 3;

        for (int i = 0; i < sourceCount; ++i)
            row->source.addItem (sources[i], i + 1);
        for (int i = 0; i < destinationCount; ++i)
            row->destination.addItem (destinations[i], i + 1);

        row->source.setJustificationType (juce::Justification::centred);
        row->destination.setJustificationType (juce::Justification::centred);

        row->amount.setSliderStyle (juce::Slider::LinearHorizontal);
        row->amount.setTextBoxStyle (juce::Slider::TextBoxRight, false, 52, 20);
        row->amount.setRange (-1.0, 1.0, 0.01);
        row->amount.setDoubleClickReturnValue (true, 0.0);
        row->amount.setColour (juce::Slider::thumbColourId, theme::accent());
        row->amount.setColour (juce::Slider::trackColourId, theme::accentDeep());
        row->amount.setColour (juce::Slider::backgroundColourId, theme::outline());
        row->amount.setColour (juce::Slider::textBoxTextColourId, theme::text());
        row->amount.setColour (juce::Slider::textBoxBackgroundColourId, theme::panelLight());
        row->amount.setColour (juce::Slider::textBoxOutlineColourId, theme::outline());

        row->sourceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            state, paramId (base), row->source);
        row->destinationAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            state, paramId (base + 1), row->destination);
        row->amountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            state, paramId (base + 2), row->amount);

        addAndMakeVisible (row->source);
        addAndMakeVisible (row->destination);
        addAndMakeVisible (row->amount);
        rows[(size_t) slot] = std::move (row);
    }

    addAndMakeVisible (macros);
}

void MatrixPage::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().reduced (2);
    auto right = area.removeFromRight (juce::jmin (320, area.getWidth() / 3));
    area.removeFromRight (8);
    juce::ignoreUnused (right);

    auto header = area.removeFromTop (22);
    header.removeFromLeft (26);

    g.setColour (theme::textDim());
    g.setFont (theme::labelFont (10.0f));

    const int sourceWidth = header.getWidth() * 38 / 100;
    g.drawFittedText ("SOURCE", header.removeFromLeft (sourceWidth).reduced (6, 0),
                      juce::Justification::centredLeft, 1);

    header.removeFromLeft (4);
    const int destinationWidth = header.getWidth() * 62 / 100;
    g.drawFittedText ("DESTINATION", header.removeFromLeft (destinationWidth).reduced (6, 0),
                      juce::Justification::centredLeft, 1);

    header.removeFromLeft (4);
    g.drawFittedText ("AMOUNT", header.reduced (6, 0), juce::Justification::centredRight, 1);

    const int rowHeight = area.getHeight() > 0 ? area.getHeight() / kMatrixSlots : 0;
    g.setFont (theme::valueFont (11.0f));

    for (int slot = 0; slot < kMatrixSlots; ++slot)
    {
        auto number = juce::Rectangle<int> (area.getX() + 4,
                                            area.getY() + slot * rowHeight,
                                            20, rowHeight);
        g.setColour (((slot + 1) % 2) == 0 ? theme::accentDeep() : theme::textDim());
        g.drawFittedText (juce::String (slot + 1), number, juce::Justification::centred, 1);
    }
}

void MatrixPage::resized()
{
    auto area = getLocalBounds().reduced (2);

    auto right = area.removeFromRight (juce::jmin (320, area.getWidth() / 3));
    area.removeFromRight (8);
    macros.setBounds (right);

    area.removeFromTop (22);
    const int rowHeight = area.getHeight() > 0 ? area.getHeight() / kMatrixSlots : 0;

    for (int slot = 0; slot < kMatrixSlots; ++slot)
    {
        auto& row = *rows[(size_t) slot];
        auto line = area.removeFromTop (rowHeight).reduced (2, 1);

        line.removeFromLeft (26);
        const int sourceWidth = line.getWidth() * 38 / 100;
        row.source.setBounds (line.removeFromLeft (sourceWidth));
        line.removeFromLeft (4);
        const int destinationWidth = line.getWidth() * 62 / 100;
        row.destination.setBounds (line.removeFromLeft (destinationWidth));
        line.removeFromLeft (4);
        row.amount.setBounds (line);
    }
}

//==============================================================================
AdvancedPage::AdvancedPage (juce::AudioProcessorValueTreeState& state, Engine& engineToUse)
    : apvts (state), engine (engineToUse)
{
    owned.add (new WobblePage (state, engineToUse));
    owned.add (new SectionPage (state, oscPanels()));
    owned.add (new SectionPage (state, filterPanels()));
    owned.add (new SectionPage (state, envPanels()));
    owned.add (new LfoPage (state));
    owned.add (new MatrixPage (state));
    owned.add (new SectionPage (state, distPanels()));
    owned.add (new SectionPage (state, fxPanels()));

    gatePage = new GatePage (state, engineToUse);
    owned.add (gatePage);

    const auto& names = sectionNames();
    for (int i = 0; i < kSectionCount; ++i)
    {
        sections[(size_t) i] = owned[i];
        addChildComponent (owned[i]);

        auto& tab = tabs[(size_t) i];
        tab.setButtonText (names[(size_t) i]);
        tab.setClickingTogglesState (true);
        tab.setRadioGroupId (7311);
        tab.setColour (juce::TextButton::buttonOnColourId, theme::accentDeep());
        tab.onClick = [this, i] { showSection (i); };
        addAndMakeVisible (tab);
    }

    showSection (0);
}

void AdvancedPage::paint (juce::Graphics& g)
{
    juce::ignoreUnused (g);
}

void AdvancedPage::refresh()
{
    if (gatePage != nullptr)
        gatePage->refresh();
}

void AdvancedPage::showSection (int index)
{
    current = juce::jlimit (0, kSectionCount - 1, index);

    for (int i = 0; i < kSectionCount; ++i)
    {
        tabs[(size_t) i].setToggleState (i == current, juce::dontSendNotification);
        sections[(size_t) i]->setVisible (i == current);
    }
}

void AdvancedPage::resized()
{
    auto area = getLocalBounds();

    auto strip = area.removeFromTop (26);
    area.removeFromTop (4);

    const int tabWidth = strip.getWidth() / kSectionCount;
    for (auto& tab : tabs)
        tab.setBounds (strip.removeFromLeft (tabWidth).reduced (1, 0));

    for (auto* section : sections)
        section->setBounds (area);
}

} // namespace mk
