/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

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
    
    // Body resonance control
    bodyResonanceLabel.setText("Body Resonance", juce::dontSendNotification);
    bodyResonanceLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    bodyResonanceLabel.setJustificationType(juce::Justification::centred);
    bodyResonanceLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(bodyResonanceLabel);
    
    bodyResonanceSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    bodyResonanceSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    bodyResonanceSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::sandybrown);
    bodyResonanceSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    addAndMakeVisible(bodyResonanceSlider);
    
    bodyResonanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "bodyResonance", bodyResonanceSlider);
    
    // Excitation blend control
    excitationLabel.setText("Excitation Type", juce::dontSendNotification);
    excitationLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    excitationLabel.setJustificationType(juce::Justification::centred);
    excitationLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(excitationLabel);
    
    excitationSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    excitationSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    excitationSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::lightgreen);
    excitationSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    addAndMakeVisible(excitationSlider);
    
    excitationAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "excitationBlend", excitationSlider);
    
    // Play mode toggle
    playModeToggle.setButtonText("Free Play Mode");
    playModeToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    playModeToggle.setColour(juce::ToggleButton::tickColourId, juce::Colours::lightgreen);
    addAndMakeVisible(playModeToggle);
    
    playModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "playMode", playModeToggle);
    
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
    
    // Play mode toggle below title
    auto toggleArea = bounds.removeFromTop(30);
    playModeToggle.setBounds(toggleArea.withSizeKeepingCentre(150, 25));
    
    bounds.removeFromTop(10);
    
    // Bottom info
    auto bottomArea = bounds.removeFromBottom(35);
    infoLabel.setBounds(bottomArea.reduced(20, 5));
    
    // Main content area
    auto contentArea = bounds.reduced(20, 10);
    
    // Control area on right (damping, body resonance, and excitation)
    auto controlArea = contentArea.removeFromRight(150);
    
    // Divide control area into thirds
    int thirdHeight = controlArea.getHeight() / 3;
    
    // Excitation control at top
    auto excitationControl = controlArea.removeFromTop(thirdHeight);
    excitationControl = excitationControl.withSizeKeepingCentre(120, 140);
    excitationLabel.setBounds(excitationControl.removeFromTop(25));
    excitationSlider.setBounds(excitationControl);
    
    // Body resonance control in middle
    auto bodyControl = controlArea.removeFromTop(thirdHeight);
    bodyControl = bodyControl.withSizeKeepingCentre(120, 140);
    bodyResonanceLabel.setBounds(bodyControl.removeFromTop(25));
    bodyResonanceSlider.setBounds(bodyControl);
    
    // Damping control at bottom
    controlArea = controlArea.withSizeKeepingCentre(120, 140);
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
