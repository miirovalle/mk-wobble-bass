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

float normalisedOf (juce::RangedAudioParameter* parameter)
{
    return parameter != nullptr ? juce::jlimit (0.0f, 1.0f, parameter->getValue()) : 0.0f;
}

float plainOf (juce::RangedAudioParameter* parameter)
{
    return parameter != nullptr ? parameter->convertFrom0to1 (parameter->getValue()) : 0.0f;
}

// Writes a normalised position, optionally snapping to whole parameter steps
// (sync divisions and shape indexes are integers).
void writeNormalised (juce::RangedAudioParameter* parameter, float normalised, bool snap = false)
{
    if (parameter == nullptr)
        return;

    normalised = juce::jlimit (0.0f, 1.0f, normalised);

    if (snap)
        normalised = parameter->convertTo0to1 (std::round (parameter->convertFrom0to1 (normalised)));

    parameter->setValueNotifyingHost (normalised);
}

void writePlain (juce::RangedAudioParameter* parameter, float plain)
{
    if (parameter != nullptr)
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (plain));
}

juce::String rateText (float hz)
{
    return hz >= 1.0f ? juce::String (hz, 2) + " Hz" : juce::String (hz * 1000.0f, 0) + " mHz";
}

// Wobble shapes 0..9 mapped onto the LFO picture shapes 0..5.
int wobbleShapeToPicture (int shape) noexcept
{
    switch (shape)
    {
        case ShapeSine:      return LfoSine;
        case ShapeTriangle:  return LfoTriangle;
        case ShapeSawUp:     return LfoSawUp;
        case ShapeSawDown:   return LfoSawDown;
        case ShapeSquare:
        case ShapePulse:     return LfoSquare;
        case ShapeRamp:      return LfoSawUp;
        case ShapeRandom:
        case ShapeSampleHold: return LfoRandom;
        default:             return LfoTriangle;
    }
}

// A 34 x 14 live picture for a parameter, keyed by the engine group so every
// control shows what it is doing instead of being a bare bar.
void drawParamGlyph (juce::Graphics& g, juce::Rectangle<float> area, int paramIndex,
                     float normalised, bool small)
{
    if (area.getWidth() < 8.0f || area.getHeight() < 6.0f)
        return;

    const auto group = paramMeta (paramIndex).group;
    const float value = juce::jlimit (0.0f, 1.0f, normalised);
    const auto centreY = area.getCentreY();
    const float amplitude = area.getHeight() * 0.40f;
    auto colour = theme::accentBright().withAlpha (0.85f);

    juce::Path path;

    switch (group)
    {
        case Group::Filt:
        {
            // Low-pass knee that follows the control, with a resonance bump.
            const float knee = juce::jmap (value, 0.15f, 0.85f);
            path.startNewSubPath (area.getX(), centreY - amplitude * 0.55f);
            path.lineTo (area.getX() + area.getWidth() * knee, centreY - amplitude * 0.55f);
            path.quadraticTo (area.getX() + area.getWidth() * (knee + 0.18f), centreY - amplitude * 0.7f,
                              area.getRight(), area.getBottom());
            break;
        }

        case Group::Env:
        {
            // ADSR: attack, decay to sustain, release.
            path.startNewSubPath (area.getX(), area.getBottom());
            path.lineTo (area.getX() + area.getWidth() * 0.22f, area.getY());
            path.lineTo (area.getX() + area.getWidth() * 0.45f, centreY);
            path.lineTo (area.getX() + area.getWidth() * 0.72f, centreY);
            path.lineTo (area.getRight(), area.getBottom());
            colour = theme::accentBright();
            break;
        }

        case Group::Dist:
        {
            // Wave folder / clipper: harder fold as the drive rises.
            const float drive = 0.5f + value * 3.5f;
            for (int i = 0; i <= 24; ++i)
            {
                const float t = (float) i / 24.0f;
                const float in = t * 2.0f - 1.0f;
                const float out = juce::jlimit (-1.0f, 1.0f, std::tanh (in * drive));
                const float x = area.getX() + t * area.getWidth();
                const float y = centreY - out * area.getHeight() * 0.42f;

                if (i == 0)
                    path.startNewSubPath (x, y);
                else
                    path.lineTo (x, y);
            }
            break;
        }

        case Group::Fx:
        {
            // Echo tail: repeat count follows the value.
            const int repeats = juce::jlimit (1, 5, (int) (value * 5.0f) + 1);
            const float step = area.getWidth() / (float) (repeats + 1);
            for (int i = 0; i < repeats; ++i)
            {
                const float x = area.getX() + step * (float) (i + 0.5f);
                const float h = amplitude * (1.0f - (float) i / (float) (repeats + 1));
                path.startNewSubPath (x, centreY + h);
                path.lineTo (x, centreY - h);
            }
            break;
        }

        case Group::Lfo:
        case Group::Wob:
        {
            // Free-running wave: cycle count and size follow the control.
            const float cycles = juce::jmap (value, 0.6f, 3.0f);
            const int steps = juce::jmax (12, (int) area.getWidth());

            for (int i = 0; i <= steps; ++i)
            {
                const float t = (float) i / (float) steps;
                const float v = std::sin (kTwoPi * t * cycles);
                const float x = area.getX() + t * area.getWidth();
                const float y = centreY - v * amplitude;

                if (i == 0)
                    path.startNewSubPath (x, y);
                else
                    path.lineTo (x, y);
            }
            break;
        }

        case Group::Osc1:
        case Group::Osc2:
        case Group::Osc3:
        case Group::Sub:
        case Group::Fm:
        {
            // One cycle whose shape follows the wave choice, amplitude the level.
            const float amp = amplitude * juce::jmap (value, 0.35f, 1.0f);
            const int steps = juce::jmax (10, (int) area.getWidth());

            for (int i = 0; i <= steps; ++i)
            {
                const float t = (float) i / (float) steps;
                const float v = (t < 0.5f ? 1.0f : -1.0f) * (0.35f + 0.65f * (t < 0.5f ? t * 2.0f : (1.0f - t) * 2.0f));
                const float x = area.getX() + t * area.getWidth();

                if (i == 0)
                    path.startNewSubPath (x, centreY - v * amp);
                else
                    path.lineTo (x, centreY - v * amp);
            }
            break;
        }

        default:
        {
            // Macros and global params: a filled wedge that grows with the value.
            path.startNewSubPath (area.getX(), area.getBottom());
            path.lineTo (area.getX(), area.getY() + (1.0f - value) * area.getHeight());
            path.lineTo (area.getX() + area.getWidth() * value, area.getBottom());
            path.closeSubPath();
            g.setColour (colour.withAlpha (0.28f));
            g.fillPath (path);
            break;
        }
    }

    g.setColour (colour);
    g.strokePath (path, juce::PathStrokeType (small ? 1.1f : 1.4f));
}

} // namespace

