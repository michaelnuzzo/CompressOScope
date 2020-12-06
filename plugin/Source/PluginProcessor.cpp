/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NewProjectAudioProcessor::NewProjectAudioProcessor()
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
                    , displayCollector(1), audioCollector(1)
                    , parameters(*this, nullptr, "Parameters", createParameters())
{
    displayCollector.setIsOverwritable(true);
    audioCollector.setIsOverwritable(true);
    parameters.state = juce::ValueTree("Parameters");
}

NewProjectAudioProcessor::~NewProjectAudioProcessor()
{
}

//==============================================================================
const juce::String NewProjectAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool NewProjectAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool NewProjectAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool NewProjectAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double NewProjectAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int NewProjectAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int NewProjectAudioProcessor::getCurrentProgram()
{
    return 0;
}

void NewProjectAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String NewProjectAudioProcessor::getProgramName (int index)
{
    return {};
}

void NewProjectAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void NewProjectAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    minmaxBuffer.setSize(getTotalNumInputChannels(), 2);
    displayCollector.reset();

    audioCollector.resize(sampleRate*5);

    juce::AudioBuffer<float> initializeWithZeros;
    initializeWithZeros.setSize(getTotalNumInputChannels(), displayCollector.getTotalSize()-1);
    initializeWithZeros.clear();
    displayCollector.push(initializeWithZeros);

    setUpdate();
}

void NewProjectAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NewProjectAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
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

void NewProjectAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if(requiresUpdate)
    {
        updateParameters();
    }


    audioCollector.push(buffer);

    // TODO: if timeSlize < 0, we should have a different condition
    // so we can interpolate between zoomed samples
    while(audioCollector.getNumUnread() > timeSlice)
    {
        audioCollector.pop(tmp);
        auto tmpBlock = juce::dsp::AudioBlock<float>(tmp);

        for(int ch = 0; ch < getTotalNumInputChannels(); ch++)
        {
            auto minmax = tmpBlock.getSubsetChannelBlock(ch, 1).findMinAndMax();
            minmaxBuffer.setSample(ch, 0, minmax.getStart());
            minmaxBuffer.setSample(ch, 1, minmax.getEnd());
        }
        displayCollector.push(minmaxBuffer);
        while(displayCollector.getNumUnread() > numPixels + 20)
        {
            displayCollector.trim(1);
        }
    }
}

void NewProjectAudioProcessor::updateParameters()
{
    timeSlice = *parameters.getRawParameterValue("TIME")/numPixels * getSampleRate() * 2;
    if(timeSlice >= 1)
    {
        tmp.setSize(getTotalNumInputChannels(), timeSlice);
    }
    else if(tmp.getNumSamples() != 1)
    {
        tmp.setSize(getTotalNumInputChannels(), 1);
    }

    // if the window size has been changed
    if(numPixels*2 != displayCollector.getTotalSize())
    {
        // We double the size to leave extra room to account for
        // concurrent access between the audio and graphics threads
        displayCollector.resize(numPixels*2);
    }

    requiresUpdate = false;
}

juce::AudioProcessorValueTreeState::ParameterLayout NewProjectAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TIME","Time", juce::NormalisableRange<float>(0.001f,5.f,0.001f), 1.f));
    return { params.begin(), params.end() };
}

//==============================================================================
bool NewProjectAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* NewProjectAudioProcessor::createEditor()
{
    return new NewProjectAudioProcessorEditor (*this);
}

//==============================================================================
void NewProjectAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    std::unique_ptr<juce::XmlElement> xml(parameters.state.createXml());
    copyXmlToBinary(*xml, destData);
}

void NewProjectAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if(xmlState.get() != nullptr)
    {
        if(xmlState->hasTagName(parameters.state.getType()))
        {
            parameters.state = juce::ValueTree::fromXml(*xmlState);
        }
    }
    setUpdate();
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NewProjectAudioProcessor();
}
