/*
  ==============================================================================

    TraditionalModePanel.h
    Container for all string controls used in traditional mode

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "StringControl.h"

//==============================================================================
/**
    Panel containing all 12 string controls for traditional gayageum mode
*/
class TraditionalModePanel : public juce::Component
{
public:
    TraditionalModePanel(juce::AudioProcessorValueTreeState& apvts);
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
private:
    static constexpr int numStrings = 12;
    std::array<std::unique_ptr<StringControl>, numStrings> stringControls;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TraditionalModePanel)
};
