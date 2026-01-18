/*
  ==============================================================================

    FreePlayModePanel.cpp
    Container for free play mode controls

  ==============================================================================
*/

#include "FreePlayModePanel.h"

//==============================================================================
FreePlayModePanel::FreePlayModePanel(juce::AudioProcessorValueTreeState& apvts)
{
    // Double strings toggle
    doubleLabel.setText("Double Strings", juce::dontSendNotification);
    doubleLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    doubleLabel.setJustificationType(juce::Justification::centred);
    doubleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(doubleLabel);
    
    doubleToggle.setButtonText("Enable");
    doubleToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    doubleToggle.setColour(juce::ToggleButton::tickColourId, juce::Colours::lightblue);
    addAndMakeVisible(doubleToggle);
    
    doubleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, "doubleStrings", doubleToggle);
    
    // Detune slider
    detuneLabel.setText("Detune (semitones)", juce::dontSendNotification);
    detuneLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    detuneLabel.setJustificationType(juce::Justification::centred);
    detuneLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(detuneLabel);
    
    detuneSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    detuneSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    detuneSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::lightblue);
    detuneSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    addAndMakeVisible(detuneSlider);
    
    detuneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "doubleDetune", detuneSlider);
}

void FreePlayModePanel::paint(juce::Graphics& g)
{
    // Draw panel background
    auto bounds = getLocalBounds();
    
    g.setColour(juce::Colours::darkblue.withAlpha(0.3f));
    g.fillRoundedRectangle(bounds.toFloat(), 10.0f);
    
    g.setColour(juce::Colours::lightblue.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.toFloat(), 10.0f, 2.0f);
}

void FreePlayModePanel::resized()
{
    auto bounds = getLocalBounds().reduced(20, 10);
    
    // Double strings toggle section (left half)
    auto doubleArea = bounds.removeFromLeft(bounds.getWidth() / 2);
    doubleLabel.setBounds(doubleArea.removeFromTop(30));
    doubleToggle.setBounds(doubleArea.withSizeKeepingCentre(120, 30));
    
    // Detune slider section (right half)
    auto detuneArea = bounds;
    detuneLabel.setBounds(detuneArea.removeFromTop(30));
    detuneSlider.setBounds(detuneArea.withSizeKeepingCentre(120, 120));
}
