#pragma once

// On-screen piano roll: 16 steps x 25 notes, played by the built-in sequencer.
// Cells are tapped or painted with a finger; the playhead repaints only the two
// columns that change, so it stays smooth on an iPad.

#include "DSP/Engine.h"
#include "DSP/Sequencer.h"
#include "UI/Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace mk
{

class PianoRoll final : public juce::Component, private juce::Timer
{
public:
    explicit PianoRoll (Sequencer&);
    ~PianoRoll() override;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    juce::Rectangle<int> headerArea() const;
    juce::Rectangle<int> gridArea() const;
    juce::Rectangle<int> keyArea() const;
    juce::Rectangle<int> stepLabelArea() const;
    juce::Rectangle<int> playArea() const;
    juce::Rectangle<int> bpmArea() const;
    juce::Rectangle<int> divisionArea() const;
    juce::Rectangle<int> octaveDownArea() const;
    juce::Rectangle<int> octaveUpArea() const;
    juce::Rectangle<int> clearArea() const;

    juce::Rectangle<int> columnRect (int step) const;
    int stepAt (juce::Point<int>) const;
    int rowAt (juce::Point<int>) const;

    void drawChip (juce::Graphics&, juce::Rectangle<int>, const juce::String& text, bool lit) const;

    Sequencer& sequencer;
    int lastDisplayedStep = -1;
    int paintValue = 1;          // gesture value while dragging
    bool draggingGrid = false;
    bool draggingBpm = false;
    double bpmAtDragStart = 120.0;
    int dragStartX = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRoll)
};

} // namespace mk
