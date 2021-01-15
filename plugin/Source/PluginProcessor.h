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

    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    juce::AudioProcessorValueTreeState& getParameters() {return parameters;}

    /* my functions */
    inline void setUpdate() {requiresUpdate = true;}
    inline void setNumPixels(int num) {numPixels = num;}
    inline void setGuiReady(bool r) {guiReady = r;}
    inline double getNumSamplesPerPixel() {return samplesPerPixel;}
    inline int getState() {return state;}
    inline bool isDoneProcessing() {return !isInUse;}

    void updateParameters();
    void interpolate(const juce::dsp::AudioBlock<float> inBlock, juce::dsp::AudioBlock<float>& outBlock, float numInterps, int type = 0);

    const int NUM_CH; // we require 2 channels to run the compressoscope!
    ASyncBuffer displayCollector; // we are going to access this from the graphics thread (yes, i know)

private:
    ASyncBuffer audioCollector; // collects raw audio data circularly
    juce::AudioBuffer<float> inBuffer; // stores data read from the audiocollector
    juce::AudioBuffer<float> outBuffer; // stores the processed samples and pushes them to the display collector
    juce::AudioBuffer<float> copyBuffer; // copies from the buffer to the collector
    MedianFilter medianFilter; // smooths the data
    bool smoothing; // is smoothing on?
    double samplesPerPixel;
    int numPixels; // width of the waveform display window
    int state; // switches between methods of converting the audio data to display data
    bool guiReady; // has the gui been initialized?
    bool isInUse; // are we writing to the display window?
    unsigned long counter;
    bool requiresUpdate; // have the VST parameters changed?
    juce::AudioProcessorValueTreeState parameters; // stores the current state of the VST for saving

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressOScopeAudioProcessor)
};
