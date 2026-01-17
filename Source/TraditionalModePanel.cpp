/*
  ==============================================================================

    TraditionalModePanel.cpp
    Container for all string controls used in traditional mode

  ==============================================================================
*/

#include "TraditionalModePanel.h"

//==============================================================================
TraditionalModePanel::TraditionalModePanel(juce::AudioProcessorValueTreeState& apvts)
{
    // Create 12 string controls
    for (int i = 0; i < numStrings; ++i)
    {
        stringControls[i] = std::make_unique<StringControl>(i, apvts);
        addAndMakeVisible(stringControls[i].get());
    }
}

void TraditionalModePanel::paint(juce::Graphics& g)
{
    // Draw instrument body outline
    auto bounds = getLocalBounds();
    
    g.setColour(juce::Colours::saddlebrown.withAlpha(0.3f));
    g.fillRoundedRectangle(bounds.toFloat(), 10.0f);
    
    g.setColour(juce::Colours::peru.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.toFloat(), 10.0f, 2.0f);
}

void TraditionalModePanel::resized()
{
    auto bounds = getLocalBounds().reduced(10, 5);
    
    if (bounds.getWidth() > 0 && numStrings > 0)
    {
        int stringWidth = bounds.getWidth() / numStrings;
        
        for (int i = 0; i < numStrings; ++i)
        {
            if (stringControls[i] != nullptr)
            {
                auto stringBounds = bounds.removeFromLeft(stringWidth).reduced(2, 0);
                stringControls[i]->setBounds(stringBounds);
            }
        }
    }
}
