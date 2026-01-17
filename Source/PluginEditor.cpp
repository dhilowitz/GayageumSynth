/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// StringControl Implementation
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

//==============================================================================
// Gayageum1AudioProcessorEditor Implementation
//==============================================================================

Gayageum1AudioProcessorEditor::Gayageum1AudioProcessorEditor (Gayageum1AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
    , resizer(this, &resizeConstraints)
{
    // Set size constraints
    resizeConstraints.setMinimumSize(800, 300);
    resizeConstraints.setMaximumSize(1600, 800);
    
    // Title
    titleLabel.setText("Gayageum Virtual Instrument", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::gold);
    addAndMakeVisible(titleLabel);
    
    // Create 12 string controls
    for (int i = 0; i < numStrings; ++i)
    {
        stringControls[i] = std::make_unique<StringControl>(i, audioProcessor.apvts);
        addAndMakeVisible(stringControls[i].get());
    }
    
    // Global damping control
    dampingLabel.setText("Global Damping", juce::dontSendNotification);
    dampingLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    dampingLabel.setJustificationType(juce::Justification::centred);
    dampingLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(dampingLabel);
    
    dampingSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dampingSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    dampingSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::lightblue);
    dampingSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    addAndMakeVisible(dampingSlider);
    
    dampingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "damping", dampingSlider);
    
    // Info label
    infoLabel.setText("Move Anjok bridges (sliders) to tune each string. MIDI notes 60-71 (C4-B4) trigger strings 1-12.", 
                      juce::dontSendNotification);
    infoLabel.setFont(juce::FontOptions(11.0f));
    infoLabel.setJustificationType(juce::Justification::centred);
    infoLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(infoLabel);
    
    // Add resizer
    addAndMakeVisible(resizer);
    
    // Set overall size AFTER adding all components
    setSize (1000, 400);
    setResizable(true, true);
}

Gayageum1AudioProcessorEditor::~Gayageum1AudioProcessorEditor()
{
}

//==============================================================================
void Gayageum1AudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background gradient
    auto bounds = getLocalBounds();
    juce::ColourGradient gradient(
        juce::Colour(0xff1a1a2e), 0, 0,
        juce::Colour(0xff16213e), 0, bounds.getHeight(),
        false);
    g.setGradientFill(gradient);
    g.fillAll();
    
    // Draw instrument body outline
    auto bodyArea = bounds.reduced(20, 80);
    bodyArea.removeFromRight(150); // Space for damping control
    
    g.setColour(juce::Colours::saddlebrown.withAlpha(0.3f));
    g.fillRoundedRectangle(bodyArea.toFloat(), 10.0f);
    
    g.setColour(juce::Colours::peru.withAlpha(0.5f));
    g.drawRoundedRectangle(bodyArea.toFloat(), 10.0f, 2.0f);
}

void Gayageum1AudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Position resizer in bottom-right corner
    resizer.setBounds(bounds.removeFromRight(16).removeFromBottom(16));
    
    // Title at top
    titleLabel.setBounds(bounds.removeFromTop(40).reduced(20, 5));
    
    bounds.removeFromTop(10);
    
    // Bottom info
    auto bottomArea = bounds.removeFromBottom(35);
    infoLabel.setBounds(bottomArea.reduced(20, 5));
    
    // Main content area
    auto contentArea = bounds.reduced(20, 10);
    
    // Damping control on right
    auto controlArea = contentArea.removeFromRight(150);
    controlArea = controlArea.withSizeKeepingCentre(120, 150);
    dampingLabel.setBounds(controlArea.removeFromTop(25));
    dampingSlider.setBounds(controlArea);
    
    // Strings area
    auto stringsArea = contentArea.reduced(10, 5);
    
    if (stringsArea.getWidth() > 0 && numStrings > 0)
    {
        int stringWidth = stringsArea.getWidth() / numStrings;
        
        for (int i = 0; i < numStrings; ++i)
        {
            if (stringControls[i] != nullptr)
            {
                auto stringBounds = stringsArea.removeFromLeft(stringWidth).reduced(2, 0);
                stringControls[i]->setBounds(stringBounds);
            }
        }
    }
}
