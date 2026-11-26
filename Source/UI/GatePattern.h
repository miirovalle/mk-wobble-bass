#pragma once

// Sixteen tap cells for the trance gate, with the live playhead and gain.

#include "DSP/Engine.h"
#include "UI/Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace mk
{

class GatePattern final : public juce::Component, private juce::Timer
{
public:
    explicit GatePattern (Engine&);
    ~GatePattern() override;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    juce::Rectangle<int> gridArea() const;
    int stepAt (juce::Point<int>) const;

    Engine& engine;
    int lastStep = -1;
    int paintValue = 1;
    bool dragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GatePattern)
};

} // namespace mk
