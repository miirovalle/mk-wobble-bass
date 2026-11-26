#include "UI/GraphControls.h"

#include "DSP/Lfo.h"

namespace mk
{
namespace
{

juce::RangedAudioParameter* rangedParam (juce::AudioProcessorValueTreeState& state, int index)
{
    return dynamic_cast<juce::RangedAudioParameter*> (state.getParameter (paramId (index)));
}

void writePlain (juce::AudioProcessorValueTreeState& state, int index, float plain)
{
    if (auto* parameter = rangedParam (state, index))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (plain));
}

} // namespace

//==============================================================================
ParamKnob::ParamKnob (juce::AudioProcessorValueTreeState& state, int paramIndex,
                      const juce::String& labelOverride)
    : labelText (labelOverride.isNotEmpty() ? labelOverride : juce::String (paramName (paramIndex))),
      index (paramIndex)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                juce::MathConstants<float>::pi * 2.75f, true);
    slider.setDoubleClickReturnValue (true, (double) paramMeta (paramIndex).def);
    addAndMakeVisible (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        state, paramId (paramIndex), slider);
}

ParamKnob::~ParamKnob()
{
    attachment.reset();
}

void ParamKnob::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    g.setColour (theme::textDim());
    g.setFont (theme::labelFont (12.0f));
    g.drawFittedText (labelText, bounds.removeFromTop (18), juce::Justification::centred, 1);

    char text[32] {};
    formatParamValue (index, (float) slider.getValue(), text, (int) sizeof (text));

    g.setColour (theme::text());
    g.setFont (theme::valueFont (13.0f));
    g.drawFittedText (text, bounds.removeFromBottom (20), juce::Justification::centred, 1);
}

void ParamKnob::resized()
{
    slider.setBounds (getLocalBounds().reduced (0, 20));
}

//==============================================================================
namespace
{

struct WobbleType
{
    const char* name;
    int paramIndex;
    float amount;
};

const std::array<WobbleType, 8>& wobbleTypes()
{
    static const std::array<WobbleType, 8> types { {
        { "FILTER",  (int) P::wobCut,     0.85f },
        { "RESO",    (int) P::wobReso,    0.55f },
        { "PITCH",   (int) P::wobPitch,   0.25f },
        { "WAVE",    (int) P::wobWt,      0.65f },
        { "DIRT",    (int) P::wobDrive,   0.65f },
        { "WIDTH",   (int) P::wobWidth,   0.65f },
        { "FORMANT", (int) P::wobFormant, 0.55f },
        { "FM",      (int) P::wobFm,      0.35f }
    } };

    return types;
}

} // namespace

int TypeKnob::typeCount() noexcept { return (int) wobbleTypes().size(); }

const char* TypeKnob::typeName (int index) noexcept
{
    const auto& types = wobbleTypes();
    return types[(size_t) juce::jlimit (0, (int) types.size() - 1, index)].name;
}

TypeKnob::TypeKnob (juce::AudioProcessorValueTreeState& state) : apvts (state)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                juce::MathConstants<float>::pi * 2.75f, true);
    slider.setRange (0.0, (double) (typeCount() - 1), 1.0);
    slider.setValue (0.0, juce::dontSendNotification);
    slider.onValueChange = [this] { applyType ((int) slider.getValue()); };
    addAndMakeVisible (slider);

    applyType (0);
}

void TypeKnob::applyType (int type)
{
    current = juce::jlimit (0, typeCount() - 1, type);
    const auto& types = wobbleTypes();

    for (int i = 0; i < (int) types.size(); ++i)
        writePlain (apvts, types[(size_t) i].paramIndex, i == current ? types[(size_t) i].amount : 0.0f);

    repaint();
}

void TypeKnob::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    g.setColour (theme::textDim());
    g.setFont (theme::labelFont (12.0f));
    g.drawFittedText ("WOBBLE TYPE", bounds.removeFromTop (18), juce::Justification::centred, 1);

    g.setColour (theme::accentBright());
    g.setFont (theme::valueFont (13.0f));
    g.drawFittedText (typeName (current), bounds.removeFromBottom (20), juce::Justification::centred, 1);
}

void TypeKnob::resized()
{
    slider.setBounds (getLocalBounds().reduced (0, 20));
}

