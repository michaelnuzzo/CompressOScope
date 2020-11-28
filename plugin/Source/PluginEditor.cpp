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
    window.setTop(padding/2);
//    window.setX(padding);
//    window.setY(padding/2);

    timeKnob.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    timeKnob.setNormalisableRange(juce::NormalisableRange<double>(0.05f,5.f,0.01f));
    timeKnob.addListener(this);
    addAndMakeVisible(timeKnob);

    sampleBuffer.setSize(audioProcessor.getTotalNumInputChannels(), int(audioProcessor.getSampleRate()*timeKnob.getValue()));
    sampleBuffer.clear();

    displayBuffer.setSize(audioProcessor.getTotalNumInputChannels(), window.getWidth());
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

    // TODO: i think this is not quite right, fix it at some point...
    float ratio = sampleBuffer.getNumSamples()/window.getWidth();
    for(int ch = 0; ch < displayBuffer.getNumChannels(); ch++)
    {
        float * displayData = displayBuffer.getWritePointer(ch);
        auto sub = juce::dsp::AudioBlock<float>(sampleBuffer).getSubsetChannelBlock(ch, 1);
        for(int i = 0; i < displayBuffer.getNumSamples()/2; i++)
        {
            auto minmax = sub.getSubBlock(int(i*ratio), int(ratio)).findMinAndMax();
            displayData[i*2] = minmax.getStart();
            displayData[i*2+1] = minmax.getEnd();
        }
    }

    repaint();
}

void NewProjectAudioProcessorEditor::plot(juce::Graphics& g, juce::Rectangle<int> rect)
{
    auto w = rect.getWidth();
    auto h = rect.getHeight();
    auto t = rect.getTopLeft();


    // oscilloscope
    for(int ch = 0; ch < displayBuffer.getNumChannels(); ch++)
    {
        g.setColour(palette[ch]);
        auto data = displayBuffer.getReadPointer(ch);

        for (int i = 1; i < w; i++)
        {
            float x1 = (w - i) - 1 + t.getX();
            float y1 = juce::jlimit(float(t.getY()), float(h+t.getY()), float(h/2. - h * data[i - 1]));
            float x2 = (w - i) + t.getX();
            float y2 = juce::jlimit(float(t.getY()), float(h+t.getY()), float(h/2. - h * data[i]));

            g.drawLine (x1,y1,x2,y2);
        }
    }

    // compression mode
    g.setColour(juce::Colours::lightgreen);
    auto input = displayBuffer.getReadPointer(0);
    auto output = displayBuffer.getReadPointer(1);

    for (int i = 1; i < w; i++)
    {
        float x1 = (w - i) - 1 + t.getX();
        float y1 = juce::jlimit(float(t.getY()), float(h+t.getY()), float(h - h/2. * output[i - 1]/input[i - 1]));
        float x2 = (w - i) + t.getX();
        float y2 = juce::jlimit(float(t.getY()), float(h+t.getY()), float(h - h/2. * output[i]/input[i]));
        
        if(isfinite(y1) && isfinite(y2))
        {
            g.drawLine (x1,y1,x2,y2);
        }
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


