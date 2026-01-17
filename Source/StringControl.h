/*
  ==============================================================================

    StringControl.h
    Visual representation of a single gayageum string with Anjok control

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Visual representation of a single gayageum string with Anjok control
*/
class StringControl : public juce::Component
{
public:
    StringControl(int stringNumber, juce::AudioProcessorValueTreeState& apvts);
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
private:
    int stringNum;
    juce::Label stringLabel;
    juce::Slider anjokSlider;
    juce::Label positionLabel;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> anjokAttachment;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StringControl)
};
