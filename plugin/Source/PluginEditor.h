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
    float fract(float v);

private:
    void timerCallback() override;
    NewProjectAudioProcessor& audioProcessor;
    juce::AudioBuffer<float> windowBuffer;
    juce::Rectangle<int> window;
    /* parameters */
    juce::Label timeLabel, gain1Label, gain2Label, compressionLabel, freezeLabel, yMinLabel, yMaxLabel;
    juce::Slider timeKnob, yMinKnob, yMaxKnob;
    std::array<std::unique_ptr<juce::Slider>,2> gainKnobs;
    juce::ToggleButton compressionButton, freezeButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> timeAttachment, gain1Attachment, gain2Attachment, yMinAttachment, yMaxAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> compressionAttachment, freezeAttachment;
    juce::Colour palette[2] {juce::Colours::lightblue, juce::Colours::red};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NewProjectAudioProcessorEditor);
};
