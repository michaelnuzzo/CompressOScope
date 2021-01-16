/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CompressOScopeAudioProcessor::CompressOScopeAudioProcessor()
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
                    , NUM_CH(2), displayCollector((NUM_CH + 1) * 2, 1), audioCollector(NUM_CH + 1, 1), medianFilter(1), guiReady(false), isInUse(false)
                    , parameters(*this, nullptr, "Parameters", createParameters())
{
    inBuffer.setSize(NUM_CH + 1, 1);
    outBuffer.setSize((NUM_CH + 1) * 2, 1);
    copyBuffer.setSize(NUM_CH + 1, 1);

    displayCollector.setIsOverwritable(true);
    audioCollector.setIsOverwritable(true);
    parameters.state = juce::ValueTree("Parameters");
}

CompressOScopeAudioProcessor::~CompressOScopeAudioProcessor()
{
}

//==============================================================================
const juce::String CompressOScopeAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CompressOScopeAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool CompressOScopeAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool CompressOScopeAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double CompressOScopeAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CompressOScopeAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int CompressOScopeAudioProcessor::getCurrentProgram()
{
    return 0;
}

void CompressOScopeAudioProcessor::setCurrentProgram (int /*index*/)
{
}

const juce::String CompressOScopeAudioProcessor::getProgramName (int /*index*/)
{
    return {};
}

void CompressOScopeAudioProcessor::changeProgramName (int /*index*/, const juce::String& /*newName*/)
{
}

//==============================================================================
void CompressOScopeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    displayCollector.reset();

    audioCollector.resize(int(sampleRate)*5);

    copyBuffer.setSize(copyBuffer.getNumChannels(), samplesPerBlock);

    setUpdate();
}

void CompressOScopeAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CompressOScopeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())

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

void CompressOScopeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
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

    //==========================================================================================//

    /* save incoming audio */

    auto rawBuffer = juce::dsp::AudioBlock<float>(buffer); // the incoming buffer
    auto copyBlock = juce::dsp::AudioBlock<float>(copyBuffer).getSubBlock(0, size_t(buffer.getNumSamples())); // storage container for the audio + compression data
    auto audioCopyBlock = copyBlock.getSubsetChannelBlock(0, size_t(NUM_CH));
    auto compCopyBlock  = copyBlock.getSubsetChannelBlock(2, 1);

    audioCopyBlock.copyFrom(rawBuffer);

    auto in1 = audioCopyBlock.getChannelPointer(0);
    auto in2 = audioCopyBlock.getChannelPointer(1);
    auto out  = compCopyBlock.getChannelPointer(0);
    for(int i = 0; i < buffer.getNumSamples(); i++)
    {
        float compVal = abs(in2[i]/in1[i]);
        if(smoothing)
        {
            medianFilter.push(compVal);
            out[i] = medianFilter.getMedian();
        }
        else
        {
            out[i] = compVal;
        }
    }

    audioCollector.push(copyBlock);

    //==========================================================================================//

    /* process data and send to display */
    isInUse = true;
    while(audioCollector.getNumUnread() > inBuffer.getNumSamples())
    {
        auto inBlock = juce::dsp::AudioBlock<float>(inBuffer); // used to process samples read from collector
        auto outBlock = juce::dsp::AudioBlock<float>(outBuffer); // used to collect processed samples and push to the display
        auto outValBlock = outBlock.getSubsetChannelBlock(0, size_t(NUM_CH + 1));
        auto outMinBlock = outBlock.getSubsetChannelBlock(size_t(NUM_CH + 1), size_t(NUM_CH + 1));
        outMinBlock.fill(NAN);
        int numToRead = int(counter*(samplesPerPixel)) - int((counter-1)*(samplesPerPixel));
        int numToWrite;

        /* process zoomed audio data */

        // samples/pixels is 1
        if(state == 1 || (state == 2 && numToRead == 1))
        {
            audioCollector.pop(inBlock,numToRead,numToRead);
            inBlock = inBlock.getSubBlock(0, 1);
            outValBlock = outValBlock.getSubBlock(0, 1);
            outValBlock.copyFrom(inBlock);
            numToWrite = 1;
        }
        // samples/pixels is >1
        else if(state == 2)
        {
            audioCollector.pop(inBlock,numToRead,numToRead);
            inBlock = inBlock.getSubBlock(0, size_t(numToRead));

            for(size_t ch = 0; ch < size_t(NUM_CH + 1); ch++)
            {
                auto curCh = inBlock.getSubsetChannelBlock(ch, 1);
                juce::Range<float> minmax = curCh.findMinAndMax();
                outValBlock.setSample(int(ch), 0, minmax.getStart());
                outMinBlock.setSample(int(ch), 0, minmax.getEnd());
            }
            numToWrite = 1;
        }
        // samples/pixels is <1
        else if(state == 3)
        {
            numToRead = 2;
            audioCollector.pop(inBlock,numToRead,numToRead - 1);
            numToWrite = int(counter*(1/samplesPerPixel)) - int((counter-1)*(1/samplesPerPixel));
            interpolate(inBlock, outValBlock, numToWrite, 1);
        }
        else
        {
            numToWrite = 0;
        }

        displayCollector.push(outBuffer,-1,numToWrite);

        // We leave a 20 sample allowance to account for concurrent access
        while(displayCollector.getNumUnread() > numPixels + 20)
        {
            displayCollector.trim(1);
        }

        counter++;
    }
    isInUse = false;
}

