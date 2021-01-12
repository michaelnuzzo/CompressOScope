/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "ASyncBuffer.h"
#include "MedianFilter.h"

//==============================================================================
/**
*/
class CompressOScopeAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    CompressOScopeAudioProcessor();
    ~CompressOScopeAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    inline void setUpdate() {requiresUpdate = true;}
    inline void setNumPixels(int num) {numPixels = num;}
    inline double getNumSamplesPerPixel() {return samplesPerPixel;}
    inline int getState() {return state;}
    void updateParameters();

    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    juce::AudioProcessorValueTreeState& getParameters() {return parameters;}

    /* my functions */

    void getMinAndMaxOrdered(const juce::dsp::AudioBlock<float> inBlock, juce::dsp::AudioBlock<float>& outBlock);
    void interpolate(const juce::dsp::AudioBlock<float> inBlock, juce::dsp::AudioBlock<float>& outBlock, float numInterps, int type = 0);
    inline void setReady(bool r) {ready = r;}
    const int NUM_CH = 2; // we require 2 channels to run the compressoscope!

    ASyncBuffer displayCollector; // we are going to access this from the graphics thread (yes, i know)

private:
    ASyncBuffer audioCollector; // collects raw audio data circularly
    juce::AudioBuffer<float> inBuffer; // stores data read from the audiocollector
    juce::AudioBuffer<float> outBuffer; // stores the processed samples and pushes them to the display collector
    juce::AudioBuffer<float> copyBuffer;
    MedianFilter medianFilter;
    bool smoothing;
    double samplesPerPixel;
    int numPixels;
    int state;
    bool ready;
    unsigned long counter;
    bool requiresUpdate;
    juce::AudioProcessorValueTreeState parameters;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressOScopeAudioProcessor)
};
