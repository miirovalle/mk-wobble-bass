#include "UI/Theme.h"

namespace mk
{

MkLookAndFeel::MkLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, theme::background());
    setColour (juce::PopupMenu::backgroundColourId, theme::panel());
    setColour (juce::PopupMenu::textColourId, theme::text());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, theme::accentDeep());
    setColour (juce::PopupMenu::highlightedTextColourId, theme::text());
    setColour (juce::ComboBox::backgroundColourId, theme::panelLight());
    setColour (juce::ComboBox::outlineColourId, theme::outline());
    setColour (juce::ComboBox::textColourId, theme::text());
    setColour (juce::ComboBox::arrowColourId, theme::accentBright());
    setColour (juce::ComboBox::focusedOutlineColourId, theme::accent());
    setColour (juce::TextButton::buttonColourId, theme::panelLight());
    setColour (juce::TextButton::buttonOnColourId, theme::accentDeep());
    setColour (juce::TextButton::textColourOffId, theme::textDim());
    setColour (juce::TextButton::textColourOnId, theme::text());
    setColour (juce::Label::textColourId, theme::text());
    setColour (juce::Slider::textBoxTextColourId, theme::text());
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, theme::panelLight());
    setColour (juce::Slider::trackColourId, theme::accentDeep());
    setColour (juce::Slider::backgroundColourId, theme::outline());
    setColour (juce::Slider::thumbColourId, theme::accent());
    setColour (juce::TextEditor::backgroundColourId, theme::panelLight());
    setColour (juce::TextEditor::textColourId, theme::text());
    setColour (juce::TextEditor::outlineColourId, theme::outline());
    setColour (juce::TextEditor::focusedOutlineColourId, theme::accent());
    setColour (juce::ListBox::backgroundColourId, theme::panel());
    setColour (juce::ListBox::textColourId, theme::text());
    setColour (juce::ScrollBar::thumbColourId, theme::outline());
    setColour (juce::TooltipWindow::backgroundColourId, theme::panelLight());
    setColour (juce::TooltipWindow::textColourId, theme::text());
}

void MkLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                      float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                      juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height)
                            .reduced (4.0f);
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const float radius = size * 0.5f;
    const auto centre = bounds.getCentre();
    const float lineWidth = juce::jmax (3.0f, radius * 0.16f);
    const float arcRadius = radius - lineWidth * 0.5f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    juce::Path background;
    background.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                              rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (theme::outline());
    g.strokePath (background, juce::PathStrokeType (lineWidth, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                         rotaryStartAngle, angle, true);

    // Soft glow under the value arc keeps the control readable on stage.
    g.setColour (theme::accent().withAlpha (0.22f));
    g.strokePath (value, juce::PathStrokeType (lineWidth * 2.2f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    g.setColour (slider.isMouseOverOrDragging() ? theme::accentBright() : theme::accent());
    g.strokePath (value, juce::PathStrokeType (lineWidth, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    const float pointerLength = arcRadius * 0.72f;
    const juce::Point<float> start (centre.x + pointerLength * 0.35f * std::sin (angle),
                                    centre.y - pointerLength * 0.35f * std::cos (angle));
    const juce::Point<float> end (centre.x + pointerLength * std::sin (angle),
                                  centre.y - pointerLength * std::cos (angle));
    g.setColour (theme::text());
    g.drawLine ({ start, end }, juce::jmax (2.0f, lineWidth * 0.55f));

    g.setColour (theme::panelLight());
    g.fillEllipse (juce::Rectangle<float> (radius * 0.42f, radius * 0.42f).withCentre (centre));
}

void MkLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                  int, int, int, int, juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (1.0f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (box.hasKeyboardFocus (true) ? theme::accent() : theme::outline());
    g.drawRoundedRectangle (bounds, 6.0f, box.hasKeyboardFocus (true) ? 1.6f : 1.0f);

    const float arrowSize = juce::jmin (9.0f, (float) height * 0.28f);
    juce::Path arrow;
    arrow.addTriangle ((float) width - 20.0f, (float) height * 0.5f - arrowSize * 0.5f,
                       (float) width - 20.0f + arrowSize, (float) height * 0.5f - arrowSize * 0.5f,
                       (float) width - 20.0f + arrowSize * 0.5f, (float) height * 0.5f + arrowSize * 0.5f);
    g.setColour (theme::accentBright());
    g.fillPath (arrow);
}

void MkLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                          const juce::Colour& backgroundColour,
                                          bool highlighted, bool down)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    auto colour = backgroundColour;
    if (button.getToggleState())
        colour = theme::accentDeep();
    if (down)
        colour = colour.brighter (0.2f);
    else if (highlighted)
        colour = colour.brighter (0.08f);

    g.setColour (colour);
    g.fillRoundedRectangle (bounds, 7.0f);
    g.setColour (button.getToggleState() ? theme::accent() : theme::outline());
    g.drawRoundedRectangle (bounds, 7.0f, 1.0f);
}

void MkLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                      bool highlighted, bool)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto box = bounds.removeFromLeft (bounds.getHeight()).reduced (5.0f);
    const bool on = button.getToggleState();

    g.setColour (on ? theme::accentDeep() : theme::panelLight());
    g.fillRoundedRectangle (box, 5.0f);
    g.setColour (on ? theme::accentBright() : theme::outline());
    g.drawRoundedRectangle (box, 5.0f, 1.2f);

    if (on)
    {
        g.setColour (theme::accentBright());
        g.fillEllipse (box.reduced (box.getWidth() * 0.28f));
    }

    g.setColour (highlighted ? theme::text() : theme::textDim());
    g.setFont (theme::labelFont (13.0f));
    g.drawFittedText (button.getButtonText(), bounds.toNearestInt(), juce::Justification::centredLeft, 1);
}

void MkLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                      float sliderPos, float minSliderPos, float maxSliderPos,
                                      juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearHorizontal)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos,
                                          minSliderPos, maxSliderPos, style, slider);
        return;
    }

    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
    auto track = bounds.withSizeKeepingCentre (juce::jmax (0.0f, bounds.getWidth() - 2.0f), 6.0f);

    g.setColour (slider.findColour (juce::Slider::backgroundColourId));
    g.fillRoundedRectangle (track, 3.0f);

    // Amount is bipolar, so the fill grows out of the centre.
    const float centre = track.getCentreX();
    const float clamped = juce::jlimit (track.getX(), track.getRight(), sliderPos);
    const float left = juce::jmin (centre, clamped);
    const float right = juce::jmax (centre, clamped);

    if (right - left > 0.5f)
    {
        g.setColour (slider.findColour (juce::Slider::trackColourId));
        g.fillRoundedRectangle (juce::Rectangle<float> (left, track.getY(), right - left, track.getHeight()),
                                3.0f);
    }

    g.setColour (slider.findColour (juce::Slider::thumbColourId));
    g.fillEllipse (juce::Rectangle<float> (11.0f, 11.0f).withCentre ({ clamped, track.getCentreY() }));
}

juce::Font MkLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return theme::valueFont (14.0f);
}

juce::Font MkLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return theme::labelFont (juce::jmin (15.0f, (float) buttonHeight * 0.45f));
}

juce::Font MkLookAndFeel::getPopupMenuFont()
{
    return theme::valueFont (15.0f);
}

} // namespace mk