void CompressOScopeAudioProcessor::updateParameters()
{
    while(!guiReady){} // if gui isn't ready then parameters will be junk

    //==========================================================================================//

    smoothing = bool(*parameters.getRawParameterValue("SMOOTHING"));
    if(smoothing)
    {
        medianFilter.setOrder(int(getSampleRate() * *parameters.getRawParameterValue("FILTER")/1000.f));
    }
    else
    {
        medianFilter.setOrder(1);
    }

    //==========================================================================================//

    samplesPerPixel = *parameters.getRawParameterValue("TIME")/numPixels * getSampleRate();

    // one sample per pixel
    if(samplesPerPixel == 1)
    {
        state = 1;
        // in this case, we simply write the audio buffer to the display buffer
        inBuffer.setSize(inBuffer.getNumChannels(), 1);
        outBuffer.setSize(displayCollector.getNumChannels(), 1); // does nothing
    }
    // multiple samples per pixel
    else if(samplesPerPixel > 1)
    {
        state = 2;
        // in this case, we need to find min and max values
        inBuffer.setSize(inBuffer.getNumChannels(), int(samplesPerPixel) + 2);
        outBuffer.setSize(displayCollector.getNumChannels(), 1); // stores min & max
    }
    // multiple pixels per sample
    else
    {
        state = 3;
        // in this case, we interpolate between the two samples that we have
        inBuffer.setSize(inBuffer.getNumChannels(), 2);
        outBuffer.setSize(displayCollector.getNumChannels(), int(1/(samplesPerPixel) + 2)); // stores interpolated samples
    }

    //==========================================================================================//

    // if the window size has been changed
    if(numPixels*2 != displayCollector.getTotalSize() && numPixels > 0)
    {
        // We double the size to leave extra room to account for
        // concurrent access between the audio and graphics threads
        displayCollector.resize(numPixels*2);
        if(displayCollector.getNumUnread() == 0)
        {
            juce::AudioBuffer<float> init;
            init.setSize(displayCollector.getNumChannels(), displayCollector.getTotalSize() - 1);
            juce::dsp::AudioBlock<float> initWithNAN;
            initWithNAN.fill(NAN);
            displayCollector.push(init);
        }
    }

    counter = 1;

    requiresUpdate = false;
}

void CompressOScopeAudioProcessor::interpolate(const juce::dsp::AudioBlock<float> inBlock, juce::dsp::AudioBlock<float>& outBlock, float n, int type)
{
    jassert(inBlock.getNumChannels() == outBlock.getNumChannels());
    jassert(inBlock.getNumSamples() == 2 && outBlock.getNumSamples() > 0);

    for(size_t ch = 0; ch < inBlock.getNumChannels(); ch++)
    {
        auto pi = inBlock.getChannelPointer(ch);
        auto po = outBlock.getChannelPointer(ch);

        for(size_t i = 0; i < n; i++)
        {
            if(type == 0) // nearest neighbor
            {
                po[i] = (i < n/2) ? pi[0] : pi[1];
            }
            else if(type == 1) // linear interpolation
            {
                po[i] = pi[0] + (pi[1] - pi[0]) * float(i) / float(n);
            }
        }
    }
}


juce::AudioProcessorValueTreeState::ParameterLayout CompressOScopeAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TIME"     , "Time"     , juce::NormalisableRange<float>(0.0001f, 5.f  , 0.0001f, 1/3.f), 1.f  ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("FILTER"   , "Filter"   , juce::NormalisableRange<float>(.1f    , 10.f , 0.001f        ), .1f  ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("GAIN1"    , "Gain 1"   , juce::NormalisableRange<float>(0.f    , 100.f, 0.001f        ), 0.f  ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("GAIN2"    , "Gain 2"   , juce::NormalisableRange<float>(0.f    , 100.f, 0.001f        ), 0.f  ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("YMAX"     , "Y max"    , juce::NormalisableRange<float>(-100.f , 60.f , 0.001f        ), 0.f  ));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("YMIN"     , "Y min"    , juce::NormalisableRange<float>(-100.f , 60.f , 0.001f        ), -54.f));
    params.push_back(std::make_unique<juce::AudioParameterBool >("COMPMODE" , "Comp Mode", false                                                                ));
    params.push_back(std::make_unique<juce::AudioParameterBool >("FREEZE"   , "Freeze"   , false                                                                ));
    params.push_back(std::make_unique<juce::AudioParameterBool >("SMOOTHING", "Smoothing", true                                                                 ));

    return { params.begin(), params.end() };
}

//==============================================================================
bool CompressOScopeAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* CompressOScopeAudioProcessor::createEditor()
{
    return new CompressOScopeAudioProcessorEditor (*this);
}

//==============================================================================
void CompressOScopeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    std::unique_ptr<juce::XmlElement> xml(parameters.state.createXml());
    copyXmlToBinary(*xml, destData);
}

void CompressOScopeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
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
    return new CompressOScopeAudioProcessor();
}
