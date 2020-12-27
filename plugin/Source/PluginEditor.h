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
    juce::AudioBuffer<float> windowBuffer;
    juce::AudioBuffer<float> DEBUG_BUFFER;
    juce::Rectangle<int> window;
    /* parameters */
    juce::Label timeLabel, filterLabel, compressionLabel, freezeLabel, yMinLabel, yMaxLabel;
    juce::Slider timeKnob, filterKnob, yMinKnob, yMaxKnob;
    std::array<std::unique_ptr<juce::Slider>,2> gainKnobs;
    std::array<std::unique_ptr<juce::Label>,2> gainLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,2> gainAttachments;
    juce::ToggleButton compressionButton, freezeButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> timeAttachment, filterAttachment, yMinAttachment, yMaxAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> compressionAttachment, freezeAttachment;
    juce::Colour palette[4] {juce::Colours::dodgerblue, juce::Colours::firebrick, juce::Colours::lightgreen, juce::Colours::green};
    juce::Font f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NewProjectAudioProcessorEditor);
};
