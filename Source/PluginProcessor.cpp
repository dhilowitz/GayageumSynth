/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

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
    
    // Play mode control
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("playMode",1),
        "Free Play Mode",
        false  // Default to traditional mode
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
    
    // Initialize all voices
    for (auto& string : strings)
    {
        string.prepare(sampleRate);
    }
    
    // Initialize voice allocation
    for (auto& voice : voices)
    {
        voice.midiNote = -1;
        voice.stringIndex = -1;
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
    
    // Initialize remaining voices for free play mode
    for (int i = numStrings; i < maxVoices; ++i)
    {
        strings[i].setFrequency(440.0f);  // Default to A4
        strings[i].setDamping(0.7f);
    }
    
    updateStringParameters();
}

void Gayageum1AudioProcessor::updateStringParameters()
{
    // Check play mode
    bool freePlayMode = apvts.getRawParameterValue("playMode")->load() > 0.5f;
    
    // Update each string's Anjok position from parameters (traditional mode only)
    if (!freePlayMode)
    {
        for (int i = 0; i < numStrings; ++i)
        {
            auto paramID = "anjok" + juce::String(i + 1);
            float position = apvts.getRawParameterValue(paramID)->load();
            strings[i].setAnjokPosition(position);
        }
    }
    
    // Update global damping (both modes)
    float damping = apvts.getRawParameterValue("damping")->load();
    for (auto& string : strings)
    {
        string.setDamping(damping);
    }
    
    // Update excitation blend (both modes)
    float excitationBlend = apvts.getRawParameterValue("excitationBlend")->load();
    for (auto& string : strings)
    {
        string.setExcitationBlend(excitationBlend);
    }
    
    // Update body resonance (both modes)
    float bodyResonance = apvts.getRawParameterValue("bodyResonance")->load();
    bodyResonator.setResonanceAmount(bodyResonance);
}

int Gayageum1AudioProcessor::findFreeVoice()
{
    int voice = nextVoiceIndex;
    nextVoiceIndex = (nextVoiceIndex + 1) % maxVoices;
    return voice;
}

int Gayageum1AudioProcessor::findVoiceForNote(int midiNote)
{
    for (int i = 0; i < maxVoices; ++i)
    {
        if (voices[i].midiNote == midiNote)
            return i;
    }
    return -1;
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
    
    // Get play mode once for the entire block
    bool freePlayMode = apvts.getRawParameterValue("playMode")->load() > 0.5f;

    // Process MIDI messages to trigger strings
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        
        if (message.isNoteOn())
        {
            int noteNumber = message.getNoteNumber();
            float velocity = message.getFloatVelocity();
            
            if (freePlayMode)
            {
                // Free play mode: allocate a voice and set frequency directly from MIDI note
                int voiceIndex = findFreeVoice();
                voices[voiceIndex].midiNote = noteNumber;
                voices[voiceIndex].stringIndex = voiceIndex;
                
                // Calculate frequency from MIDI note number: f = 440 * 2^((n-69)/12)
                float frequency = 440.0f * std::pow(2.0f, (noteNumber - 69) / 12.0f);
                strings[voiceIndex].setFrequency(frequency);
                
                // Reapply damping after frequency change (damping compensation depends on frequency)
                float damping = apvts.getRawParameterValue("damping")->load();
                strings[voiceIndex].setDamping(damping);
                
                strings[voiceIndex].trigger(velocity);
            }
            else
            {
                // Traditional mode: map MIDI notes to the 12 gayageum strings
                // MIDI note 60 (C4) maps to string 0, etc.
                int stringIndex = (noteNumber - 60) % numStrings;
                if (stringIndex < 0) stringIndex += numStrings;
                
                strings[stringIndex].trigger(velocity);
            }
        }
        else if (message.isNoteOff())
        {
            if (freePlayMode)
            {
                // Free play mode: release the voice
                int noteNumber = message.getNoteNumber();
                int voiceIndex = findVoiceForNote(noteNumber);
                if (voiceIndex != -1)
                {
                    voices[voiceIndex].midiNote = -1;
                    voices[voiceIndex].stringIndex = -1;
                    // Optionally: trigger a quick release envelope here
                }
            }
            // Traditional mode: notes sustain until they naturally decay
        }
    }

    // Process audio samples
    int numSamples = buffer.getNumSamples();
    int activeVoices = freePlayMode ? maxVoices : numStrings;
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float output = 0.0f;
        
        // Sum active voices
        for (int voiceNum = 0; voiceNum < activeVoices; ++voiceNum)
        {
            output += strings[voiceNum].processSample(0.0f);
        }
        
        // Scale output to prevent clipping
        output *= 0.15f;  // We use the same scaling factor for traditional and free mode, even though free mode has more voices.
        
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
