#include "UI/PianoRoll.h"

namespace mk
{
namespace
{

constexpr int kHeaderHeight = 30;
constexpr int kLabelHeight = 14;
constexpr int kKeyWidth = 38;

const char* noteName (int midiNote)
{
    static const char* const names[] = { "C", "C#", "D", "D#", "E", "F",
                                         "F#", "G", "G#", "A", "A#", "B" };
    return names[((midiNote % 12) + 12) % 12];
}

bool isBlackKey (int midiNote) noexcept
{
    const int pc = ((midiNote % 12) + 12) % 12;
    return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
}

} // namespace

PianoRoll::PianoRoll (Sequencer& sequencerToUse) : sequencer (sequencerToUse)
{
    startTimerHz (20);
}

PianoRoll::~PianoRoll() { stopTimer(); }

//==============================================================================
juce::Rectangle<int> PianoRoll::headerArea() const
{
    return getLocalBounds().reduced (2).removeFromTop (kHeaderHeight);
}

juce::Rectangle<int> PianoRoll::stepLabelArea() const
{
    auto bounds = getLocalBounds().reduced (2);
    bounds.removeFromTop (kHeaderHeight);
    bounds.removeFromLeft (kKeyWidth);
    return bounds.removeFromTop (kLabelHeight);
}

juce::Rectangle<int> PianoRoll::keyArea() const
{
    auto bounds = getLocalBounds().reduced (2);
    bounds.removeFromTop (kHeaderHeight + kLabelHeight);
    return bounds.removeFromLeft (kKeyWidth);
}

juce::Rectangle<int> PianoRoll::gridArea() const
{
    auto bounds = getLocalBounds().reduced (2);
    bounds.removeFromTop (kHeaderHeight + kLabelHeight);
    bounds.removeFromLeft (kKeyWidth);
    return bounds;
}

juce::Rectangle<int> PianoRoll::playArea() const
{
    return headerArea().removeFromLeft (54).reduced (0, 3);
}

juce::Rectangle<int> PianoRoll::bpmArea() const
{
    auto header = headerArea();
    header.removeFromLeft (60);
    return header.removeFromLeft (74).reduced (0, 3);
}

juce::Rectangle<int> PianoRoll::divisionArea() const
{
    auto header = headerArea();
    header.removeFromLeft (140);
    return header.removeFromLeft (50).reduced (0, 3);
}

juce::Rectangle<int> PianoRoll::octaveDownArea() const
{
    auto header = headerArea();
    header.removeFromLeft (196);
    return header.removeFromLeft (40).reduced (0, 3);
}

juce::Rectangle<int> PianoRoll::octaveUpArea() const
{
    auto header = headerArea();
    header.removeFromLeft (240);
    return header.removeFromLeft (40).reduced (0, 3);
}

juce::Rectangle<int> PianoRoll::clearArea() const
{
    return headerArea().removeFromRight (54).reduced (0, 3);
}

juce::Rectangle<int> PianoRoll::columnRect (int step) const
{
    const auto grid = gridArea();
    const float width = (float) grid.getWidth() / (float) Sequencer::kSteps;
    const int x = grid.getX() + (int) ((float) step * width);
    const int nextX = grid.getX() + (int) ((float) (step + 1) * width);
    return { x, grid.getY(), juce::jmax (1, nextX - x), grid.getHeight() };
}

int PianoRoll::stepAt (juce::Point<int> point) const
{
    const auto grid = gridArea();

    if (! grid.contains (point))
        return -1;

    const float width = (float) grid.getWidth() / (float) Sequencer::kSteps;
    return juce::jlimit (0, Sequencer::kSteps - 1, (int) ((float) (point.x - grid.getX()) / width));
}

int PianoRoll::rowAt (juce::Point<int> point) const
{
    const auto grid = gridArea();

    if (! grid.contains (point))
        return -1;

    const float height = (float) grid.getHeight() / (float) Sequencer::kRows;
    return juce::jlimit (0, Sequencer::kRows - 1, (int) ((float) (point.y - grid.getY()) / height));
}

//==============================================================================
void PianoRoll::timerCallback()
{
    if (! isVisible())
        return;

    const int step = sequencer.isPlaying() ? sequencer.currentStep() : -1;

    if (step == lastDisplayedStep)
        return;

    if (lastDisplayedStep >= 0)
        repaint (columnRect (lastDisplayedStep));

    if (step >= 0)
        repaint (columnRect (step));

    lastDisplayedStep = step;
    repaint (headerArea());
}

void PianoRoll::drawChip (juce::Graphics& g, juce::Rectangle<int> area,
                          const juce::String& text, bool lit) const
{
    g.setColour (lit ? theme::accentDeep() : theme::panelLight());
    g.fillRoundedRectangle (area.toFloat(), 6.0f);
    g.setColour (lit ? theme::accentBright() : theme::outline());
    g.drawRoundedRectangle (area.toFloat(), 6.0f, 1.0f);

    g.setColour (lit ? theme::text() : theme::textDim());
    g.setFont (theme::labelFont (11.0f));
    g.drawFittedText (text, area, juce::Justification::centred, 1);
}

void PianoRoll::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (theme::panel());
    g.fillRoundedRectangle (bounds, 12.0f);
    g.setColour (theme::outline());
    g.drawRoundedRectangle (bounds, 12.0f, 1.0f);

    // Header: transport, tempo, division, octave, clear.
    const bool playing = sequencer.isPlaying();
    drawChip (g, playArea(), playing ? "STOP" : "PLAY", playing);
    drawChip (g, bpmArea(), juce::String (juce::roundToInt (sequencer.bpm())) + " BPM", false);
    drawChip (g, divisionArea(), Sequencer::divisionName (sequencer.division()), false);
    drawChip (g, octaveDownArea(), "OCT-", false);
    drawChip (g, octaveUpArea(), "OCT+", false);
    drawChip (g, clearArea(), "CLEAR", false);