//==============================================================================
float lfoShapeValue (int shape, float phase01) noexcept
{
    const float p = phase01 - std::floor (phase01);

    switch (shape)
    {
        case LfoSine:      return std::sin (kTwoPi * p);
        case LfoTriangle:  return 1.0f - 4.0f * std::fabs (p - 0.5f);
        case LfoSawDown:   return 1.0f - 2.0f * p;
        case LfoSawUp:     return 2.0f * p - 1.0f;
        case LfoSquare:    return p < 0.5f ? 1.0f : -1.0f;
        case LfoRandom:
        {
            const int segment = (int) (p * 8.0f);
            uint32_t h = (uint32_t) (segment + 1) * 2654435761u;
            h ^= h >> 13;
            return (float) (h & 0xffffu) / 32768.0f - 1.0f;
        }
        default:           return 0.0f;
    }
}

void drawWavePicture (juce::Graphics& g, juce::Rectangle<float> area, int shape,
                      float cycles, float depth, juce::Colour colour)
{
    if (area.getWidth() < 4.0f || area.getHeight() < 4.0f)
        return;

    const auto centreY = area.getCentreY();
    const float amplitude = area.getHeight() * 0.42f * juce::jlimit (0.0f, 1.0f, depth);
    const int steps = juce::jmax (24, (int) area.getWidth());

    juce::Path path;
    for (int i = 0; i <= steps; ++i)
    {
        const float t = (float) i / (float) steps;
        const float v = lfoShapeValue (shape, t * cycles);
        const float x = area.getX() + t * area.getWidth();
        const float y = centreY - v * amplitude;

        if (i == 0)
            path.startNewSubPath (x, y);
        else
            path.lineTo (x, y);
    }

    g.setColour (colour.withAlpha (0.22f));
    g.strokePath (path, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved));
    g.setColour (colour);
    g.strokePath (path, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved));
}

//==============================================================================
ParamSlider::ParamSlider (juce::AudioProcessorValueTreeState& state, int paramIndex,
                          const juce::String& labelOverride)
    : parameter (dynamic_cast<juce::RangedAudioParameter*> (state.getParameter (paramId (paramIndex)))),
      labelText (labelOverride.isNotEmpty() ? labelOverride : juce::String (paramName (paramIndex))),
      index (paramIndex)
{
    if (parameter != nullptr)
    {
        attachment = std::make_unique<juce::ParameterAttachment> (
            *parameter, [this] (float value)
            {
                plainValue = value;
                normalised = parameter->convertTo0to1 (value);
                repaint();
            });

        attachment->sendInitialUpdate();
    }
}

ParamSlider::~ParamSlider() = default;

