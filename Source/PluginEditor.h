#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Theme.h"

//==============================================================================
/** The twelve strings' Anjok faders on the left; Tone (Excitation, Body, Damping) and Doubling on
 *  the right; the play mode in the title bar. Drawn in DHTracker's style, like DHAmbience and DHRoom.
 *  Controls the current mode doesn't use stay in place, dimmed.
 */
class GayageumSynthAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit GayageumSynthAudioProcessorEditor (GayageumSynthAudioProcessor&);
    ~GayageumSynthAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    //==========================================================================
    /** One control in a section: its name in small capitals, then the control, then its value. */
    class Cell : public juce::Component
    {
    public:
        explicit Cell (const juce::String& name);
        void paint (juce::Graphics&) override;

        static constexpr int nameHeight = 16, readoutHeight = 20, gap = 4;

    protected:
        juce::Rectangle<int> controlArea() const;
        juce::Rectangle<int> readoutArea() const;

    private:
        juce::String name;
    };

    /** A rotary knob with its value in the readout. */
    class Knob : public Cell
    {
    public:
        Knob (juce::AudioProcessorValueTreeState&, const juce::String& paramID, const juce::String& name,
              const juce::String& tooltip, float origin = 0.0f);
        void resized() override;

        juce::Slider slider;

    private:
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    /** One string: its number, the note it's tuned to, its Anjok fader and the Anjok's position. */
    class StringCell : public Cell
    {
    public:
        StringCell (juce::AudioProcessorValueTreeState&, int stringIndex);
        void resized() override;
        void paint (juce::Graphics&) override;

        static constexpr int pitchHeight = 16;

    private:
        int stringIndex;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    /** A round push button in a knob's place. */
    class RoundButton : public juce::Button
    {
    public:
        RoundButton() : juce::Button ({}) {}
        void paintButton (juce::Graphics&, bool isMouseOver, bool isButtonDown) override;
    };

    /** A toggle, with "on" or "off" in the readout. */
    class ToggleCell : public Cell
    {
    public:
        ToggleCell (juce::AudioProcessorValueTreeState&, const juce::String& paramID, const juce::String& name,
                    const juce::String& tooltip);
        void resized() override;
        void paint (juce::Graphics&) override;

    private:
        RoundButton button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
    };

private:
    void showMode (bool freePlay);

    // Section areas, worked out in resized() and painted in paint()
    struct Section { juce::String title; juce::Rectangle<int> area; };
    std::vector<Section> sections;

    //==========================================================================
    GayageumSynthAudioProcessor& audioProcessor;
    Theme::LookAndFeel lookAndFeel;   // before the components, so it outlives them

    // Play mode, in the title bar
    juce::TextButton traditionalButton { "Traditional" }, freePlayButton { "Free Play" };
    std::unique_ptr<juce::ParameterAttachment> playModeAttachment;
    bool freePlayMode = false;

    // Strings
    std::array<std::unique_ptr<StringCell>, GayageumSynthAudioProcessor::numStrings> strings;
    // Tone
    Knob excitation, body, damping;
    // Doubling
    ToggleCell doubleStrings;
    Knob detune;

    juce::TooltipWindow tooltips { this, 700 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GayageumSynthAudioProcessorEditor)
};
