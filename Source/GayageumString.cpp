/*
  ==============================================================================

    GayageumString.cpp
    Digital Waveguide String Model for Gayageum
    Based on Cho et al. (2007)

  ==============================================================================
*/

#include "GayageumString.h"

//==============================================================================
G1GayageumString::G1GayageumString()
    : writeIndex(0)
    , currentDelay(100.0f)
    , targetDelay(100.0f)
    , filterState(0.0f)
    , filterCoeff(0.5f)
    , filterGain(0.999f)
    , excitationIndex(0)
    , isExciting(false)
    , anjokPosition(0.6f)
    , anjokSlope(1000.0f)
    , baseFrequency(220.0f)
    , currentFrequency(220.0f)
    , excitationBlend(1.0f)
    , sampleRate(44100.0)
{
    delayLine.fill(0.0f);
    excitationBuffer.fill(0.0f);
}

void G1GayageumString::prepare(double sr)
{
    sampleRate = sr;
    reset();
}

void G1GayageumString::reset()
{
    delayLine.fill(0.0f);
    excitationBuffer.fill(0.0f);
    writeIndex = 0;
    filterState = 0.0f;
    excitationIndex = 0;
    isExciting = false;
}

void G1GayageumString::setFrequency(float frequency)
{
    if (frequency > 0.0f && frequency < sampleRate / 2.0f)
    {
        baseFrequency = frequency;
        currentFrequency = frequency;
        // Delay length = fs / f0 (Equation 4 from the paper)
        targetDelay = static_cast<float>(sampleRate / frequency);
        targetDelay = juce::jlimit(2.0f, static_cast<float>(maxDelayLength - 1), targetDelay);
    }
}

void G1GayageumString::setAnjokPosition(float position)
{
    // Position in meters, typically 0.3 to 0.9 from the paper
    anjokPosition = juce::jlimit(0.3f, 0.9f, position);
    
    // Recalculate frequency based on new Anjok position
    float newFrequency = calculateFrequencyFromAnjok();
    currentFrequency = newFrequency;
    
    // Update delay length
    if (newFrequency > 0.0f && newFrequency < sampleRate / 2.0f)
    {
        targetDelay = static_cast<float>(sampleRate / newFrequency);
        targetDelay = juce::jlimit(2.0f, static_cast<float>(maxDelayLength - 1), targetDelay);
    }
}

void G1GayageumString::setAnjokSlope(float slope)
{
    // Slope parameters from Table 1, Section 3.3
    // String 2: 887.980, String 5: 987.880, String 8: 1105.700, String 11: 1140.900
    anjokSlope = slope;
}

float G1GayageumString::calculateFrequencyFromAnjok()
{
    // Section 3.3 - Leaky integrator method
    // This is the most accurate method from the paper
    // The frequency is calculated using the slope parameter and position
    
    // The relationship is: f = slope / position
    // This comes from the inverse relationship between string length and frequency
    float frequency = anjokSlope / anjokPosition;
    
    return frequency;
}

void G1GayageumString::setExcitationBlend(float blend)
{
    excitationBlend = juce::jlimit(0.0f, 1.0f, blend);
}

void G1GayageumString::setDamping(float damping)
{
    // damping: 0.0 = heavily damped (short decay), 1.0 = minimal damping (long decay)
    // Much more extreme range for dramatic effect
    
    // Filter coefficient - controls high frequency damping
    filterCoeff = juce::jlimit(0.05f, 0.95f, damping * 0.95f + 0.05f);
    
    // Filter gain - controls overall decay time
    // Frequency-compensated: all strings should have equal decay time in seconds
    // Reference frequency: 165 Hz (lowest string)
    float referenceFreq = 165.0f;
    
    // Base gain for reference frequency: 0.93 to 0.9999
    float baseGain = 0.93f + damping * 0.0699f;
    
    // For equal decay time across all frequencies:
    // If reference string loops at referenceFreq Hz with gain baseGain,
    // and this string loops at currentFrequency Hz,
    // then to have equal decay time: gain^currentFreq = baseGain^referenceFreq
    // Therefore: gain = baseGain^(referenceFreq/currentFrequency)
    // 
    // Use a more aggressive exponent to compensate for the additional 0.9995 feedback in processSample
    float exponent = referenceFreq / juce::jmax(currentFrequency, 1.0f);
    exponent = exponent * 0.5f;  // Less aggressive - was making high strings sustain TOO long
    float compensatedGain = std::pow(baseGain, exponent);
    
    filterGain = juce::jlimit(0.93f, 0.99999f, compensatedGain);
}

