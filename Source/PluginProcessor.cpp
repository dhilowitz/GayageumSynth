/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

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
        // Delay length = fs / f0 (Equation 4 from the paper)
        targetDelay = static_cast<float>(sampleRate / frequency);
        targetDelay = juce::jlimit(2.0f, static_cast<float>(maxDelayLength - 1), targetDelay);
    }
}

void GayageumString::setDamping(float damping)
{
    // damping: 0.0 = heavily damped, 1.0 = minimal damping
    filterCoeff = juce::jlimit(0.1f, 0.9f, damping);
    filterGain = juce::jlimit(0.99f, 0.9999f, 0.999f + (1.0f - damping) * 0.0009f);
}

void GayageumString::trigger(float velocity)
{
    // Create a simple pluck excitation signal
    // In a real implementation, this could be more sophisticated
    float vel = juce::jlimit(0.0f, 1.0f, velocity);
    
    for (int i = 0; i < excitationBuffer.size(); ++i)
    {
        // Simple triangular pluck shape
        float phase = static_cast<float>(i) / excitationBuffer.size();
        excitationBuffer[i] = vel * (phase < 0.5f ? phase * 2.0f : 2.0f - phase * 2.0f);
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
    float feedback = input + excitation + filtered * 0.995f;
    
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
{
}

Gayageum1AudioProcessor::~Gayageum1AudioProcessor()
{
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
    
    // Set default tuning for the 12 strings (approximate traditional tuning)
    // These are typical frequencies for sanjo gayageum
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
    
    for (int i = 0; i < numStrings; ++i)
    {
        strings[i].setFrequency(defaultTuning[i]);
        strings[i].setDamping(0.7f);
    }
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
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void Gayageum1AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Gayageum1AudioProcessor();
}
