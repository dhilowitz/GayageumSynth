/*
  ==============================================================================

    StringControl.cpp
    Visual representation of a single gayageum string with Anjok control

  ==============================================================================
*/

#include "StringControl.h"

//==============================================================================
StringControl::StringControl(int stringNumber, juce::AudioProcessorValueTreeState& apvts)
    : stringNum(stringNumber)
{
    // String label
    stringLabel.setText("String " + juce::String(stringNum + 1), juce::dontSendNotification);
    stringLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    stringLabel.setJustificationType(juce::Justification::centred);
    stringLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(stringLabel);
    
    // Anjok position slider (vertical)
    anjokSlider.setSliderStyle(juce::Slider::LinearVertical);
    anjokSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    anjokSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);
    anjokSlider.setColour(juce::Slider::thumbColourId, juce::Colours::lightgoldenrodyellow);
    addAndMakeVisible(anjokSlider);
    
    // Position label
    positionLabel.setText("Anjok", juce::dontSendNotification);
    positionLabel.setFont(juce::FontOptions(10.0f));
    positionLabel.setJustificationType(juce::Justification::centred);
    positionLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(positionLabel);
    
    // Attach to parameter
    auto paramID = "anjok" + juce::String(stringNum + 1);
    anjokAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, paramID, anjokSlider);
}

void StringControl::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Draw string representation
    auto stringArea = bounds.reduced(4, 30);
    stringArea.removeFromBottom(60);
    
    // String line (silk string appearance)
    g.setColour(juce::Colours::sandybrown.withAlpha(0.8f));
    auto stringLine = stringArea.withHeight(3).withCentre(stringArea.getCentre());
    g.fillRect(stringLine);
    
    // Highlight if this is a reference string (2, 5, 8, 11 from the paper)
    if (stringNum == 1 || stringNum == 4 || stringNum == 7 || stringNum == 10)
    {
        g.setColour(juce::Colours::orange.withAlpha(0.2f));
        g.fillRect(bounds);
    }
}

void StringControl::resized()
{
    auto bounds = getLocalBounds();
    
    stringLabel.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(10);
    
    positionLabel.setBounds(bounds.removeFromBottom(15));
    anjokSlider.setBounds(bounds.reduced(10, 0));
}
