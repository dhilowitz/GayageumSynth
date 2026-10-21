#pragma once

#include <JuceHeader.h>

//==============================================================================
// DHTracker's look (Projects/DHTracker/Source/Theme.h), trimmed to what this editor uses: cool greys,
// Barlow Condensed for the interface and JetBrains Mono for values, and one accent blue.
namespace Theme
{
    namespace Colours
    {
        const juce::Colour ground         { 0xff111215 };   // window background
        const juce::Colour bar            { 0xff16181c };   // title bar
        const juce::Colour panel          { 0xff1b1d22 };   // sections
        const juce::Colour border         { 0xff2a2d35 };
        const juce::Colour divider        { 0xff262930 };
        const juce::Colour button         { 0xff1d2026 };
        const juce::Colour inset          { 0xff0e0f12 };   // readouts, fields
        const juce::Colour insetBorder    { 0xff24272e };
        const juce::Colour well           { 0xff0b0c0e };   // fader tracks, meters
        const juce::Colour knobFace       { 0xff22252b };
        const juce::Colour knobEdge       { 0xff3a3e48 };
        const juce::Colour knobTrack      { 0xff2c3039 };
        const juce::Colour faderCap       { 0xff3a3e48 };
        const juce::Colour faderCapEdge   { 0xff5a5f6b };

        const juce::Colour text           { 0xffe4e6eb };
        const juce::Colour textSecondary  { 0xff9aa0ad };
        const juce::Colour label          { 0xff7d8391 };   // small capitals labels
        const juce::Colour dim            { 0xff5d6370 };

        const juce::Colour accent         { 0xff7aa7ff };
        const juce::Colour meterGreen     { 0xff3fbf5f };
        const juce::Colour meterAmber     { 0xffe6b13f };
        const juce::Colour meterRed       { 0xffe0533f };
    }

    enum class Weight { medium, semibold, bold };

    juce::Font ui (float height, Weight weight = Weight::semibold);   // Barlow Condensed
    juce::Font mono (float height, bool semibold = false);            // JetBrains Mono

    // Draws a label in small spaced capitals ("GATING")
    void drawLabel (juce::Graphics&, const juce::String& text, juce::Rectangle<int> area,
                    juce::Justification = juce::Justification::centredLeft, float height = 12.0f);

    //==============================================================================
    class LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel();

        juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;

        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                                   bool isMouseOverButton, bool isButtonDown) override;
        juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

        // Rotary sliders: an arc in the slider's rotarySliderFillColourId from the minimum, or from
        // the slider's "origin" property (a 0-1 position, e.g. 0 dB on a gain knob), and a pointer
        void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                               float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;

        // Horizontal faders: a thin track filled to the value and a cap; vertical faders: DHTracker's
        // mixer fader, filled from the bottom; LinearBarVertical sliders are bare readouts (their
        // text box is the whole slider, and dragging it changes the value)
        void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                               float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle, juce::Slider&) override;

        juce::Label* createSliderTextBox (juce::Slider&) override;

        juce::Font getPopupMenuFont() override;
        juce::Font getComboBoxFont (juce::ComboBox&) override;
        juce::Font getLabelFont (juce::Label&) override;
    };
}
