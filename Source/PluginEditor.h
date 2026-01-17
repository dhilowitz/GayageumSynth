/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "StringControl.h"

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
    
    juce::Label bodyResonanceLabel;
    juce::Slider bodyResonanceSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bodyResonanceAttachment;
    
    juce::Label excitationLabel;
    juce::Slider excitationSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> excitationAttachment;
    
    // Play mode toggle
    juce::ToggleButton playModeToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> playModeAttachment;
    
    // Info labels
    juce::Label infoLabel;
    
    // Resizer
    juce::ResizableCornerComponent resizer;
    juce::ComponentBoundsConstrainer resizeConstraints;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Gayageum1AudioProcessorEditor)
};
