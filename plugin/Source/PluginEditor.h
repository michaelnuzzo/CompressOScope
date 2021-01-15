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
class CompressOScopeAudioProcessorEditor  : public juce::AudioProcessorEditor, juce::Timer
{
public:
    CompressOScopeAudioProcessorEditor (CompressOScopeAudioProcessor&);
    ~CompressOScopeAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void write(juce::String txt, int xPos, int yPos, juce::Justification j, juce::Graphics& g);
    void plot(juce::Graphics& g);
    void prepareFilledLine(float v, float vNext, float vMin, float vMinNext, float &out1, float &out2);

private:
    void timerCallback() override;
    CompressOScopeAudioProcessor& audioProcessor;
    juce::AudioBuffer<float> displayBuffer;
    juce::Rectangle<int> window;
    /* parameters */
    juce::Label timeLabel, filterLabel, compressionLabel, freezeLabel, smoothingLabel, yMinLabel, yMaxLabel;
    juce::Slider timeKnob, filterKnob, yMinKnob, yMaxKnob;
    std::array<std::unique_ptr<juce::Slider>,2> gainKnobs;
    std::array<std::unique_ptr<juce::Label>,2> gainLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,2> gainAttachments;
    juce::ToggleButton compressionButton, freezeButton, smoothingButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> timeAttachment, filterAttachment, yMinAttachment, yMaxAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> compressionAttachment, freezeAttachment, smoothingAttachment;
    juce::Colour palette[4] {juce::Colours::dodgerblue, juce::Colours::firebrick, juce::Colours::lightgreen, juce::Colours::green};
    juce::Font f;
    juce::Image logo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressOScopeAudioProcessorEditor)
};