void ParamSlider::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    auto labelRow = bounds.removeFromTop (13);

    g.setColour (theme::textDim());
    g.setFont (theme::labelFont (10.5f));
    g.drawFittedText (labelText, labelRow, juce::Justification::centredLeft, 1);

    // Every slider carries a live picture of what it does - no bare controls.
    drawParamGlyph (g, juce::Rectangle<float> ((float) labelRow.getRight() - 34.0f,
                                               (float) labelRow.getY(), 34.0f, (float) labelRow.getHeight()),
                    index, normalised, true);

    auto bar = bounds.reduced (0, 1).toFloat();
    g.setColour (theme::panelLight());
    g.fillRoundedRectangle (bar, 4.0f);

    const float filled = bar.getWidth() * juce::jlimit (0.0f, 1.0f, normalised);
    if (filled > 2.0f)
    {
        g.setColour (theme::accentDeep());
        g.fillRoundedRectangle (bar.withWidth (filled), 4.0f);
    }

    char text[40] {};
    formatParamValue (index, plainValue, text, (int) sizeof (text));

    g.setColour (theme::text());
    g.setFont (theme::valueFont (11.0f));
    g.drawFittedText (text, bar.toNearestInt().reduced (6, 0), juce::Justification::centredRight, 1);
}

void ParamSlider::applyDrag (juce::Point<float> position)
{
    auto bar = getLocalBounds().toFloat().reduced (0.0f, 14.0f);

    if (bar.getWidth() < 1.0f || parameter == nullptr || attachment == nullptr)
        return;

    const float nx = juce::jlimit (0.0f, 1.0f, (position.x - bar.getX()) / bar.getWidth());
    attachment->setValueAsPartOfGesture (parameter->convertFrom0to1 (nx));
}

void ParamSlider::mouseDown (const juce::MouseEvent& event)
{
    if (attachment != nullptr)
        attachment->beginGesture();

    applyDrag (event.position);
}

void ParamSlider::mouseDrag (const juce::MouseEvent& event)
{
    applyDrag (event.position);
}

void ParamSlider::mouseUp (const juce::MouseEvent&)
{
    if (attachment != nullptr)
        attachment->endGesture();
}

void ParamSlider::mouseDoubleClick (const juce::MouseEvent&)
{
    if (parameter != nullptr && attachment != nullptr)
        attachment->setValueAsCompleteGesture (parameter->convertFrom0to1 (parameter->getDefaultValue()));
}

//==============================================================================
ParamCombo::ParamCombo (juce::AudioProcessorValueTreeState& state, int paramIndex,
                        const juce::String& labelOverride)
    : apvts (state), index (paramIndex)
{
    labelText = labelOverride.isNotEmpty() ? labelOverride : juce::String (paramName (paramIndex));

    int count = 0;
    if (const auto* choices = paramChoices (paramIndex, count))
        for (int i = 0; i < count; ++i)
            combo.addItem (choices[i], i + 1);

    combo.setJustificationType (juce::Justification::centredLeft);
    combo.setTooltip (labelText);
    addAndMakeVisible (combo);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, paramId (paramIndex), combo);
}

ParamCombo::~ParamCombo() = default;

void ParamCombo::paint (juce::Graphics& g)
{
    g.setColour (theme::textDim());
    g.setFont (theme::labelFont (10.5f));
    g.drawFittedText (labelText, getLocalBounds().removeFromTop (13), juce::Justification::centredLeft, 1);
}

void ParamCombo::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (13);
    combo.setBounds (bounds.reduced (0, 2));
}

//==============================================================================
Section::Section (juce::String panelTitle) : title (std::move (panelTitle))
{
    setInterceptsMouseClicks (false, true);
}

void Section::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (0.5f);

    g.setColour (theme::panel());
    g.fillRoundedRectangle (bounds, 12.0f);

    juce::ColourGradient glow (theme::accent().withAlpha (0.10f), bounds.getTopLeft(),
                               theme::panel(), bounds.getBottomLeft(), false);
    g.setGradientFill (glow);
    g.fillRoundedRectangle (bounds, 12.0f);

    g.setColour (theme::outline());
    g.drawRoundedRectangle (bounds, 12.0f, 1.0f);

    if (title.isNotEmpty())
    {
        g.setColour (theme::accentBright());
        g.setFont (theme::labelFont (11.5f));
        g.drawFittedText (title.toUpperCase(), getLocalBounds().removeFromTop (20).reduced (12, 0),
                          juce::Justification::centredLeft, 1);
    }
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

    auto inner = area.reduced (12.0f, 10.0f);
    auto header = inner.removeFromTop (15.0f);

    g.setColour (theme::textDim());
    g.setFont (theme::labelFont (11.0f));
    g.drawFittedText ("WAVEFORM", header.toNearestInt(), juce::Justification::centredLeft, 1);

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
        // Min/max per pixel column: the classic dense waveform look.
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
        g.strokePath (wave, juce::PathStrokeType (1.6f));
    }
}

//==============================================================================
XyPad::XyPad (juce::AudioProcessorValueTreeState& state, int xParam, int yParam,
              juce::String padTitle)
    : apvts (state), xIndex (xParam), yIndex (yParam), title (std::move (padTitle))
{
    xParameter = rangedParam (apvts, xParam);
    yParameter = rangedParam (apvts, yParam);

    setInterceptsMouseClicks (true, false);
    startTimerHz (15);
}

XyPad::~XyPad() { stopTimer(); }

