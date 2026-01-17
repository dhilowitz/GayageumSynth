/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "GayageumString.h"
#include "BodyResonator.h"

//==============================================================================
/**
*/
class Gayageum1AudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    Gayageum1AudioProcessor();
    ~Gayageum1AudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    //==============================================================================
    // Get the parameter layout for APVTS
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Public access to parameters for the editor
    juce::AudioProcessorValueTreeState apvts;

private:
    //==============================================================================
    // 12 strings for the gayageum
    static constexpr int numStrings = 12;
    std::array<G1GayageumString, numStrings> strings;
    
    // Body resonator
    G1BodyResonator bodyResonator;
    
    double currentSampleRate = 44100.0;
    
    // Update string parameters from APVTS
    void updateStringParameters();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Gayageum1AudioProcessor)
};
