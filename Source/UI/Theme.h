#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace mk
{
namespace theme
{

inline juce::Colour background()      { return juce::Colour (0xff0b0910); }
inline juce::Colour panel()           { return juce::Colour (0xff16121f); }
inline juce::Colour panelLight()      { return juce::Colour (0xff221a33); }
inline juce::Colour outline()         { return juce::Colour (0xff2f2543); }
inline juce::Colour accent()          { return juce::Colour (0xffa855f7); }
inline juce::Colour accentBright()    { return juce::Colour (0xffc88bfb); }
inline juce::Colour accentDeep()      { return juce::Colour (0xff6d28d9); }
inline juce::Colour text()            { return juce::Colour (0xffede9f7); }
inline juce::Colour textDim()         { return juce::Colour (0xff8b84a3); }
inline juce::Colour meter()           { return juce::Colour (0xffc88bfb); }
inline juce::Colour warning()         { return juce::Colour (0xffef7d7d); }

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

    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getPopupMenuFont() override;
};

} // namespace mk
