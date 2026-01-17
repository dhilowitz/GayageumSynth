/*
  ==============================================================================

    GayageumString.h
    Digital Waveguide String Model for Gayageum
    Based on Cho et al. (2007)

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
class G1GayageumString
{
public:
    G1GayageumString();
    
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
    
    // Set excitation blend (0.0 = triangle, 1.0 = noise burst)
    void setExcitationBlend(float blend);
    
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
    float excitationBlend;     // Blend between triangle (0.0) and noise (1.0)
    
    double sampleRate;
    
    // Calculate frequency from Anjok position using leaky integrator method
    float calculateFrequencyFromAnjok();
};
