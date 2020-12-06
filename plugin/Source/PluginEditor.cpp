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
    setSize (1000, 800);
    startTimerHz(60);

    window.setSize(getWidth()-padding, getHeight()*2/3);
    window.setLeft(padding);
    window.setTop(padding/2);

    audioProcessor.setNumPixels(window.getWidth());

    timeKnob.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    timeKnob.setNormalisableRange(juce::NormalisableRange<double>(0.001f,5.f,0.001f));
    timeKnob.onValueChange = [this] {audioProcessor.setNumPixels(window.getWidth()); audioProcessor.setUpdate();};
    timeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getParameters(),"TIME",timeKnob);
    addAndMakeVisible(timeKnob);

    dbKnobs[0] = std::make_unique<juce::Slider>();
    dbKnobs[0]->setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    dbKnobs[0]->setNormalisableRange(juce::NormalisableRange<double>(0.f,100.f,0.1f));
    addAndMakeVisible(*dbKnobs[0]);

    dbKnobs[1] = std::make_unique<juce::Slider>();
    dbKnobs[1]->setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    dbKnobs[1]->setNormalisableRange(juce::NormalisableRange<double>(0.f,100.f,0.1f));
    addAndMakeVisible(*dbKnobs[1]);

    compressionButton.setButtonText("Compression Mode");
    addAndMakeVisible(compressionButton);

    displayBuffer.setSize(audioProcessor.getTotalNumInputChannels(), window.getWidth());
    displayBuffer.clear();

    timeKnob.setBounds(getWidth()/4, getHeight()-100, 400, 25);
    dbKnobs[0]->setBounds(getWidth()/4, getHeight()-200, 400, 25);
    dbKnobs[1]->setBounds(getWidth()/4, getHeight()-150, 400, 25);
    compressionButton.setBounds(getWidth()-100, getHeight()-100, 25, 25);
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

}

void NewProjectAudioProcessorEditor::resized()
{
}

void NewProjectAudioProcessorEditor::timerCallback()
{
    repaint();
}

void NewProjectAudioProcessorEditor::plot(juce::Graphics& g, juce::Rectangle<int> rect)
{
    auto w = rect.getWidth();
    auto h = rect.getHeight();
    auto t = rect.getTopLeft();

    if(audioProcessor.displayCollector.getNumUnread() >= displayBuffer.getNumSamples())
    {
        audioProcessor.displayCollector.readHead(displayBuffer);
    }

    if(!compressionButton.getToggleStateValue().getValue())
    {
        // oscilloscope
        for(int ch = 0; ch < displayBuffer.getNumChannels(); ch++)
        {
            g.setColour(palette[ch]);
            auto data = displayBuffer.getReadPointer(ch);
            auto gain = juce::Decibels::decibelsToGain(dbKnobs[ch]->getValue());

            for (int i = 0; i < w; i++)
            {
                float x1 = i + t.getX();
                float y1 = juce::jlimit(float(t.getY()), float(h + t.getY()), float(h/2. - h * data[i] * gain));
                float x2 = i + 1 + t.getX();
                float y2 = juce::jlimit(float(t.getY()), float(h + t.getY()), float(h/2. - h * data[i + 1] * gain));

                g.drawLine (x1,y1,x2,y2);
            }
        }
    }
    else
    {
        // compression mode
        g.setColour(juce::Colours::lightgreen);
        auto input = displayBuffer.getReadPointer(0);
        auto output = displayBuffer.getReadPointer(1);

        for (int i = 0; i < w; i++)
        {
            float x1 = i + t.getX();
            float y1 = juce::jlimit(float(t.getY()), float(h+t.getY()), float(h - h/2. * output[i]/input[i]));
            float x2 = i + 1 + t.getX();
            float y2 = juce::jlimit(float(t.getY()), float(h+t.getY()), float(h - h/2. * output[i + 1]/input[i + 1]));

            if(isfinite(y1) && isfinite(y2))
            {
                g.drawLine (x1,y1,x2,y2);
            }
        }
    }
}
