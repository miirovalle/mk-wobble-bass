#include "UI/Pages.h"

namespace mk
{
namespace
{

std::vector<int> paramsInGroup (Group group)
{
    std::vector<int> indices;
    for (int i = 0; i < kParamCount; ++i)
        if (paramMeta (i).group == group)
            indices.push_back (i);
    return indices;
}

} // namespace

//==============================================================================
ParamGridPage::ParamGridPage (juce::AudioProcessorValueTreeState& state, juce::String pageTitle,
                              std::vector<int> paramIndices, LearnCallback learnCallback)
    : title (std::move (pageTitle))
{
    for (const int index : paramIndices)
    {
        int choiceCount = 0;
        if (paramChoices (index, choiceCount) != nullptr)
        {
            auto combo = std::make_unique<ParamCombo> (state, index);
            addAndMakeVisible (*combo);
            controls.push_back (std::move (combo));
        }
        else
        {
            auto knob = std::make_unique<ParamKnob> (state, index);
            if (learnCallback != nullptr)
                knob->onLearnRequest = learnCallback;
            addAndMakeVisible (*knob);
            controls.push_back (std::move (knob));
        }
    }
}

void ParamGridPage::setColumns (int newColumns)
{
    columns = juce::jmax (1, newColumns);
    resized();
}

void ParamGridPage::paint (juce::Graphics& g)
{
    g.setColour (theme::textDim().withAlpha (0.9f));
    g.setFont (theme::labelFont (12.0f));
    g.drawFittedText (title.toUpperCase(), getLocalBounds().removeFromTop (18).reduced (6, 0),
                      juce::Justification::centredLeft, 1);
}

void ParamGridPage::resized()
{
    if (controls.empty())
        return;

    auto bounds = getLocalBounds();
    bounds.removeFromTop (20);

    const int rows = (int) ((controls.size() + (size_t) columns - 1) / (size_t) columns);
    const int width = bounds.getWidth() / columns;
    const int height = juce::jmax (64, bounds.getHeight() / juce::jmax (1, rows));

    for (size_t i = 0; i < controls.size(); ++i)
    {
        const int column = (int) (i % (size_t) columns);
        const int row = (int) (i / (size_t) columns);
        controls[i]->setBounds (bounds.getX() + column * width, bounds.getY() + row * height,
                                width, height);
    }
}

//==============================================================================
MainPage::MainPage (juce::AudioProcessorValueTreeState& state, const Engine& engine,
                    LearnCallback learnCallback)
    : wobbleDisplay (engine),
      wobbleRate (state, (int) P::wobDiv, "WOBBLE RATE"),
      wobbleDepth (state, (int) P::wobDepth, "WOBBLE DEPTH")
{
    addAndMakeVisible (wobbleDisplay);

    if (learnCallback != nullptr)
        wobbleDepth.onLearnRequest = learnCallback;
    addAndMakeVisible (wobbleRate);
    addAndMakeVisible (wobbleDepth);

    static const char* const macroNames[kMacroCount] = { "BASS", "GROWL", "MOVEMENT", "DRIVE",
                                                         "FILTER", "SUB", "WIDTH", "MIX" };
    for (int i = 0; i < kMacroCount; ++i)
    {
        auto knob = std::make_unique<ParamKnob> (state, (int) P::macBass + i, macroNames[i]);
        if (learnCallback != nullptr)
            knob->onLearnRequest = learnCallback;
        addAndMakeVisible (*knob);
        macros[(size_t) i] = std::move (knob);
    }
}

void MainPage::paint (juce::Graphics& g)
{
    juce::ignoreUnused (g);
}

void MainPage::resized()
{
    auto bounds = getLocalBounds().reduced (4);

    auto right = bounds.removeFromRight (juce::jmax (260, bounds.getWidth() * 42 / 100));
    bounds.removeFromRight (10);
    wobbleDisplay.setBounds (right);

    auto top = bounds.removeFromTop (juce::jlimit (100, 150, bounds.getHeight() / 3));
    const auto left = top.removeFromLeft (top.getWidth() / 2);
    wobbleRate.setBounds (left.getX() + 10, left.getCentreY() - 28,
                          juce::jmin (230, left.getWidth() - 20), 64);
    wobbleDepth.setBounds (top.reduced (4));

    const int macroWidth = bounds.getWidth() / 4;
    const int macroHeight = bounds.getHeight() / 2;
    for (int i = 0; i < kMacroCount; ++i)
    {
        const int column = i % 4;
        const int row = i / 4;
        macros[(size_t) i]->setBounds (bounds.getX() + column * macroWidth,
                                       bounds.getY() + row * macroHeight, macroWidth, macroHeight);
    }
}

//==============================================================================
MatrixPage::MatrixPage (juce::AudioProcessorValueTreeState& state, LearnCallback learnCallback)
{
    for (int slot = 0; slot < kMatrixSlots; ++slot)
    {
        auto& row = rows[(size_t) slot];
        const int base = (int) P::mat1Src + slot * 3;

        row.source = std::make_unique<ParamCombo> (state, base, "Source");
        row.destination = std::make_unique<ParamCombo> (state, base + 1, "Destination");
        row.amount = std::make_unique<ParamKnob> (state, base + 2, "Amount");
        if (learnCallback != nullptr)
            row.amount->onLearnRequest = learnCallback;

        addAndMakeVisible (*row.source);
        addAndMakeVisible (*row.destination);
        addAndMakeVisible (*row.amount);
    }
}

void MatrixPage::paint (juce::Graphics& g)
{
    g.setColour (theme::textDim().withAlpha (0.9f));
    g.setFont (theme::labelFont (12.0f));
    g.drawFittedText ("MODULATION MATRIX  -  SOURCE > DESTINATION", getLocalBounds().removeFromTop (18).reduced (6, 0),
                      juce::Justification::centredLeft, 1);
}

void MatrixPage::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (20);

    const int rowHeight = juce::jmax (44, bounds.getHeight() / kMatrixSlots);
    const int amountWidth = juce::jmin (110, bounds.getWidth() / 5);

    for (int slot = 0; slot < kMatrixSlots; ++slot)
    {
        auto area = bounds.removeFromTop (rowHeight).reduced (4, 1);
        auto& row = rows[(size_t) slot];

        row.amount->setBounds (area.removeFromRight (amountWidth));
        row.source->setBounds (area.removeFromLeft (area.getWidth() / 2 - 4));
        area.removeFromLeft (8);
        row.destination->setBounds (area);
    }
}

} // namespace mk
