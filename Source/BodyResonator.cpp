/*
  ==============================================================================

    BodyResonator.cpp
    Body Resonance Model for Gayageum
    Simulates the paulownia wood body's resonant characteristics

  ==============================================================================
*/

#include "BodyResonator.h"

//==============================================================================
BodyResonator::BodyResonator()
    : resonanceAmount(0.5f)
    , sampleRate(44100.0)
{
}

void BodyResonator::prepare(double sr)
{
    sampleRate = sr;
    
    // Set up resonance peaks based on typical gayageum body characteristics
    // Paulownia wood body has characteristic resonances
    // These frequencies are approximations based on similar instruments
    
    const float frequencies[numResonances] = {
        180.0f,   // Low fundamental body resonance
        380.0f,   // Secondary resonance
        720.0f,   // Mid-range resonance
        1200.0f,  // Upper mid resonance
        2400.0f   // High frequency air resonance
    };
    
    const float Q_values[numResonances] = {
        15.0f,    // Higher Q = more pronounced resonance
        18.0f,
        12.0f,
        10.0f,
        8.0f
    };
    
    const float gains[numResonances] = {
        4.5f,     // Much stronger low resonance
        3.5f,
        2.8f,
        2.2f,
        1.5f
    };
    
    for (int i = 0; i < numResonances; ++i)
    {
        resonators[i].setResonance(sampleRate, frequencies[i], Q_values[i], gains[i]);
    }
}

void BodyResonator::reset()
{
    for (auto& resonator : resonators)
    {
        resonator.reset();
    }
}

float BodyResonator::processSample(float input)
{
    // Sum all resonant peaks
    float resonantOutput = 0.0f;
    
    for (auto& resonator : resonators)
    {
        resonantOutput += resonator.process(input);
    }
    
    // Mix dry and resonant signal - much more pronounced wet signal
    float dry = input * (1.0f - resonanceAmount * 0.6f);  // Reduce dry as resonance increases
    float wet = resonantOutput * resonanceAmount * 0.35f;  // Stronger wet signal
    
    return dry + wet;
}

void BodyResonator::setResonanceAmount(float amount)
{
    resonanceAmount = juce::jlimit(0.0f, 1.0f, amount);
}

void BodyResonator::ResonantFilter::setResonance(double sampleRate, float frequency, float Q, float gain)
{
    // Design a peaking EQ filter (biquad) for resonance
    float w0 = juce::MathConstants<float>::twoPi * frequency / static_cast<float>(sampleRate);
    float alpha = std::sin(w0) / (2.0f * Q);
    float A = std::sqrt(gain);
    
    float cosw0 = std::cos(w0);
    
    // Peaking filter coefficients
    b0 = 1.0f + alpha * A;
    b1 = -2.0f * cosw0;
    b2 = 1.0f - alpha * A;
    float a0 = 1.0f + alpha / A;
    a1 = -2.0f * cosw0;
    a2 = 1.0f - alpha / A;
    
    // Normalize by a0
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;
}

float BodyResonator::ResonantFilter::process(float input)
{
    // Direct Form II transposed
    float output = b0 * input + x1;
    x1 = b1 * input - a1 * output + x2;
    x2 = b2 * input - a2 * output;
    
    return output;
}

void BodyResonator::ResonantFilter::reset()
{
    x1 = x2 = y1 = y2 = 0.0f;
}
