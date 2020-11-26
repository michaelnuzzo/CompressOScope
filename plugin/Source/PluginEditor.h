/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class NewProjectAudioProcessorEditor  : public juce::AudioProcessorEditor, juce::Timer
{
public:
    NewProjectAudioProcessorEditor (NewProjectAudioProcessor&);
    ~NewProjectAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void plot(juce::Graphics& g);

private:
    void timerCallback() override;
    NewProjectAudioProcessor& audioProcessor;
    juce::AudioBuffer<float> displayBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NewProjectAudioProcessorEditor);
};
