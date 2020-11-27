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
    window.setSize(getWidth()-padding, getHeight()*2/3);
    window.setLeft(padding);

    timeKnob.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    timeKnob.setNormalisableRange(juce::NormalisableRange<double>(0.05f,5.f,0.01f));
    timeKnob.addListener(this);
    addAndMakeVisible(timeKnob);

    sampleBuffer.setSize(audioProcessor.getTotalNumInputChannels(), int(audioProcessor.getSampleRate()*timeKnob.getValue()));
    sampleBuffer.clear();

    displayBuffer.setSize(1, window.getWidth());
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

    g.drawRect(window);
    plot(g,window);

    timeKnob.setBounds(getWidth()/4, getHeight()-100, getWidth()/2, 100);
}

void NewProjectAudioProcessorEditor::resized()
{
}

void NewProjectAudioProcessorEditor::timerCallback()
{
    if(audioProcessor.collector.getNumUnread() >= sampleBuffer.getNumSamples())
        audioProcessor.collector.readHead(sampleBuffer);
    sampleBuffer.reverse(0, sampleBuffer.getNumSamples());

    float ratio = sampleBuffer.getNumSamples()/window.getWidth();
    float * out = displayBuffer.getWritePointer(0);
    auto sub = juce::dsp::AudioBlock<float>(sampleBuffer).getSubsetChannelBlock(0, 1);
    for(int i = 0; i < displayBuffer.getNumSamples()/2; i++)
    {
        auto minmax = sub.getSubBlock(int(i*ratio), int(ratio)).findMinAndMax();
        out[i*2] = minmax.getStart();
        out[i*2+1] = minmax.getEnd();
    }

    repaint();
}

void NewProjectAudioProcessorEditor::plot(juce::Graphics& g, juce::Rectangle<int> rect)
{
    auto center = rect.getHeight()/2;
    auto w = rect.getWidth();
    auto h = rect.getHeight()/2;
    auto data = displayBuffer.getReadPointer(0);

    for (int i = 1; i < w; i++)
    {
        float x1 = i-1+rect.getTopLeft().getX();
        float y1 = center - h * data[i - 1];
        float x2 = i+rect.getTopLeft().getX();
        float y2 = center - h * data[i];

        g.drawLine (x1,y1,x2,y2);
    }
}

void NewProjectAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &timeKnob)
    {
        sampleBuffer.setSize(audioProcessor.getTotalNumInputChannels(), int(audioProcessor.getSampleRate()*timeKnob.getValue()));
        sampleBuffer.clear();
    }
}


