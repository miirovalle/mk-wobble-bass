#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace mk
{
namespace theme
{

inline juce::Colour background()      { return juce::Colour (0xff08080a); }
inline juce::Colour panel()           { return juce::Colour (0xff141416); }
inline juce::Colour panelLight()      { return juce::Colour (0xff1f1d1c); }
inline juce::Colour outline()         { return juce::Colour (0xff332b24); }
inline juce::Colour accent()          { return juce::Colour (0xffff7a18); }
inline juce::Colour accentBright()    { return juce::Colour (0xffffa451); }
inline juce::Colour accentDeep()      { return juce::Colour (0xffa63f06); }
inline juce::Colour text()            { return juce::Colour (0xfff4efe9); }
inline juce::Colour textDim()         { return juce::Colour (0xff8a8178); }
inline juce::Colour meter()           { return juce::Colour (0xffffa451); }
inline juce::Colour warning()         { return juce::Colour (0xffef5a5a); }

inline juce::Font labelFont (float height) { return juce::Font (juce::FontOptions (height, juce::Font::bold)); }
inline juce::Font valueFont (float height) { return juce::Font (juce::FontOptions (height)); }

} // namespace theme

class MkLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    MkLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;

    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;

    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getPopupMenuFont() override;
};

} // namespace mk
