/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// BodyResonator Implementation
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

//==============================================================================
// GayageumString Implementation
//==============================================================================

GayageumString::GayageumString()
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

void GayageumString::prepare(double sr)
{
    sampleRate = sr;
    reset();
}

void GayageumString::reset()
{
    delayLine.fill(0.0f);
    excitationBuffer.fill(0.0f);
    writeIndex = 0;
    filterState = 0.0f;
    excitationIndex = 0;
    isExciting = false;
}

void GayageumString::setFrequency(float frequency)
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

void GayageumString::setAnjokPosition(float position)
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

void GayageumString::setAnjokSlope(float slope)
{
    // Slope parameters from Table 1, Section 3.3
    // String 2: 887.980, String 5: 987.880, String 8: 1105.700, String 11: 1140.900
    anjokSlope = slope;
}

float GayageumString::calculateFrequencyFromAnjok()
{
    // Section 3.3 - Leaky integrator method
    // This is the most accurate method from the paper
    // The frequency is calculated using the slope parameter and position
    
    // The relationship is: f = slope / position
    // This comes from the inverse relationship between string length and frequency
    float frequency = anjokSlope / anjokPosition;
    
    return frequency;
}

void GayageumString::setExcitationBlend(float blend)
{
    excitationBlend = juce::jlimit(0.0f, 1.0f, blend);
}

void GayageumString::setDamping(float damping)
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

void GayageumString::trigger(float velocity)
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

float GayageumString::onePoleFilter(float input)
{
    // One-pole loop filter from Equation 6:
    // H(z) = g(1 + α1) / (1 + α1*z^-1)
    float output = filterGain * (input + filterCoeff * filterState) / (1.0f + filterCoeff);
    filterState = input;
    return output;
}

float GayageumString::lagrangeInterpolation(float delayInSamples)
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

float GayageumString::processSample(float input)
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

//==============================================================================
Gayageum1AudioProcessor::Gayageum1AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
    , apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

Gayageum1AudioProcessor::~Gayageum1AudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout Gayageum1AudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Create Anjok position parameters for each of the 12 strings
    for (int i = 0; i < numStrings; ++i)
    {
        auto paramID = "anjok" + juce::String(i + 1);
        auto paramName = "String " + juce::String(i + 1) + " Anjok";
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(paramID, 1),
            paramName,
            juce::NormalisableRange<float>(0.3f, 0.9f, 0.001f),
            0.6f,  // Default position (middle)
            "m"    // Unit: meters
        ));
    }
    
    // Global damping control
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("damping",1),
        "Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.7f
    ));
    
    // Body resonance control
    layout.add(std::make_unique<juce::AudioParameterFloat>(
         juce::ParameterID("bodyResonance",1),
        "Body Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));
    
    // Excitation blend control
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("excitationBlend",1),
        "Excitation Type",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        1.0f  // Default to noise burst
    ));
    
    return layout;
}

//==============================================================================
const juce::String Gayageum1AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Gayageum1AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool Gayageum1AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool Gayageum1AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double Gayageum1AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Gayageum1AudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int Gayageum1AudioProcessor::getCurrentProgram()
{
    return 0;
}

void Gayageum1AudioProcessor::setCurrentProgram (int index)
{
}

const juce::String Gayageum1AudioProcessor::getProgramName (int index)
{
    return {};
}

void Gayageum1AudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void Gayageum1AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    // Initialize all 12 strings
    for (auto& string : strings)
    {
        string.prepare(sampleRate);
    }
    
    // Initialize body resonator
    bodyResonator.prepare(sampleRate);
    
    // Default tuning for the 12 strings (traditional gayageum tuning)
    const float defaultTuning[12] = {
        164.81f,  // E3  - String 1
        185.00f,  // F#3 - String 2
        220.00f,  // A3  - String 3
        246.94f,  // B3  - String 4
        277.18f,  // C#4 - String 5
        329.63f,  // E4  - String 6
        369.99f,  // F#4 - String 7
        440.00f,  // A4  - String 8
        493.88f,  // B4  - String 9
        554.37f,  // C#5 - String 10
        659.25f,  // E5  - String 11
        739.99f   // F#5 - String 12
    };
    
    // Default Anjok positions (middle of range, 0.6m)
    const float defaultPosition = 0.6f;
    
    // Calculate slope for each string based on desired frequency and default position
    // Using the formula: f = slope / position, so slope = f * position
    for (int i = 0; i < numStrings; ++i)
    {
        float slope = defaultTuning[i] * defaultPosition;
        strings[i].setAnjokSlope(slope);
        strings[i].setAnjokPosition(defaultPosition);  // This sets currentFrequency via calculateFrequencyFromAnjok
        strings[i].setDamping(0.7f);  // Apply damping AFTER frequency is set
    }
    
    updateStringParameters();
}

void Gayageum1AudioProcessor::updateStringParameters()
{
    // Update each string's Anjok position from parameters
    for (int i = 0; i < numStrings; ++i)
    {
        auto paramID = "anjok" + juce::String(i + 1);
        float position = apvts.getRawParameterValue(paramID)->load();
        strings[i].setAnjokPosition(position);
    }
    
    // Update global damping
    float damping = apvts.getRawParameterValue("damping")->load();
    for (auto& string : strings)
    {
        string.setDamping(damping);
    }
    
    // Update excitation blend
    float excitationBlend = apvts.getRawParameterValue("excitationBlend")->load();
    for (auto& string : strings)
    {
        string.setExcitationBlend(excitationBlend);
    }
    
    // Update body resonance
    float bodyResonance = apvts.getRawParameterValue("bodyResonance")->load();
    bodyResonator.setResonanceAmount(bodyResonance);
}

void Gayageum1AudioProcessor::releaseResources()
{
    // Reset all strings when playback stops
    for (auto& string : strings)
    {
        string.reset();
    }
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Gayageum1AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void Gayageum1AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Update string parameters from APVTS (Anjok positions and damping)
    updateStringParameters();

    // Process MIDI messages to trigger strings
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        
        if (message.isNoteOn())
        {
            int noteNumber = message.getNoteNumber();
            float velocity = message.getFloatVelocity();
            
            // Map MIDI notes to the 12 gayageum strings
            // MIDI note 60 (C4) maps to string 0, etc.
            int stringIndex = (noteNumber - 60) % numStrings;
            if (stringIndex < 0) stringIndex += numStrings;
            
            DBG("String " + juce::String(stringIndex) + "triggered.f");
            
            strings[stringIndex].trigger(velocity);
        }
    }

    // Process audio samples
    int numSamples = buffer.getNumSamples();
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float output = 0.0f;
        
        // Sum all 12 strings
        for (int stringNum = 0; stringNum < numStrings; ++stringNum)
        {
            output += strings[stringNum].processSample(0.0f);
        }
        
        // Scale output to prevent clipping (12 strings)
        output *= 0.15f;
        
        // Process through body resonator
        output = bodyResonator.processSample(output);
        
        // Write to all output channels
        for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        {
            buffer.setSample(channel, sample, output);
        }
    }
}

//==============================================================================
bool Gayageum1AudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* Gayageum1AudioProcessor::createEditor()
{
    return new Gayageum1AudioProcessorEditor (*this);
}

//==============================================================================
void Gayageum1AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void Gayageum1AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Gayageum1AudioProcessor();
}