    const auto grid = gridArea();
    const auto keys = keyArea();
    const auto labels = stepLabelArea();

    if (grid.getWidth() <= 0 || grid.getHeight() <= 0)
        return;

    const float columnWidth = (float) grid.getWidth() / (float) Sequencer::kSteps;
    const float rowHeight = (float) grid.getHeight() / (float) Sequencer::kRows;
    const int currentStep = playing ? sequencer.currentStep() : -1;

    // Step numbers.
    g.setFont (theme::valueFont (9.0f));
    for (int step = 0; step < Sequencer::kSteps; ++step)
    {
        const bool beat = step % 4 == 0;
        g.setColour (beat ? theme::accentBright() : theme::textDim());
        g.drawFittedText (juce::String (step + 1),
                          juce::Rectangle<int> (labels.getX() + (int) ((float) step * columnWidth),
                                                labels.getY(), (int) columnWidth, labels.getHeight()),
                          juce::Justification::centred, 1);
    }

    // Key strip + grid rows.
    for (int row = 0; row < Sequencer::kRows; ++row)
    {
        const int note = Sequencer::rowToNote (row);
        const int y = grid.getY() + (int) ((float) row * rowHeight);
        const int height = juce::jmax (1, (int) ((float) (row + 1) * rowHeight) - (int) ((float) row * rowHeight));
        const bool black = isBlackKey (note);

        g.setColour (black ? theme::panelLight().withAlpha (0.9f)
                           : theme::panelLight().withAlpha (0.30f));
        g.fillRect (keys.getX(), y, keys.getWidth(), height);
        g.fillRect (grid.getX(), y, grid.getWidth(), height);

        g.setColour (theme::outline().withAlpha (0.5f));
        g.drawHorizontalLine (y + height - 1, (float) keys.getX(), (float) grid.getRight());

        if (note % 12 == 0)
        {
            g.setColour (theme::textDim());
            g.setFont (theme::labelFont (9.0f));
            g.drawFittedText (juce::String (noteName (note)) + juce::String (note / 12 - 2),
                              juce::Rectangle<int> (keys.getX() + 3, y, keys.getWidth() - 5, height),
                              juce::Justification::centredLeft, 1);
        }
    }

    // Notes.
    for (int step = 0; step < Sequencer::kSteps; ++step)
    {
        const uint32_t mask = sequencer.stepWord (step);

        for (int row = 0; row < Sequencer::kRows; ++row)
        {
            if ((mask & (1u << row)) == 0)
                continue;

            auto cell = juce::Rectangle<float> (grid.getX() + (float) step * columnWidth + 1.5f,
                                                grid.getY() + (float) row * rowHeight + 1.0f,
                                                juce::jmax (2.0f, columnWidth - 3.0f),
                                                juce::jmax (2.0f, rowHeight - 2.0f));

            g.setColour (theme::accent());
            g.fillRoundedRectangle (cell, 2.5f);
            g.setColour (theme::accentBright());
            g.drawRoundedRectangle (cell, 2.5f, 1.0f);
        }
    }

    // Beat lines.
    g.setColour (theme::outline().withAlpha (0.8f));
    for (int step = 0; step <= Sequencer::kSteps; step += 4)
    {
        const float x = grid.getX() + (float) step * columnWidth;
        g.drawVerticalLine ((int) x, (float) grid.getY(), (float) grid.getBottom());
    }

    // Playhead column.
    if (currentStep >= 0)
    {
        g.setColour (theme::accentBright().withAlpha (0.14f));
        g.fillRect (columnRect (currentStep));
    }
}

//==============================================================================
void PianoRoll::mouseDown (const juce::MouseEvent& event)
{
    const auto point = event.getPosition();

    if (playArea().contains (point))
    {
        sequencer.setPlaying (! sequencer.isPlaying());
        repaint (headerArea());
        return;
    }

    if (bpmArea().contains (point))
    {
        draggingBpm = true;
        bpmAtDragStart = sequencer.bpm();
        dragStartX = point.x;
        return;
    }

    if (divisionArea().contains (point))
    {
        sequencer.setDivision ((sequencer.division() + 1) % 3);
        repaint (headerArea());
        return;
    }

    if (octaveDownArea().contains (point))
    {
        sequencer.transposeOctaves (-1);
        repaint();
        return;
    }

    if (octaveUpArea().contains (point))
    {
        sequencer.transposeOctaves (1);
        repaint();
        return;
    }

    if (clearArea().contains (point))
    {
        sequencer.clear();
        repaint();
        return;
    }

    const int step = stepAt (point);
    const int row = rowAt (point);

    if (step >= 0 && row >= 0)
    {
        draggingGrid = true;
        paintValue = sequencer.isSet (step, row) ? 0 : 1;
        sequencer.set (step, row, paintValue != 0);
        repaint();
    }
}

void PianoRoll::mouseDrag (const juce::MouseEvent& event)
{
    const auto point = event.getPosition();

    if (draggingBpm)
    {
        const double delta = (double) (point.x - dragStartX);
        sequencer.setBpm (bpmAtDragStart + delta);
        repaint (headerArea());
        return;
    }

    if (! draggingGrid)
        return;

    const int step = stepAt (point);
    const int row = rowAt (point);

    if (step >= 0 && row >= 0 && sequencer.isSet (step, row) != (paintValue != 0))
    {
        sequencer.set (step, row, paintValue != 0);
        repaint();
    }
}

void PianoRoll::mouseUp (const juce::MouseEvent&)
{
    draggingGrid = false;
    draggingBpm = false;
}

} // namespace mk