juce::Rectangle<float> XyPad::fieldArea() const
{
    auto inner = getLocalBounds().toFloat().reduced (2.0f).reduced (10.0f, 8.0f);
    inner.removeFromTop (15.0f);
    return inner.reduced (0.0f, 4.0f);
}

void XyPad::timerCallback()
{
    if (! isVisible())
        return;

    const juce::Point<float> next { normalisedOf (xParameter), 1.0f - normalisedOf (yParameter) };

    if (next.getDistanceFrom (handle) > 0.002f)
    {
        handle = next;
        repaint();
    }
}

void XyPad::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced (2.0f);

    g.setColour (theme::panel());
    g.fillRoundedRectangle (area, 12.0f);

    auto header = area.reduced (10.0f, 8.0f).removeFromTop (15.0f);

    g.setColour (theme::accentBright());
    g.setFont (theme::labelFont (11.0f));
    g.drawFittedText (title.toUpperCase(), header.toNearestInt(), juce::Justification::centredLeft, 1);

    const auto field = fieldArea();
    paintField (g, field);

    g.setColour (theme::outline());
    g.drawRoundedRectangle (field, 8.0f, 1.0f);

    const auto point = juce::Point<float> (field.getX() + handle.x * field.getWidth(),
                                           field.getY() + handle.y * field.getHeight());
    g.setColour (theme::accent().withAlpha (0.30f));
    g.fillEllipse (juce::Rectangle<float> (28.0f, 28.0f).withCentre (point));
    g.setColour (theme::text());
    g.fillEllipse (juce::Rectangle<float> (10.0f, 10.0f).withCentre (point));
}

void XyPad::mouseDown (const juce::MouseEvent& event)
{
    mouseDrag (event);
}

void XyPad::mouseDrag (const juce::MouseEvent& event)
{
    const auto field = fieldArea();

    if (field.getWidth() < 1.0f || field.getHeight() < 1.0f)
        return;

    const float nx = juce::jlimit (0.0f, 1.0f, (event.position.x - field.getX()) / field.getWidth());
    const float ny = juce::jlimit (0.0f, 1.0f, (event.position.y - field.getY()) / field.getHeight());

    writeNormalised (xParameter, nx);
    writeNormalised (yParameter, 1.0f - ny);

    handle = { nx, ny };
    repaint();
}

//==============================================================================
FilterPad::FilterPad (juce::AudioProcessorValueTreeState& state)
    : XyPad (state, (int) P::fltCutoff, (int) P::fltReso, "FILTER")
{
}

void FilterPad::paintField (juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour (theme::panelLight().withAlpha (0.5f));
    g.fillRoundedRectangle (area, 8.0f);

    const float cutoff = juce::jlimit (20.0f, 18000.0f, plainOf (rangedParam (apvts, xIndex)));
    const float resonance = juce::jlimit (0.0f, 1.0f, plainOf (rangedParam (apvts, yIndex)));
    const float q = 0.5f + resonance * 12.0f;

    juce::Path curve;
    const int steps = juce::jmax (24, (int) area.getWidth());
    for (int i = 0; i <= steps; ++i)
    {
        const float t = (float) i / (float) steps;
        const float hz = 20.0f * std::pow (900.0f, t);          // 20 Hz .. 18 kHz
        const float r = hz / cutoff;
        const float denom = std::sqrt ((1.0f - r * r) * (1.0f - r * r) + (r / q) * (r / q));
        const float magnitude = juce::jlimit (0.0f, 4.0f, 1.0f / juce::jmax (denom, 1.0e-4f));
        const float normalised = juce::jlimit (0.0f, 1.0f, magnitude * 0.5f);

        const float x = area.getX() + t * area.getWidth();
        const float y = area.getBottom() - normalised * area.getHeight() * 0.92f - 2.0f;

        if (i == 0)
            curve.startNewSubPath (x, y);
        else
            curve.lineTo (x, y);
    }

    juce::Path filled (curve);
    filled.lineTo (area.getRight(), area.getBottom());
    filled.lineTo (area.getX(), area.getBottom());
    filled.closeSubPath();

    g.setColour (theme::accentDeep().withAlpha (0.35f));
    g.fillPath (filled);
    g.setColour (theme::accentBright().withAlpha (0.9f));
    g.strokePath (curve, juce::PathStrokeType (1.5f));
}

//==============================================================================
GrowlPad::GrowlPad (juce::AudioProcessorValueTreeState& state)
    : XyPad (state, (int) P::distDrive, (int) P::wobDepth, "GROWL")
{
}

