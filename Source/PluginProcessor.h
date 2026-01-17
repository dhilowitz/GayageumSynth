/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Digital Waveguide String Model for Gayageum
    Based on Cho et al. (2007) - "Development of string model whose delay line 
    length controlled by Anjok in gayageum"
*/
class GayageumString
{
public:
    GayageumString();
    
    void prepare(double sampleRate);
    void reset();
    
    // Process one sample through the waveguide
    float processSample(float input);
    
    // Set the delay line length based on fundamental frequency
    void setFrequency(float frequency);
    
    // Set Anjok position (movable bridge) which controls string length
    // Position in meters: 0.3 to 0.9 (typical range from the paper)
    void setAnjokPosition(float position);
    
    // Set the slope parameter for Anjok frequency calculation
    // From Table 1, Section 3.3 of the paper
    void setAnjokSlope(float slope);
    
    // Trigger the string with an excitation signal
    void trigger(float velocity);
    
    // Set damping characteristics
    void setDamping(float damping);
    
private:
    // Lagrange interpolation for fractional delay
    float lagrangeInterpolation(float delayInSamples);
    
    // One-pole loop filter for frequency-dependent damping
    float onePoleFilter(float input);
    
    // Delay line buffer
    static constexpr int maxDelayLength = 4096;
    std::array<float, maxDelayLength> delayLine;
    int writeIndex;
    
    // Delay parameters
    float currentDelay;
    float targetDelay;
    
    // Filter state
    float filterState;
    float filterCoeff;  // α1 coefficient
    float filterGain;   // g coefficient
    
    // Excitation buffer (for pluck simulation)
    std::array<float, 128> excitationBuffer;
    int excitationIndex;
    bool isExciting;
    
    // Anjok (movable bridge) parameters
    float anjokPosition;        // Position in meters (0.3 - 0.9)
    float anjokSlope;          // Slope parameter from Section 3.3
    float baseFrequency;       // Reference frequency at default position
    float currentFrequency;    // Current frequency for damping compensation
    
    double sampleRate;
    
    // Calculate frequency from Anjok position using leaky integrator method
    float calculateFrequencyFromAnjok();
};

//==============================================================================
/**
    Body Resonator Model for Gayageum
    Simulates the paulownia wood body resonances using parallel biquad filters
*/
class BodyResonator
{
public:
    BodyResonator();
    
    void prepare(double sampleRate);
    void reset();
    
    // Process input through body resonances
    float processSample(float input);
    
    // Set the resonance strength (0.0 = no resonance, 1.0 = full resonance)
    void setResonanceAmount(float amount);
    
private:
    // Biquad filter for resonance peaks
    struct ResonantFilter
    {
        float b0, b1, b2, a1, a2;  // Filter coefficients
        float x1, x2, y1, y2;       // State variables
        
        ResonantFilter() : b0(1), b1(0), b2(0), a1(0), a2(0), x1(0), x2(0), y1(0), y2(0) {}
        
        void setResonance(double sampleRate, float frequency, float Q, float gain);
        float process(float input);
        void reset();
    };
    
    // Multiple resonance peaks for the body
    static constexpr int numResonances = 5;
    std::array<ResonantFilter, numResonances> resonators;
    
    float resonanceAmount;
    double sampleRate;
};

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
    std::array<GayageumString, numStrings> strings;
    
    // Body resonator
    BodyResonator bodyResonator;
    
    double currentSampleRate = 44100.0;
    
    // Update string parameters from APVTS
    void updateStringParameters();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Gayageum1AudioProcessor)
};
