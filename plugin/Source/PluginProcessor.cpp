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
    displayCollector.reset();

    audioCollector.resize(sampleRate*5);

    setUpdate();
}

void NewProjectAudioProcessor::releaseResources()
{
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

    while(audioCollector.getNumUnread() > audioBuffer.getNumSamples())
    {
        auto audioBlock = juce::dsp::AudioBlock<float>(audioBuffer);
        auto interBlock = juce::dsp::AudioBlock<float>(interBuffer);

        int numToWrite = int(counter*(samplesPerPixel)) - int((counter-1)*(samplesPerPixel));

        if(state == 1 || (state == 2 && numToWrite == 1))
        {
            audioCollector.pop(audioBlock,-1,1);
            interBlock.copyFrom(audioBlock);
        }
        else if(state == 2)
        {

            numToWrite = int(2*counter*(samplesPerPixel)) - int(2*(counter-1)*(samplesPerPixel));
            audioCollector.pop(audioBlock,-1,numToWrite);
            audioBlock = audioBlock.getSubBlock(0, numToWrite);
            getMinAndMaxOrdered(audioBlock, interBlock);
        }
        else if(state == 3)
        {
            audioCollector.pop(audioBlock,-1,1);
            interpolate(audioBlock, interBlock, 1);
        }

        displayCollector.push(interBuffer);

        // We leave an allowance to account for concurrent access
        while(displayCollector.getNumUnread() > numPixels + 20)
        {
            displayCollector.trim(1);
        }
        counter++;
    }
}

void NewProjectAudioProcessor::updateParameters()
{
    samplesPerPixel = *parameters.getRawParameterValue("TIME")/numPixels * getSampleRate();


    // one sample per pixel
    if(samplesPerPixel == 1)
    {
        state = 1;
        // in this case, we simply write the audio buffer to the display buffer
        interBuffer.setSize(getTotalNumInputChannels(), 1); // does nothing
        audioBuffer.setSize(getTotalNumInputChannels(), 1);
    }
    // multiple samples per pixel
    else if(samplesPerPixel > 1)
    {
        state = 2;
        // in this case, we need to find min and max values
        interBuffer.setSize(getTotalNumInputChannels(), 2); // stores min & max
        audioBuffer.setSize(getTotalNumInputChannels(), int(samplesPerPixel + 1) * 2);
    }
    // multiple pixels per sample
    else
    {
        state = 3;
        interBuffer.setSize(getTotalNumInputChannels(), int(2/(samplesPerPixel))); // stores interpolated samples
        audioBuffer.setSize(getTotalNumInputChannels(), 2);
    }

    // if the window size has been changed
    if(numPixels*2 != displayCollector.getTotalSize())
    {
        // We double the size to leave extra room to account for
        // concurrent access between the audio and graphics threads
        displayCollector.resize(numPixels*2);
        if(displayCollector.getNumUnread() == 0)
        {
            juce::AudioBuffer<float> initializeWithZeros;
            initializeWithZeros.setSize(getTotalNumInputChannels(), displayCollector.getTotalSize()-1);
            initializeWithZeros.clear();
            displayCollector.push(initializeWithZeros);
        }
    }

    counter = 1;

    requiresUpdate = false;
}

void NewProjectAudioProcessor::getMinAndMaxOrdered(const juce::dsp::AudioBlock<float> inBlock, juce::dsp::AudioBlock<float>& outBlock)
{
    jassert(inBlock.getNumChannels() == outBlock.getNumChannels());
    jassert(inBlock.getNumSamples() > 1 && outBlock.getNumSamples() == 2);

    // find the min and max values in the audio block, but preserve their order of appearance
    for(int ch = 0; ch < inBlock.getNumChannels(); ch++)
    {
        auto pi = inBlock.getChannelPointer(ch);
        auto po = outBlock.getChannelPointer(ch);

        float val1 = 1; // temp min
        float val2 = -1; // temp max
        int idx1 = 0;
        int idx2 = 0;
        for(int i = 0; i < inBlock.getNumSamples(); i++)
        {
            if(pi[i] < val1)
            {
                val1 = pi[i];
                idx1 = i;
            }
            if(pi[i] > val2)
            {
                val2 = pi[i];
                idx2 = i;
            }
        }
        if(idx1 > idx2)
        {
            std::swap(val1, val2);
        }
        po[0] = val1;
        po[1] = val2;
    }
    // val1 is either the min or the max depending on if it came first, and vice versa for val2
}

void NewProjectAudioProcessor::interpolate(const juce::dsp::AudioBlock<float> inBlock, juce::dsp::AudioBlock<float>& outBlock, int type)
{
    jassert(inBlock.getNumChannels() == outBlock.getNumChannels());
    jassert(inBlock.getNumSamples() == 2 && outBlock.getNumSamples() > 1);

    auto numInterps = outBlock.getNumSamples();

    for(int ch = 0; ch < inBlock.getNumChannels(); ch++)
    {
        auto pi = inBlock.getChannelPointer(ch);
        auto po = outBlock.getChannelPointer(ch);

        for(int i = 0; i < numInterps; i++)
        {
            if(type == 0) // nearest neighbor
            {
                po[i] = (i < numInterps/2) ? pi[0] : pi[1];
            }
            else if(type == 1) // linear interpolation
            {
                po[i] = pi[0] + (pi[1] - pi[0])*float(i)/float(numInterps);
            }
        }
    }
}


juce::AudioProcessorValueTreeState::ParameterLayout NewProjectAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TIME"    , "Time"     , juce::NormalisableRange<float>(0.0001f, 5.f  , 0.0001f, 1/3.f), 1.f  ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("GAIN1"   , "Gain 1"   , juce::NormalisableRange<float>(0.f    , 100.f, 0.001f        ), 0.f  ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("GAIN2"   , "Gain 2"   , juce::NormalisableRange<float>(0.f    , 100.f, 0.001f        ), 0.f  ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("YMAX"    , "Y max"    , juce::NormalisableRange<float>(-200.f , 20.f , 0.001f        ), 0.f  ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("YMIN"    , "Y min"    , juce::NormalisableRange<float>(-200.f , 20.f , 0.001f        ), -60.f));
    params.push_back(std::make_unique<juce::AudioParameterBool >("COMPMODE", "Comp Mode", false                                                                ));
    params.push_back(std::make_unique<juce::AudioParameterBool >("FREEZE"  , "Freeze"   , false                                                                ));
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