//==============================================================================
DrawableLfo::DrawableLfo (juce::AudioProcessorValueTreeState& state, Engine& engineToUse)
    : apvts (state), engine (engineToUse)
{
    startTimerHz (20);
}

DrawableLfo::~DrawableLfo() { stopTimer(); }

juce::Rectangle<float> DrawableLfo::canvasArea() const
{
    auto bounds = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    bounds.removeFromTop (18.0f);      // title
    bounds.removeFromBottom (14.0f);   // hint
    return bounds.reduced (1.0f);
}

void DrawableLfo::timerCallback()
{
    if (! isVisible())
        return;

    phase = engine.wobblePhase();
    value = engine.wobbleValue();
    repaint();
}

void DrawableLfo::writePoint (juce::Point<float> position)
{
    const auto canvas = canvasArea();

    if (canvas.getWidth() < 1.0f || canvas.getHeight() < 1.0f)
        return;

    const float nx = juce::jlimit (0.0f, 1.0f, (position.x - canvas.getX()) / canvas.getWidth());
    const float ny = juce::jlimit (0.0f, 1.0f, (position.y - canvas.getY()) / canvas.getHeight());

    const int index = juce::jlimit (0, Engine::kWobblePoints - 1,
                                    (int) (nx * (float) Engine::kWobblePoints));
    engine.setWobblePoint (index, 1.0f - ny * 2.0f);

    // Drawing switches the engine to the custom shape.
    writePlain (apvts, (int) P::wobShape, (float) ShapeCustom);

    repaint();
}

void DrawableLfo::loadShape (int shape)
{
    for (int i = 0; i < Engine::kWobblePoints; ++i)
    {
        const float t = (float) i / (float) Engine::kWobblePoints;

        float v = 0.0f;
        switch (shape)
        {
            case ShapeSine:     v = std::sin (kTwoPi * t); break;
            case ShapeTriangle: v = 1.0f - 4.0f * std::fabs (t - 0.5f); break;
            case ShapeSawUp:    v = 2.0f * t - 1.0f; break;
            case ShapeSquare:   v = t < 0.5f ? 1.0f : -1.0f; break;
            case ShapeRamp:     v = t * t * 2.0f - 1.0f; break;
            default:            v = 0.0f; break;
        }

        engine.setWobblePoint (i, v);
    }

    writePlain (apvts, (int) P::wobShape, (float) ShapeCustom);
    repaint();
}

void DrawableLfo::mouseDown (const juce::MouseEvent& event)
{
    writePoint (event.position);
}

void DrawableLfo::mouseDrag (const juce::MouseEvent& event)
{
    writePoint (event.position);
}

void DrawableLfo::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced (2.0f);

    g.setColour (theme::panel());
    g.fillRoundedRectangle (area, 14.0f);
    g.setColour (theme::outline());
    g.drawRoundedRectangle (area, 14.0f, 1.0f);

    auto title = area.reduced (12.0f, 8.0f).removeFromTop (16.0f);
    g.setColour (theme::accentBright());
    g.setFont (theme::labelFont (12.0f));
    g.drawFittedText ("DRAW THE WOBBLE", title.toNearestInt(), juce::Justification::centredLeft, 1);

    g.setColour (theme::textDim());
    g.setFont (theme::valueFont (11.0f));
    g.drawFittedText ("drag with one finger", title.toNearestInt(), juce::Justification::centredRight, 1);

    const auto canvas = canvasArea();
    g.setColour (theme::panelLight().withAlpha (0.45f));
    g.fillRoundedRectangle (canvas, 8.0f);

    // Grid: centre line plus quarter lines.
    g.setColour (theme::outline().withAlpha (0.75f));
    g.drawHorizontalLine ((int) canvas.getCentreY(), canvas.getX(), canvas.getRight());

    for (int i = 1; i < 4; ++i)
    {
        const float x = canvas.getX() + canvas.getWidth() * (float) i / 4.0f;
        g.drawVerticalLine ((int) x, canvas.getY(), canvas.getBottom());
    }

    // The drawn curve.
    juce::Path curve;
    const int points = Engine::kWobblePoints;
    const float stepX = canvas.getWidth() / (float) points;

    for (int i = 0; i <= points; ++i)
    {
        const float v = engine.wobblePoint (i % points);
        const float x = canvas.getX() + stepX * (float) i;
        const float y = canvas.getCentreY() - v * canvas.getHeight() * 0.46f;

        if (i == 0)
            curve.startNewSubPath (x, y);
        else
            curve.lineTo (x, y);
    }

    g.setColour (theme::accent().withAlpha (0.22f));
    g.strokePath (curve, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved));

    g.setColour (theme::accentBright());
    g.strokePath (curve, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved));

    // Handles so the curve reads as something you can grab.
    for (int i = 0; i < points; i += 8)
    {
        const float v = engine.wobblePoint (i);
        const juce::Point<float> point (canvas.getX() + stepX * (float) i,
                                        canvas.getCentreY() - v * canvas.getHeight() * 0.46f);
        g.setColour (theme::text().withAlpha (0.55f));
        g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre (point));
    }

    // Playhead and live value.
    const float playX = canvas.getX() + canvas.getWidth() * phase;
    g.setColour (theme::accent().withAlpha (0.55f));
    g.drawVerticalLine ((int) playX, canvas.getY(), canvas.getBottom());

    const juce::Point<float> dot (playX, canvas.getCentreY() - value * canvas.getHeight() * 0.46f);
    g.setColour (theme::text());
    g.fillEllipse (juce::Rectangle<float> (11.0f, 11.0f).withCentre (dot));

    g.setColour (theme::outline());
    g.drawRoundedRectangle (canvas, 8.0f, 1.0f);
}

