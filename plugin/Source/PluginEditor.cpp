/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NewProjectAudioProcessorEditor::NewProjectAudioProcessorEditor (NewProjectAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (1000, 1000);
    startTimerHz(60);
    displayBuffer.setSize(p.getTotalNumInputChannels(), getWidth());
    displayBuffer.clear();
}

NewProjectAudioProcessorEditor::~NewProjectAudioProcessorEditor()
{
}

//==============================================================================
void NewProjectAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    g.setColour(juce::Colours::white);

    plot(g);
}

void NewProjectAudioProcessorEditor::resized()
{
}

void NewProjectAudioProcessorEditor::timerCallback()
{
    if(audioProcessor.collector.getNumUnread() >= displayBuffer.getNumSamples())
        audioProcessor.collector.readHead(displayBuffer);
    repaint();
}

void NewProjectAudioProcessorEditor::plot(juce::Graphics& g)
{

    auto center = getHeight()/2;
    auto w = getLocalBounds().getWidth();
    auto h = getLocalBounds().getHeight()/2;
    auto right = getLocalBounds().getRight();
    auto data = displayBuffer.getReadPointer(0);
    auto numSamples = displayBuffer.getNumSamples();

    for (int i = 1; i < getWidth(); i++)
    {
        float x1 = juce::jmap (i - 1, 0, numSamples - 1, right - w, right);
        float y1 = center - h * data[i - 1];
        float x2 = (float)juce::jmap (i, 0, numSamples - 1, right - w, right);
        float y2 = center - h * data[i];

        g.drawLine (x1,y1,x2,y2);
    }
}