void GrowlPad::paintField (juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour (theme::panelLight().withAlpha (0.5f));
    g.fillRoundedRectangle (area, 8.0f);

    const float drive = 0.2f + juce::jlimit (0.0f, 1.0f, plainOf (rangedParam (apvts, xIndex))) * 6.0f;

    juce::Path curve;
    const int steps = juce::jmax (24, (int) area.getWidth());
    for (int i = 0; i <= steps; ++i)
    {
        const float t = (float) i / (float) steps;
        const float in = t * 2.0f - 1.0f;
        const float out = juce::jlimit (-1.0f, 1.0f, std::tanh (in * drive));
        const float x = area.getX() + t * area.getWidth();
        const float y = area.getCentreY() - out * area.getHeight() * 0.45f;

        if (i == 0)
            curve.startNewSubPath (x, y);
        else
            curve.lineTo (x, y);
    }

    g.setColour (theme::accentDeep().withAlpha (0.30f));
    g.strokePath (curve, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved));
    g.setColour (theme::warning().withAlpha (0.9f));
    g.strokePath (curve, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved));
}

//==============================================================================
WobblePanel::WobblePanel (juce::AudioProcessorValueTreeState& state, const Engine& engineToUse)
    : apvts (state), engine (engineToUse)
{
    startTimerHz (20);
}

WobblePanel::~WobblePanel() { stopTimer(); }

juce::Rectangle<int> WobblePanel::shapeStripArea() const
{
    return getLocalBounds().reduced (12, 9).removeFromBottom (24);
}

int WobblePanel::shapeAt (juce::Point<int> point) const
{
    const auto strip = shapeStripArea();

    if (! strip.contains (point))
        return -1;

    const int index = (int) ((float) (point.x - strip.getX()) / (float) juce::jmax (1, strip.getWidth())
                             * 10.0f);
    return juce::jlimit (0, 9, index);
}

void WobblePanel::timerCallback()
{
    if (! isVisible())
        return;

    repaint();
}

void WobblePanel::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced (2.0f);

    g.setColour (theme::panel());
    g.fillRoundedRectangle (area, 12.0f);

    auto inner = area.reduced (12.0f, 9.0f);
    auto header = inner.removeFromTop (15.0f);

    const int shape = (int) plainOf (rangedParam (apvts, (int) P::wobShape));
    const float depth = juce::jlimit (0.0f, 1.0f, plainOf (rangedParam (apvts, (int) P::wobDepth)));
    const bool synced = plainOf (rangedParam (apvts, (int) P::wobMode)) < 0.5f;

    g.setColour (theme::accentBright());
    g.setFont (theme::labelFont (10.5f));
    g.drawFittedText ("WOBBLE  X = RATE   Y = DEPTH", header.toNearestInt(),
                      juce::Justification::centredLeft, 1);

    g.setColour (theme::textDim());
    g.setFont (theme::valueFont (10.5f));
    g.drawFittedText ((synced ? juce::String ("SYNC ") : juce::String ("FREE "))
                          + rateText (engine.wobbleRateHz()),
                      header.toNearestInt(), juce::Justification::centredRight, 1);

    auto curve = inner.withTrimmedBottom (28.0f);
    g.setColour (theme::panelLight().withAlpha (0.45f));
    g.fillRoundedRectangle (curve, 8.0f);

    const auto centreY = curve.getCentreY();
    const float amplitude = curve.getHeight() * 0.42f * juce::jmax (0.06f, depth);
    const float phase = engine.wobblePhase();
    const int steps = juce::jmax (48, (int) curve.getWidth());

    juce::Path path;
    for (int i = 0; i <= steps; ++i)
    {
        const float t = (float) i / (float) steps;
        const float v = shapeValue (shape, phase + t * 2.0f, 0.55f);
        const float x = curve.getX() + t * curve.getWidth();
        const float y = centreY - v * amplitude;

        if (i == 0)
            path.startNewSubPath (x, y);
        else
            path.lineTo (x, y);
    }

    g.setColour (theme::accent().withAlpha (0.25f));
    g.strokePath (path, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved));
    g.setColour (theme::accentBright());
    g.strokePath (path, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved));

    g.setColour (theme::outline());
    g.drawRoundedRectangle (curve, 8.0f, 1.0f);

    // Shape picker strip.
    const auto strip = shapeStripArea().toFloat();
    const float cellWidth = strip.getWidth() / 10.0f;

    for (int s = 0; s < 10; ++s)
    {
        auto cell = juce::Rectangle<float> (strip.getX() + (float) s * cellWidth, strip.getY(),
                                            cellWidth - 3.0f, strip.getHeight());
        const bool selected = s == shape;

        g.setColour (selected ? theme::accentDeep() : theme::panelLight().withAlpha (0.6f));
        g.fillRoundedRectangle (cell, 5.0f);

        drawWavePicture (g, cell.reduced (3.0f, 4.0f), wobbleShapeToPicture (s),
                         1.0f, 1.0f, selected ? theme::text() : theme::textDim());
    }
}

void WobblePanel::mouseDown (const juce::MouseEvent& event)
{
    const int shape = shapeAt (event.getPosition());

    if (shape >= 0)
    {
        draggingShape = true;
        writeNormalised (rangedParam (apvts, (int) P::wobShape), (float) shape / 9.0f);
        repaint();
        return;
    }

    draggingCurve = true;
    mouseDrag (event);
}

