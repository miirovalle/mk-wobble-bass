#include "UI/Pages.h"

namespace mk
{
namespace
{

constexpr int kItemHeight = 30;
constexpr int kRowGap = 4;
constexpr int kTitleHeight = 22;
constexpr int kSectionPadding = 10;
constexpr int kSectionGap = 8;

const char* sectionTitle (Group group) noexcept
{
    switch (group)
    {
        case Group::Glob: return "VOICE";
        case Group::Osc1: return "OSC 1";
        case Group::Osc2: return "OSC 2";
        case Group::Osc3: return "OSC 3";
        case Group::Sub:  return "SUB";
        case Group::Fm:   return "FM";
        case Group::Filt: return "FILTER";
        case Group::Wob:  return "WOBBLE";
        case Group::Lfo:  return "LFO";
        case Group::Env:  return "ENVELOPES";
        case Group::Dist: return "DISTORTION";
        case Group::Fx:   return "EFFECTS";
        case Group::Mac:  return "MACROS";
        case Group::Mat:  return "MATRIX";
        case Group::Misc: return "MISC";
        default:          return "PARAMETERS";
    }
}

} // namespace

AdvancedPage::AdvancedPage (juce::AudioProcessorValueTreeState& state)
{
    for (int groupIndex = 0; groupIndex < (int) Group::kGroupCount; ++groupIndex)
    {
        const auto group = (Group) groupIndex;

        if (group == Group::Mat)
            continue;

        GroupView view;
        view.title = sectionTitle (group);

        for (int i = 0; i < kParamCount; ++i)
        {
            if (paramMeta (i).group != group)
                continue;

            int choiceCount = 0;

            if (paramChoices (i, choiceCount) != nullptr)
            {
                auto combo = std::make_unique<ParamCombo> (state, i);
                addAndMakeVisible (*combo);
                view.items.push_back (std::move (combo));
            }
            else
            {
                auto slider = std::make_unique<ParamSlider> (state, i);
                addAndMakeVisible (*slider);
                view.items.push_back (std::move (slider));
            }
        }

        if (! view.items.empty())
            groups.push_back (std::move (view));
    }

    for (int slot = 0; slot < kMatrixSlots; ++slot)
    {
        auto& row = matrix[(size_t) slot];
        const int base = (int) P::mat1Src + slot * 3;

        row.source = std::make_unique<ParamCombo> (state, base, "Source " + juce::String (slot + 1));
        row.destination = std::make_unique<ParamCombo> (state, base + 1, "Destination");
        row.amount = std::make_unique<ParamSlider> (state, base + 2, "Amount");

        addAndMakeVisible (*row.source);
        addAndMakeVisible (*row.destination);
        addAndMakeVisible (*row.amount);
    }
}

int AdvancedPage::columnsFor (int width)
{
    return juce::jlimit (2, 5, width / 230);
}

int AdvancedPage::preferredHeight (int width) const
{
    const int columnsNow = columnsFor (width);
    int total = 0;

    for (const auto& group : groups)
    {
        const int rows = ((int) group.items.size() + columnsNow - 1) / columnsNow;
        total += kTitleHeight + rows * (kItemHeight + kRowGap) + kSectionPadding * 2 + kSectionGap;
    }

    total += kTitleHeight + kMatrixSlots * (kItemHeight + kRowGap) + kSectionPadding * 2;

    return juce::jmax (total, 400);
}

void AdvancedPage::resized()
{
    columns = columnsFor (juce::jmax (200, getWidth()));
    const int itemWidth = juce::jmax (120, (getWidth() - kSectionPadding * 2) / columns);

    int y = 0;

    for (auto& group : groups)
    {
        y += kSectionPadding;

        const int rows = ((int) group.items.size() + columns - 1) / columns;

        for (size_t i = 0; i < group.items.size(); ++i)
        {
            const int column = (int) i % columns;
            const int row = (int) i / columns;

            group.items[i]->setBounds (kSectionPadding + column * itemWidth,
                                       y + kTitleHeight + row * (kItemHeight + kRowGap),
                                       itemWidth - 8, kItemHeight);
        }

        y += kTitleHeight + rows * (kItemHeight + kRowGap) + kSectionPadding + kSectionGap;
    }

    // Mod matrix rows.
    y += kSectionPadding;
    const int comboWidth = juce::jmax (90, (getWidth() - 2 * kSectionPadding) / 4);

    for (auto& row : matrix)
    {
        row.source->setBounds (kSectionPadding, y, comboWidth, kItemHeight);
        row.destination->setBounds (kSectionPadding + comboWidth + 6, y, comboWidth, kItemHeight);
        row.amount->setBounds (kSectionPadding + (comboWidth + 6) * 2, y,
                               juce::jmax (120, comboWidth * 2), kItemHeight);
        y += kItemHeight + kRowGap;
    }
}

void AdvancedPage::paint (juce::Graphics& g)
{
    g.fillAll (theme::background());

    columns = columnsFor (juce::jmax (200, getWidth()));
    const int itemWidth = juce::jmax (120, (getWidth() - kSectionPadding * 2) / columns);

    int y = 0;

    const auto drawSection = [&] (const juce::String& title, int contentHeight)
    {
        auto bounds = juce::Rectangle<int> (kSectionPadding / 2, y,
                                            juce::jmax (40, getWidth() - kSectionPadding),
                                            contentHeight).toFloat();

        g.setColour (theme::panel());
        g.fillRoundedRectangle (bounds, 10.0f);
        g.setColour (theme::outline());
        g.drawRoundedRectangle (bounds, 10.0f, 1.0f);

        g.setColour (theme::accentBright());
        g.setFont (theme::labelFont (11.0f));
        g.drawFittedText (title.toUpperCase(),
                          bounds.toNearestInt().reduced (kSectionPadding, 0).removeFromTop (kTitleHeight),
                          juce::Justification::centredLeft, 1);
    };

    for (const auto& group : groups)
    {
        y += kSectionPadding;

        const int rows = ((int) group.items.size() + columns - 1) / columns;
        const int contentHeight = kTitleHeight + rows * (kItemHeight + kRowGap) + kSectionPadding;

        drawSection (group.title, contentHeight);
        y += contentHeight + kSectionGap;
    }

    y += kSectionPadding;
    drawSection ("Mod matrix", kTitleHeight + kMatrixSlots * (kItemHeight + kRowGap) + kSectionPadding);

    juce::ignoreUnused (itemWidth);
}

} // namespace mk
