#include "UI/GatePattern.h"

namespace mk
{

GatePattern::GatePattern (Engine& engineToUse) : engine (engineToUse)
{
    startTimerHz (20);
}

GatePattern::~GatePattern() { stopTimer(); }

juce::Rectangle<int> GatePattern::gridArea() const
{
    auto bounds = getLocalBounds().reduced (10, 8);
    bounds.removeFromTop (16);      // title
    bounds.removeFromBottom (10);   // gain bar
    return bounds;
}

int GatePattern::stepAt (juce::Point<int> point) const
{
    const auto grid = gridArea();

    if (! grid.contains (point))
        return -1;

    const float width = (float) grid.getWidth() / (float) TranceGate::kSteps;
    return juce::jlimit (0, TranceGate::kSteps - 1, (int) ((float) (point.x - grid.getX()) / width));
}

void GatePattern::timerCallback()
{
    if (! isVisible())
        return;

    const int step = engine.tranceGate().currentStep();

    if (step != lastStep)
    {
        lastStep = step;
        repaint();
    }
}

void GatePattern::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced (2.0f);

    g.setColour (theme::panel());
    g.fillRoundedRectangle (area, 14.0f);
    g.setColour (theme::outline());
    g.drawRoundedRectangle (area, 14.0f, 1.0f);

    auto title = area.reduced (12.0f, 8.0f).removeFromTop (14.0f);
    g.setColour (theme::accentBright());
    g.setFont (theme::labelFont (12.0f));
    g.drawFittedText ("TRANCE GATE", title.toNearestInt(), juce::Justification::centredLeft, 1);

    g.setColour (theme::textDim());
    g.setFont (theme::valueFont (11.0f));
    g.drawFittedText ("tap or drag the steps", title.toNearestInt(),
                      juce::Justification::centredRight, 1);

    const auto grid = gridArea();
    const auto& gate = engine.tranceGate();
    const float cellWidth = (float) grid.getWidth() / (float) TranceGate::kSteps;
    const int playhead = gate.currentStep();

    for (int step = 0; step < TranceGate::kSteps; ++step)
    {
        auto cell = juce::Rectangle<float> (grid.getX() + (float) step * cellWidth + 2.0f,
                                            (float) grid.getY(),
                                            juce::jmax (4.0f, cellWidth - 4.0f),
                                            (float) grid.getHeight());

        const bool on = gate.stepIsOn (step);

        g.setColour (on ? theme::accentDeep() : theme::panelLight().withAlpha (0.55f));
        g.fillRoundedRectangle (cell, 6.0f);

        g.setColour (on ? theme::accentBright() : theme::outline());
        g.drawRoundedRectangle (cell, 6.0f, 1.0f);

        g.setColour (on ? theme::text() : theme::textDim());
        g.setFont (theme::labelFont (11.0f));
        g.drawFittedText (juce::String (step + 1), cell.toNearestInt(),
                          juce::Justification::centred, 1);

        if (step == playhead)
        {
            g.setColour (theme::text());
            g.drawRoundedRectangle (cell.reduced (1.0f), 6.0f, 2.0f);
        }
    }

    // Live gain: shows the smoothing working between the steps.
    auto bar = getLocalBounds().toFloat().reduced (12.0f, 8.0f).removeFromBottom (4.0f);
    g.setColour (theme::panelLight());
    g.fillRoundedRectangle (bar, 2.0f);

    const float gain = juce::jlimit (0.0f, 1.0f, gate.currentEnvelope());
    g.setColour (theme::accent());
    g.fillRoundedRectangle (bar.withWidth (juce::jmax (2.0f, bar.getWidth() * gain)), 2.0f);
}

void GatePattern::mouseDown (const juce::MouseEvent& event)
{
    const int step = stepAt (event.getPosition());

    if (step < 0)
        return;

    dragging = true;
    paintValue = engine.tranceGate().stepIsOn (step) ? 0 : 1;
    engine.tranceGate().setStep (step, paintValue != 0);
    repaint();
}

void GatePattern::mouseDrag (const juce::MouseEvent& event)
{
    if (! dragging)
        return;

    const int step = stepAt (event.getPosition());

    if (step >= 0 && engine.tranceGate().stepIsOn (step) != (paintValue != 0))
    {
        engine.tranceGate().setStep (step, paintValue != 0);
        repaint();
    }
}

void GatePattern::mouseUp (const juce::MouseEvent&)
{
    dragging = false;
}

} // namespace mk