void WobblePanel::mouseDrag (const juce::MouseEvent& event)
{
    if (draggingShape)
    {
        const int shape = shapeAt (event.getPosition());

        if (shape >= 0)
            writeNormalised (rangedParam (apvts, (int) P::wobShape), (float) shape / 9.0f);

        repaint();
        return;
    }

    if (! draggingCurve)
        return;

    auto curve = getLocalBounds().toFloat().reduced (14.0f, 9.0f);
    curve.removeFromTop (17.0f);
    curve.removeFromBottom (30.0f);

    if (curve.getWidth() < 1.0f || curve.getHeight() < 1.0f)
        return;

    const float nx = juce::jlimit (0.0f, 1.0f, (event.position.x - curve.getX()) / curve.getWidth());
    const float ny = juce::jlimit (0.0f, 1.0f, (event.position.y - curve.getY()) / curve.getHeight());

    const bool synced = plainOf (rangedParam (apvts, (int) P::wobMode)) < 0.5f;

    if (synced)
        writeNormalised (rangedParam (apvts, (int) P::wobDiv), nx, true);
    else
        writeNormalised (rangedParam (apvts, (int) P::wobRate), nx);

    writeNormalised (rangedParam (apvts, (int) P::wobDepth), 1.0f - ny);
    repaint();
}

//==============================================================================
WobbleTargets::WobbleTargets (juce::AudioProcessorValueTreeState& state) : apvts (state)
{
    tiles = { {
        { "SYNC",    -1,                 0.0f  },
        { "FILTER",  (int) P::wobCut,    0.85f },
        { "RESO",    (int) P::wobReso,   0.5f  },
        { "PITCH",   (int) P::wobPitch,  0.25f },
        { "WAVE",    (int) P::wobWt,     0.6f  },
        { "DIRT",    (int) P::wobDrive,  0.6f  },
        { "WIDTH",   (int) P::wobWidth,  0.6f  },
        { "FORMANT", (int) P::wobFormant, 0.5f }
    } };

    startTimerHz (8);
}

juce::Rectangle<int> WobbleTargets::tileBounds (int index) const
{
    const auto area = getLocalBounds();
    const int column = index % 4;
    const int row = index / 4;
    const int width = area.getWidth() / 4;
    const int height = area.getHeight() / 2;

    return juce::Rectangle<int> (area.getX() + column * width, area.getY() + row * height,
                                 width, height).reduced (3);
}

void WobbleTargets::timerCallback()
{
    if (! isVisible())
        return;

    bool changed = false;

    for (size_t i = 0; i < tiles.size(); ++i)
    {
        const auto& tile = tiles[i];
        const bool on = tile.paramIndex < 0
                            ? plainOf (rangedParam (apvts, (int) P::wobMode)) < 0.5f   // 0 = tempo sync
                            : std::fabs (plainOf (rangedParam (apvts, tile.paramIndex))) > 0.02f;

        if (on != active[i])
        {
            active[i] = on;
            changed = true;
        }
    }

    if (changed)
        repaint();
}

