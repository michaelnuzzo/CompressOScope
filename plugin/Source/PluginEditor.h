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

    void plot(juce::Graphics& g, juce::Rectangle<int> rect);
    float fract(float v);

private:
    void timerCallback() override;
    NewProjectAudioProcessor& audioProcessor;
    juce::AudioBuffer<float> windowBuffer;
    juce::Rectangle<int> window;
    juce::Label timeLabel, db1Label, db2Label, compressionLabel, freezeLabel;
    juce::Slider timeKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> timeAttachment;
    std::array<std::unique_ptr<juce::Slider>,2> dbKnobs;
    juce::ToggleButton compressionButton, freezeButton;
    juce::Colour palette[2] {juce::Colours::lightblue, juce::Colours::red};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NewProjectAudioProcessorEditor);
};