void G1GayageumString::trigger(float velocity)
{
    float vel = juce::jlimit(0.0f, 1.0f, velocity);
    
    // Make excitation length proportional to the string's period
    int excitationLength = juce::jmin(128, static_cast<int>(targetDelay * 1.5f));
    excitationLength = juce::jmax(10, excitationLength);
    
    for (int i = 0; i < excitationBuffer.size(); ++i)
    {
        float triangleValue = 0.0f;
        float noiseValue = 0.0f;
        
        if (i < excitationLength)
        {
            // Triangle wave excitation (smoother, more tonal)
            float phase = static_cast<float>(i) / excitationLength;
            triangleValue = vel * (phase < 0.5f ? phase * 2.0f : 2.0f - phase * 2.0f);
            
            // Noise burst excitation (brighter, more percussive)
            float noise = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
            float envelope = 1.0f - (static_cast<float>(i) / excitationLength);
            noiseValue = vel * noise * envelope * 2.0f;
        }
        
        // Blend between triangle and noise based on excitationBlend parameter
        excitationBuffer[i] = triangleValue * (1.0f - excitationBlend) + noiseValue * excitationBlend;
    }
    
    excitationIndex = 0;
    isExciting = true;
}

float G1GayageumString::onePoleFilter(float input)
{
    // One-pole loop filter from Equation 6:
    // H(z) = g(1 + α1) / (1 + α1*z^-1)
    float output = filterGain * (input + filterCoeff * filterState) / (1.0f + filterCoeff);
    filterState = input;
    return output;
}

float G1GayageumString::lagrangeInterpolation(float delayInSamples)
{
    // 3rd-order Lagrange interpolation (N=3 in Equation 5)
    // This provides fractional delay for accurate pitch
    
    int intDelay = static_cast<int>(delayInSamples);
    float frac = delayInSamples - intDelay;
    
    // Read 4 samples around the delay point
    float y[4];
    for (int i = 0; i < 4; ++i)
    {
        int readPos = writeIndex - intDelay - i + 1;
        while (readPos < 0) readPos += maxDelayLength;
        readPos %= maxDelayLength;
        y[i] = delayLine[readPos];
    }
    
    // Lagrange interpolation formula
    float d = frac;
    float h0 = -d * (d - 1.0f) * (d - 2.0f) / 6.0f;
    float h1 = (d + 1.0f) * (d - 1.0f) * (d - 2.0f) / 2.0f;
    float h2 = -(d + 1.0f) * d * (d - 2.0f) / 2.0f;
    float h3 = (d + 1.0f) * d * (d - 1.0f) / 6.0f;
    
    return h0 * y[0] + h1 * y[1] + h2 * y[2] + h3 * y[3];
}

float G1GayageumString::processSample(float input)
{
    // Smoothly interpolate to target delay (for Anjok movement)
    currentDelay += (targetDelay - currentDelay) * 0.001f;
    
    // Add excitation if string was triggered
    float excitation = 0.0f;
    if (isExciting)
    {
        excitation = excitationBuffer[excitationIndex++];
        if (excitationIndex >= excitationBuffer.size())
            isExciting = false;
    }
    
    // Read from delay line with fractional delay (Lagrange interpolation)
    float delayOut = lagrangeInterpolation(currentDelay);
    
    // Apply one-pole loop filter for damping
    float filtered = onePoleFilter(delayOut);
    
    // Combine input, excitation, and feedback
    // Higher feedback value to allow better sustain, especially for high strings
    float feedback = input + excitation + filtered * 0.9995f;
    
    // Write to delay line
    delayLine[writeIndex] = feedback;
    writeIndex = (writeIndex + 1) % maxDelayLength;
    
    return filtered;
}