void WobbleTargets::paint (juce::Graphics& g)
{
    for (int i = 0; i < (int) tiles.size(); ++i)
    {
        const auto bounds = tileBounds (i).toFloat();
        const bool on = active[(size_t) i];

        g.setColour (on ? theme::accentDeep() : theme::panelLight());
        g.fillRoundedRectangle (bounds, 8.0f);
        g.setColour (on ? theme::accentBright() : theme::outline());
        g.drawRoundedRectangle (bounds, 8.0f, 1.0f);

        auto inner = bounds.reduced (8.0f, 4.0f);
        auto label = inner.removeFromBottom (11.0f);
        const auto iconArea = juce::Rectangle<float> (inner.getCentreX() - 17.0f, inner.getY() + 1.0f,
                                                      34.0f, juce::jmax (10.0f, inner.getHeight() - 2.0f));
        const auto colour = on ? theme::text() : theme::textDim();

        juce::Path path;
        const auto centreY = iconArea.getCentreY();

        switch (i)
        {
            case 0:   // SYNC: metronome with a pendulum
                path.startNewSubPath (iconArea.getCentreX() - 7.0f, iconArea.getBottom());
                path.lineTo (iconArea.getCentreX(), iconArea.getY());
                path.lineTo (iconArea.getCentreX() + 7.0f, iconArea.getBottom());
                path.closeSubPath();
                path.startNewSubPath (iconArea.getCentreX(), iconArea.getBottom() - 3.0f);
                path.lineTo (iconArea.getCentreX() + 6.0f, iconArea.getY() + 3.0f);
                break;

            case 1:   // FILTER: low-pass curve
                path.startNewSubPath (iconArea.getX(), centreY - iconArea.getHeight() * 0.3f);
                path.lineTo (iconArea.getCentreX(), centreY - iconArea.getHeight() * 0.3f);
                path.quadraticTo (iconArea.getRight() - 3.0f, centreY - iconArea.getHeight() * 0.25f,
                                  iconArea.getRight(), iconArea.getBottom());
                break;

            case 2:   // RESO: bump
                path.startNewSubPath (iconArea.getX(), centreY + iconArea.getHeight() * 0.15f);
                path.quadraticTo (iconArea.getCentreX(), centreY - iconArea.getHeight() * 0.6f,
                                  iconArea.getRight(), centreY + iconArea.getHeight() * 0.15f);
                break;

            case 3:   // PITCH: rising steps
                for (int s = 0; s < 4; ++s)
                {
                    const float x = iconArea.getX() + (float) s * iconArea.getWidth() / 4.0f;
                    const float y = iconArea.getBottom() - (float) (s + 1) * iconArea.getHeight() / 5.0f;
                    path.startNewSubPath (x, y);
                    path.lineTo (x + iconArea.getWidth() / 4.0f, y);
                }
                break;

            case 4:   // WAVE: three stacked waves
                for (int s = 0; s < 3; ++s)
                {
                    const float y = iconArea.getY() + (float) s * iconArea.getHeight() / 3.0f;
                    path.startNewSubPath (iconArea.getX(), y + iconArea.getHeight() / 6.0f);
                    path.quadraticTo (iconArea.getCentreX(), y - iconArea.getHeight() / 6.0f,
                                      iconArea.getRight(), y + iconArea.getHeight() / 6.0f);
                }
                break;

            case 5:   // DIRT: clipped wave
                path.startNewSubPath (iconArea.getX(), centreY);
                path.lineTo (iconArea.getX() + iconArea.getWidth() * 0.3f, centreY);
                path.lineTo (iconArea.getX() + iconArea.getWidth() * 0.4f, iconArea.getY());
                path.lineTo (iconArea.getX() + iconArea.getWidth() * 0.6f, iconArea.getY());
                path.lineTo (iconArea.getX() + iconArea.getWidth() * 0.7f, centreY);
                path.lineTo (iconArea.getX() + iconArea.getWidth() * 0.8f, iconArea.getBottom());
                path.lineTo (iconArea.getRight(), iconArea.getBottom());
                break;

            case 6:   // WIDTH: two diverging lines
                path.startNewSubPath (iconArea.getCentreX(), centreY);
                path.lineTo (iconArea.getX(), iconArea.getY());
                path.startNewSubPath (iconArea.getCentreX(), centreY);
                path.lineTo (iconArea.getRight(), iconArea.getBottom());
                break;

            default:  // FORMANT: two humps
                path.startNewSubPath (iconArea.getX(), centreY + iconArea.getHeight() * 0.25f);
                path.quadraticTo (iconArea.getX() + iconArea.getWidth() * 0.25f,
                                  centreY - iconArea.getHeight() * 0.45f,
                                  iconArea.getX() + iconArea.getWidth() * 0.5f,
                                  centreY + iconArea.getHeight() * 0.25f);
                path.quadraticTo (iconArea.getX() + iconArea.getWidth() * 0.75f,
                                  centreY - iconArea.getHeight() * 0.45f,
                                  iconArea.getRight(), centreY + iconArea.getHeight() * 0.25f);
                break;
        }

        g.setColour (colour);
        g.strokePath (path, juce::PathStrokeType (1.6f));

        g.setFont (theme::labelFont (9.0f));
        g.drawFittedText (tiles[(size_t) i].label, label.toNearestInt(),
                          juce::Justification::centred, 1);
    }
}

void WobbleTargets::mouseDown (const juce::MouseEvent& event)
{
    for (int i = 0; i < (int) tiles.size(); ++i)
    {
        if (! tileBounds (i).contains (event.getPosition()))
            continue;

        const auto& tile = tiles[(size_t) i];

        if (tile.paramIndex < 0)
            writeNormalised (rangedParam (apvts, (int) P::wobMode), active[(size_t) i] ? 1.0f : 0.0f);
        else
            writePlain (rangedParam (apvts, tile.paramIndex),
                        active[(size_t) i] ? 0.0f : tile.onAmount);

        timerCallback();
        return;
    }
}

//==============================================================================
LfoSlot::LfoSlot (juce::AudioProcessorValueTreeState& state, int lfoIndex)
    : apvts (state), lfo (juce::jlimit (0, 2, lfoIndex))
{
    startTimerHz (15);
}

LfoSlot::~LfoSlot() { stopTimer(); }

juce::Rectangle<int> LfoSlot::shapeArea() const
{
    auto header = getLocalBounds().reduced (10, 8).removeFromTop (16);
    return header.removeFromRight (54);
}

juce::Rectangle<int> LfoSlot::syncArea() const
{
    return getLocalBounds().reduced (10, 8).removeFromBottom (16).removeFromLeft (48);
}

juce::Rectangle<int> LfoSlot::curveArea() const
{
    auto bounds = getLocalBounds().reduced (10, 8);
    bounds.removeFromTop (18);
    bounds.removeFromBottom (20);
    return bounds;
}

