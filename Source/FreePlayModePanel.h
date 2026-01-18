/*
  ==============================================================================

    FreePlayModePanel.h
    Container for free play mode controls

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Panel containing controls for free play mode
*/
class FreePlayModePanel : public juce::Component
{
public:
    FreePlayModePanel(juce::AudioProcessorValueTreeState& apvts);
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
private:
    // Double strings toggle
    juce::Label doubleLabel;
    juce::ToggleButton doubleToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> doubleAttachment;
    
    // Detune slider
    juce::Label detuneLabel;
    juce::Slider detuneSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> detuneAttachment;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreePlayModePanel)
};
