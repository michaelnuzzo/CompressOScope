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
class NewProjectAudioProcessorEditor  : public juce::AudioProcessorEditor, juce::Timer//, juce::Slider::Listener
{
public:
    NewProjectAudioProcessorEditor (NewProjectAudioProcessor&);
    ~NewProjectAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void plot(juce::Graphics& g, juce::Rectangle<int> rect);
    void getMinAndMaxOrdered(juce::dsp::AudioBlock<float> block, float &val1, float &val2);
    float fract(float v);

//    void sliderValueChanged (juce::Slider* slider) override;

private:
    void timerCallback() override;
    NewProjectAudioProcessor& audioProcessor;
    juce::AudioBuffer<float> displayBuffer;
    int padding = 100;
    juce::Rectangle<int> window;
    juce::Slider timeKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> timeAttachment;
    std::array<std::unique_ptr<juce::Slider>,2> dbKnobs;
    juce::Colour palette[2] {juce::Colours::lightblue, juce::Colours::red};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NewProjectAudioProcessorEditor);
};
