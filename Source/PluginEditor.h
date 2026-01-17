/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

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

//==============================================================================
/**
*/
class Gayageum1AudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    Gayageum1AudioProcessorEditor (Gayageum1AudioProcessor&);
    ~Gayageum1AudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    Gayageum1AudioProcessor& audioProcessor;
    
    // Visual components
    static constexpr int numStrings = 12;
    std::array<std::unique_ptr<StringControl>, numStrings> stringControls;
    
    // Global controls
    juce::Label titleLabel;
    juce::Label dampingLabel;
    juce::Slider dampingSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dampingAttachment;
    
    // Info labels
    juce::Label infoLabel;
    
    // Resizer
    juce::ResizableCornerComponent resizer;
    juce::ComponentBoundsConstrainer resizeConstraints;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Gayageum1AudioProcessorEditor)
};