//==============================================================================
WaveScope::WaveScope (const Engine& engineToUse) : engine (engineToUse)
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (20);
}

WaveScope::~WaveScope() { stopTimer(); }

void WaveScope::timerCallback()
{
    if (! isVisible())
        return;

    engine.copyScope (samples.data(), (int) samples.size());
    peak = engine.outputPeak();
    repaint();
}

void WaveScope::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced (2.0f);

    g.setColour (theme::panel());
    g.fillRoundedRectangle (area, 12.0f);
    g.setColour (theme::outline());
    g.drawRoundedRectangle (area, 12.0f, 1.0f);

    auto inner = area.reduced (12.0f, 9.0f);
    auto header = inner.removeFromTop (14.0f);

    g.setColour (theme::textDim());
    g.setFont (theme::labelFont (11.0f));
    g.drawFittedText ("OUTPUT", header.toNearestInt(), juce::Justification::centredLeft, 1);

    g.setColour (peak > 0.97f ? theme::warning() : theme::accentBright());
    g.setFont (theme::valueFont (11.0f));
    g.drawFittedText (peak > 0.0001f ? juce::String (juce::Decibels::gainToDecibels (peak), 1) + " dB"
                                     : juce::String ("-inf"),
                      header.toNearestInt(), juce::Justification::centredRight, 1);

    const auto centreY = inner.getCentreY();
    g.setColour (theme::outline().withAlpha (0.7f));
    g.drawHorizontalLine ((int) centreY, inner.getX(), inner.getRight());

    if (inner.getWidth() > 4.0f)
    {
        juce::Path wave;
        const int columns = juce::jmax (1, (int) inner.getWidth());
        const int perColumn = juce::jmax (1, (int) samples.size() / columns);

        for (int c = 0; c < columns; ++c)
        {
            float lo = 0.0f, hi = 0.0f;
            const int start = c * perColumn;

            for (int i = 0; i < perColumn; ++i)
            {
                const float v = samples[(size_t) juce::jlimit (0, (int) samples.size() - 1, start + i)];
                lo = juce::jmin (lo, v);
                hi = juce::jmax (hi, v);
            }

            const float x = inner.getX() + (float) c + 0.5f;
            wave.startNewSubPath (x, centreY - hi * inner.getHeight() * 0.46f);
            wave.lineTo (x, centreY - lo * inner.getHeight() * 0.46f);
        }

        g.setColour (theme::accent().withAlpha (0.9f));
        g.strokePath (wave, juce::PathStrokeType (1.5f));
    }
}

//==============================================================================
LevelMeter::LevelMeter (const Engine& engineToUse) : engine (engineToUse)
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (24);
}

LevelMeter::~LevelMeter() { stopTimer(); }

void LevelMeter::timerCallback()
{
    if (! isVisible())
        return;

    const float target = engine.outputMeter();
    level = juce::jmax (target, level * 0.82f);
    peak = juce::jmax (target, peak * 0.94f);
    repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

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