void LfoSlot::timerCallback()
{
    if (! isVisible())
        return;

    const int base = (int) P::lfo1Sync + lfo * 9;
    const float rate = juce::jlimit (0.01f, 30.0f, plainOf (rangedParam (apvts, base + 2)));

    displayPhase += juce::jlimit (0.0f, 0.25f, rate / 15.0f);

    if (displayPhase >= 1.0f)
        displayPhase -= std::floor (displayPhase);

    repaint();
}

void LfoSlot::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced (2.0f);

    g.setColour (theme::panel());
    g.fillRoundedRectangle (area, 12.0f);
    g.setColour (theme::outline());
    g.drawRoundedRectangle (area, 12.0f, 1.0f);

    const int base = (int) P::lfo1Sync + lfo * 9;
    const bool synced = plainOf (rangedParam (apvts, base)) > 0.5f;    // 1 = tempo sync
    const int shape = (int) plainOf (rangedParam (apvts, base + 3));
    const float depth = juce::jlimit (0.0f, 1.0f, plainOf (rangedParam (apvts, base + 4)));

    auto header = getLocalBounds().reduced (10, 8).removeFromTop (16);

    g.setColour (theme::accentBright());
    g.setFont (theme::labelFont (10.5f));
    g.drawFittedText ("LFO " + juce::String (lfo + 1), header, juce::Justification::centredLeft, 1);

    auto chip = shapeArea().toFloat();
    g.setColour (theme::panelLight());
    g.fillRoundedRectangle (chip, 5.0f);
    g.setColour (theme::outline());
    g.drawRoundedRectangle (chip, 5.0f, 1.0f);
    drawWavePicture (g, chip.reduced (4.0f, 3.0f), shape, 1.0f, 1.0f, theme::text());

    const auto curve = curveArea().toFloat();
    g.setColour (theme::panelLight().withAlpha (0.45f));
    g.fillRoundedRectangle (curve, 8.0f);

    const auto centreY = curve.getCentreY();
    const float amplitude = curve.getHeight() * 0.42f * juce::jmax (0.08f, depth);
    const int steps = juce::jmax (32, (int) curve.getWidth());

    juce::Path path;
    for (int i = 0; i <= steps; ++i)
    {
        const float t = (float) i / (float) steps;
        const float v = lfoShapeValue (shape, displayPhase + t * 2.0f);
        const float x = curve.getX() + t * curve.getWidth();
        const float y = centreY - v * amplitude;

        if (i == 0)
            path.startNewSubPath (x, y);
        else
            path.lineTo (x, y);
    }

    g.setColour (theme::accent().withAlpha (0.25f));
    g.strokePath (path, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved));
    g.setColour (theme::accentBright());
    g.strokePath (path, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved));

    auto pill = syncArea().toFloat();
    g.setColour (synced ? theme::accentDeep() : theme::panelLight());
    g.fillRoundedRectangle (pill, 5.0f);
    g.setColour (synced ? theme::text() : theme::textDim());
    g.setFont (theme::labelFont (9.5f));
    g.drawFittedText (synced ? "SYNC" : "FREE", pill.toNearestInt(), juce::Justification::centred, 1);

    g.setColour (theme::textDim());
    g.setFont (theme::valueFont (10.0f));
    g.drawFittedText (rateText (plainOf (rangedParam (apvts, base + 2))),
                      getLocalBounds().reduced (10, 8).removeFromBottom (16).removeFromRight (110),
                      juce::Justification::centredRight, 1);
}

void LfoSlot::mouseDown (const juce::MouseEvent& event)
{
    const int base = (int) P::lfo1Sync + lfo * 9;

    if (shapeArea().contains (event.getPosition()))
    {
        const int shape = (int) plainOf (rangedParam (apvts, base + 3));
        writeNormalised (rangedParam (apvts, base + 3), (float) ((shape + 1) % 6) / 5.0f);
        repaint();
        return;
    }

    if (syncArea().contains (event.getPosition()))
    {
        const bool synced = plainOf (rangedParam (apvts, base)) > 0.5f;
        writeNormalised (rangedParam (apvts, base), synced ? 0.0f : 1.0f);
        repaint();
        return;
    }

    draggingCurve = true;
    mouseDrag (event);
}

void LfoSlot::mouseDrag (const juce::MouseEvent& event)
{
    if (! draggingCurve)
        return;

    const int base = (int) P::lfo1Sync + lfo * 9;
    const auto curve = curveArea().toFloat();

    if (curve.getWidth() < 1.0f || curve.getHeight() < 1.0f)
        return;

    const float nx = juce::jlimit (0.0f, 1.0f, (event.position.x - curve.getX()) / curve.getWidth());
    const float ny = juce::jlimit (0.0f, 1.0f, (event.position.y - curve.getY()) / curve.getHeight());
    const bool synced = plainOf (rangedParam (apvts, base)) > 0.5f;

    if (synced)
        writeNormalised (rangedParam (apvts, base + 1), nx, true);
    else
        writeNormalised (rangedParam (apvts, base + 2), nx);

    writeNormalised (rangedParam (apvts, base + 4), 1.0f - ny);
    repaint();
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
