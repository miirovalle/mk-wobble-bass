#include "UI/WobbleDisplay.h"

#include "DSP/Lfo.h"

namespace mk
{

WobbleDisplay::WobbleDisplay (const Engine& engineToUse) : engine (engineToUse)
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (24);
}

WobbleDisplay::~WobbleDisplay() { stopTimer(); }

void WobbleDisplay::timerCallback()
{
    repaint();
}

void WobbleDisplay::refreshData()
{
    engine.copyWobbleTrace (wobbleTrace.data(), (int) wobbleTrace.size());
    engine.copyScope (scope.data(), (int) scope.size());
    phase = engine.wobblePhase();
    value = engine.wobbleValue();
    rate = engine.wobbleRateHz();
    peak = engine.outputPeak();
}

void WobbleDisplay::paint (juce::Graphics& g)
{
    refreshData();
    auto bounds = getLocalBounds().toFloat();
    const auto titleArea = bounds.removeFromTop (20.0f);

    g.setColour (theme::accentBright());
    g.setFont (theme::labelFont (12.0f));
    g.drawFittedText ("WOBBLE", titleArea.toNearestInt(), juce::Justification::centredLeft, 1);

    g.setColour (theme::textDim());
    g.setFont (theme::valueFont (12.0f));
    const juce::String rateText = rate >= 1.0f ? juce::String (rate, 2) + " Hz"
                                              : juce::String (rate * 1000.0f, 0) + " mHz";
    g.drawFittedText (rateText, titleArea.toNearestInt(), juce::Justification::centredRight, 1);

    const auto area = bounds.reduced (2.0f);
    g.setColour (theme::panelLight().withAlpha (0.55f));
    g.fillRoundedRectangle (area, 8.0f);
    g.setColour (theme::outline());
    g.drawRoundedRectangle (area, 8.0f, 1.0f);

    const auto centreY = area.getCentreY();
    g.setColour (theme::outline().withAlpha (0.6f));
    g.drawHorizontalLine ((int) centreY, area.getX() + 6.0f, area.getRight() - 6.0f);

    // Two cycles of the current shape, offset by the live phase so the display
    // scrolls as the wobble advances.
    const int shape = engine.wobbleShapeForDisplay();
    juce::Path shapePath;
    const int steps = juce::jmax (64, (int) area.getWidth() / 2);
    for (int i = 0; i <= steps; ++i)
    {
        const float t = (float) i / (float) steps;
        const float wobbleValue = shapeValue (shape, phase + t * 2.0f, 0.6f);
        const float x = area.getX() + 6.0f + t * (area.getWidth() - 12.0f);
        const float y = centreY - wobbleValue * area.getHeight() * 0.34f;
        if (i == 0)
            shapePath.startNewSubPath (x, y);
        else
            shapePath.lineTo (x, y);
    }

    g.setColour (theme::accent().withAlpha (0.30f));
    g.strokePath (shapePath, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved));
    g.setColour (theme::accentBright());
    g.strokePath (shapePath, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved));

    // Depth envelope hint: how far the modulation is allowed to travel.
    const float depth = juce::jlimit (0.0f, 1.0f, engine.wobbleDepthForDisplay());
    g.setColour (theme::accentDeep().withAlpha (0.35f));
    const float depthPixels = area.getHeight() * 0.34f * depth;
    g.drawHorizontalLine ((int) (centreY - depthPixels), area.getX() + 8.0f, area.getRight() - 8.0f);
    g.drawHorizontalLine ((int) (centreY + depthPixels), area.getX() + 8.0f, area.getRight() - 8.0f);

    // Live position marker.
    const float markerX = area.getX() + 6.0f + 6.0f;
    const float markerY = centreY - value * area.getHeight() * 0.34f;
    g.setColour (theme::text());
    g.fillEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ markerX, markerY }));

    // Output scope in the lower half.
    auto scopeArea = area.withHeight (area.getHeight() * 0.34f)
                         .withY (area.getBottom() - area.getHeight() * 0.34f - 4.0f)
                         .reduced (8.0f, 0.0f);
    juce::Path scopePath;
    for (size_t i = 0; i < scope.size(); ++i)
    {
        const float t = (float) i / (float) (scope.size() - 1);
        const float x = scopeArea.getX() + t * scopeArea.getWidth();
        const float y = scopeArea.getCentreY() - scope[i] * scopeArea.getHeight() * 0.5f;
        if (i == 0)
            scopePath.startNewSubPath (x, y);
        else
            scopePath.lineTo (x, y);
    }
    g.setColour (theme::accentDeep().withAlpha (0.85f));
    g.strokePath (scopePath, juce::PathStrokeType (1.2f));

    // Peak indicator strip.
    const float peakLevel = juce::jlimit (0.0f, 1.0f, peak);
    g.setColour (peakLevel > 0.94f ? theme::warning() : theme::meter());
    g.fillRect (area.getX() + 6.0f, area.getY() + 3.0f,
                (area.getWidth() - 12.0f) * peakLevel, 2.0f);
}

//==============================================================================
LevelMeter::LevelMeter (const Engine& engineToUse) : engine (engineToUse)
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (24);
}

void LevelMeter::timerCallback()
{
    repaint();
}

void LevelMeter::refreshData()
{
    const float target = engine.outputMeter();
    level = juce::jmax (target, level * 0.82f);
    peak = juce::jmax (target, peak * 0.94f);
}

void LevelMeter::paint (juce::Graphics& g)
{
    refreshData();
    auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0d0a14));
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (theme::outline());
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    const float fill = juce::jlimit (0.0f, 1.0f, level * 1.8f);
    if (fill > 0.001f)
    {
        g.setColour (level > 0.6f ? theme::warning() : theme::accent());
        g.fillRoundedRectangle (bounds.reduced (2.0f).withWidth ((bounds.getWidth() - 4.0f) * fill), 3.0f);
    }

    const float peakX = bounds.getX() + 2.0f
                      + (bounds.getWidth() - 4.0f) * juce::jlimit (0.0f, 1.0f, peak * 1.8f);
    g.setColour (theme::text().withAlpha (0.75f));
    g.fillRect (peakX - 1.0f, bounds.getY() + 2.0f, 2.0f, bounds.getHeight() - 4.0f);
}

} // namespace mk
