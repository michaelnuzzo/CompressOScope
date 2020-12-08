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
    //==========================================================================================//

    /* initialize */

    setSize (1000, 800);
    startTimerHz(60);

    // oscilloscope window
    int padding = 100;
    window.setSize(getWidth()-padding, getHeight()*2/3);
    window.setLeft(padding);
    window.setTop(padding/2);

    // initialize display buffer
    windowBuffer.setSize(audioProcessor.getTotalNumInputChannels(), window.getWidth());
    windowBuffer.clear();

    // talk to audio thread
    audioProcessor.setNumPixels(window.getWidth());

    // set global colour preferences
    getLookAndFeel().setColour(juce::ToggleButton::ColourIds::textColourId, juce::Colours::white);
    getLookAndFeel().setColour(juce::ToggleButton::ColourIds::tickColourId, juce::Colours::white);
    getLookAndFeel().setColour(juce::ToggleButton::ColourIds::tickDisabledColourId, juce::Colours::white);

    //==========================================================================================//

    /* set parameters */

    // time slider
    timeKnob.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    timeKnob.setNormalisableRange(juce::NormalisableRange<double>(0.001f,5.f,0.001f));
    timeKnob.onValueChange = [this] {audioProcessor.setNumPixels(window.getWidth()); audioProcessor.setUpdate();};
    timeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getParameters(),"TIME",timeKnob);
    timeLabel.setText("Time Axis", juce::dontSendNotification);
    timeLabel.setJustificationType(juce::Justification::horizontallyCentred);
    timeLabel.attachToComponent(&timeKnob, false);
    addAndMakeVisible(timeKnob);

    // db channel 1 slider
    dbKnobs[0] = std::make_unique<juce::Slider>();
    dbKnobs[0]->setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    dbKnobs[0]->setNormalisableRange(juce::NormalisableRange<double>(0.f,100.f,0.1f));
    db1Label.setText("Channel 1 Gain", juce::dontSendNotification);
    db1Label.setJustificationType(juce::Justification::horizontallyCentred);
    db1Label.attachToComponent(dbKnobs[0].get(), false);
    addAndMakeVisible(*dbKnobs[0]);

    // db channel 2 slider
    dbKnobs[1] = std::make_unique<juce::Slider>();
    dbKnobs[1]->setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    dbKnobs[1]->setNormalisableRange(juce::NormalisableRange<double>(0.f,100.f,0.1f));
    db2Label.setText("Channel 2 Gain", juce::dontSendNotification);
    db2Label.setJustificationType(juce::Justification::horizontallyCentred);
    db2Label.attachToComponent(dbKnobs[1].get(), false);
    addAndMakeVisible(*dbKnobs[1]);

    // compression mode checkbox
    compressionLabel.setText("Compression Mode", juce::dontSendNotification);
    compressionLabel.setJustificationType(juce::Justification::horizontallyCentred);
    compressionLabel.attachToComponent(&compressionButton, true);
    addAndMakeVisible(compressionButton);

    // freeze checkbox
    freezeLabel.setText("Freeze", juce::dontSendNotification);
    freezeLabel.setJustificationType(juce::Justification::horizontallyCentred);
    freezeLabel.attachToComponent(&freezeButton, true);
    addAndMakeVisible(freezeButton);

    //==========================================================================================//

    /* set positions */

    timeKnob.setBounds(getWidth()/4, getHeight()-100, 400, 25);
    dbKnobs[0]->setBounds(getWidth()/4, getHeight()-200, 400, 25);
    dbKnobs[1]->setBounds(getWidth()/4, getHeight()-150, 400, 25);
    compressionButton.setBounds(getWidth()-100, getHeight()-100, 25, 25);
    freezeButton.setBounds(getWidth()-100, getHeight()-150, 25, 25);
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
    float w = rect.getWidth();
    float h = rect.getHeight();
    float x_left = rect.getX();
    float x_right = rect.getRight();
    float y_bottom = rect.getY();
    float y_top = rect.getY() + rect.getHeight();
    auto compMode = compressionButton.getToggleStateValue().getValue();



    if(audioProcessor.displayCollector.getNumUnread() >= windowBuffer.getNumSamples() && !freezeButton.getToggleStateValue().getValue())
    {
        audioProcessor.displayCollector.readHead(windowBuffer);
    }

    if(!compMode)
    {
        // oscilloscope mode
        for(int ch = 0; ch < windowBuffer.getNumChannels(); ch++)
        {
            g.setColour(palette[ch]);
            auto data = windowBuffer.getReadPointer(ch);
            auto gain = juce::Decibels::decibelsToGain(dbKnobs[ch]->getValue());

            for (int i = 0; i < w; i++)
            {
                int x1 = i + x_left;
                int y1 = juce::jlimit(y_bottom, h + y_bottom, y_bottom + float(h/2. - h * data[i] * gain));
                int x2 = i + 1 + x_left;
                int y2 = juce::jlimit(y_bottom, h + y_bottom, y_bottom + float(h/2. - h * data[i + 1] * gain));

                g.drawLine (x1,y1,x2,y2);
            }
        }
    }
    else
    {
        // compression mode
        g.setColour(juce::Colours::lightgreen);
        auto input = windowBuffer.getReadPointer(0);
        auto output = windowBuffer.getReadPointer(1);

        for (int i = 0; i < w; i++)
        {
            int x1 = i + x_left;
            int y1 = juce::jlimit(y_bottom, h + y_bottom, float(h - h/2. * output[i]/input[i]));
            int x2 = i + 1 + x_left;
            int y2 = juce::jlimit(y_bottom, h + y_bottom, float(h - h/2. * output[i + 1]/input[i + 1]));

            if(isfinite(y1) && isfinite(y2))
            {
                g.drawLine (x1,y1,x2,y2);
            }
        }
    }

    // draw x axis
    g.setColour(juce::Colours::white);
    for(float i = 0; i < 5; i++)
    {
        g.drawText(juce::String(i/4. * timeKnob.getValue(), 3), x_right - (i/4. * w) - 17, y_top + 15, 40, 10, juce::Justification::left);
    }

    // draw y-axis
    if(!compMode)
    {
        for(int i = 0; i < 5; i++)
        {
            g.drawText(juce::String(-1 + i * 0.5), x_left - 45, y_top - i * h/4 - 5, 30, 10, juce::Justification::right);
        }
    }
    else
    {

    }
}
