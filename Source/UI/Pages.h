#pragma once

// The ADVANCED drawer: every engine parameter, grouped, as slim bars and
// drop-downs. No rotary controls and no hidden parameters - it is the whole
// synth with the same controls as the front page, just smaller.

#include "DSP/Params.h"
#include "UI/GraphControls.h"
#include "UI/Theme.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace mk
{

class AdvancedPage final : public juce::Component
{
public:
    explicit AdvancedPage (juce::AudioProcessorValueTreeState& state);

    void paint (juce::Graphics&) override;
    void resized() override;

    int preferredHeight (int width) const;

private:
    struct GroupView
    {
        juce::String title;
        std::vector<std::unique_ptr<juce::Component>> items;
    };

    struct MatrixRow
    {
        std::unique_ptr<ParamCombo> source;
        std::unique_ptr<ParamCombo> destination;
        std::unique_ptr<ParamSlider> amount;
    };

    static int columnsFor (int width);

    std::vector<GroupView> groups;
    std::array<MatrixRow, (size_t) kMatrixSlots> matrix;
    int columns = 3;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdvancedPage)
};

} // namespace mk
