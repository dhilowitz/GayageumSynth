/*
  ==============================================================================

    BodyResonator.h
    Body Resonance Model for Gayageum
    Simulates the paulownia wood body's resonant characteristics

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Body Resonance Model for Gayageum
    Simulates the paulownia wood body's resonant characteristics
*/
class BodyResonator
{
public:
    BodyResonator();
    
    void prepare(double sampleRate);
    void reset();
    
    float processSample(float input);
    void setResonanceAmount(float amount);
    
private:
    // Biquad filter for resonant peaks
    class ResonantFilter
    {
    public:
        ResonantFilter() : b0(1.0f), b1(0.0f), b2(0.0f), a1(0.0f), a2(0.0f),
                          x1(0.0f), x2(0.0f), y1(0.0f), y2(0.0f) {}
        
        void setResonance(double sampleRate, float frequency, float Q, float gain);
        float process(float input);
        void reset();
        
    private:
        float b0, b1, b2, a1, a2;  // Filter coefficients
        float x1, x2, y1, y2;       // State variables
    };
    
    static constexpr int numResonances = 5;
    std::array<ResonantFilter, numResonances> resonators;
    
    float resonanceAmount;
    double sampleRate;
};
