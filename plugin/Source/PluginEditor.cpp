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

    f = juce::Font("Gill Sans", "Regular", 15.f);

    // oscilloscope window
    int padding = 50;
    window.setSize(getWidth()-padding, getHeight()*1/2);
    window.setLeft(padding*3);
    window.setTop(padding);

    // initialize display buffer
    windowBuffer.setSize(audioProcessor.getTotalNumInputChannels() + 1, window.getWidth());
    windowBuffer.clear();
    DEBUG_BUFFER.setSize(1, window.getWidth());
    DEBUG_BUFFER.clear();


    // talk to audio thread
    audioProcessor.setNumPixels(window.getWidth());

    // set global colour preferences
    getLookAndFeel().setColour(juce::ToggleButton::ColourIds::textColourId, juce::Colours::lightgrey);
    getLookAndFeel().setColour(juce::ToggleButton::ColourIds::tickColourId, juce::Colours::lightgrey);
    getLookAndFeel().setColour(juce::ToggleButton::ColourIds::tickDisabledColourId, juce::Colours::lightgrey);
    getLookAndFeel().setColour(juce::Label::ColourIds::textColourId, juce::Colours::lightgrey);

    //==========================================================================================//

    /* set parameters */

    // time slider
    timeKnob.setColour(juce::Slider::ColourIds::thumbColourId, juce::Colours::lightgrey);
    timeKnob.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    timeKnob.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 100, 20);
    timeKnob.setTextValueSuffix(" s");
    timeKnob.onValueChange = [this] {audioProcessor.setNumPixels(window.getWidth()); audioProcessor.setUpdate();};
    timeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getParameters(),"TIME",timeKnob);
    timeLabel.setText("Time Axis", juce::dontSendNotification);
    timeLabel.setJustificationType(juce::Justification::horizontallyCentred);
    timeLabel.attachToComponent(&timeKnob, true);
    addAndMakeVisible(timeKnob);

    // filter slider
    filterKnob.setColour(juce::Slider::ColourIds::thumbColourId, palette[3]);
    filterKnob.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    filterKnob.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 100, 20);
    filterKnob.setTextValueSuffix(" ms");
    filterKnob.onValueChange = [this] {audioProcessor.setUpdate();};
    filterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getParameters(),"FILTER",filterKnob);
    filterLabel.setText("Filter Length (ms)", juce::dontSendNotification);
    filterLabel.setJustificationType(juce::Justification::horizontallyCentred);
    filterLabel.attachToComponent(&filterKnob, true);
    addAndMakeVisible(filterKnob);

    for(int ch = 0; ch < audioProcessor.getTotalNumInputChannels(); ch++)
    {
        // channel gain sliders
        gainKnobs[ch] = std::make_unique<juce::Slider>();
        gainKnobs[ch]->setColour(juce::Slider::ColourIds::thumbColourId, palette[ch]);
        gainKnobs[ch]->setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
        gainKnobs[ch]->setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 100, 20);
        gainKnobs[ch]->setTextValueSuffix(" dBFS");
        juce::String name = juce::String("GAIN") + juce::String(ch+1);
        gainAttachments[ch] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getParameters(), name, *gainKnobs[ch]);
        gainLabels[ch] = std::make_unique<juce::Label>();
        juce::String label;
        if(ch == 0)
            gainLabels[ch]->setText("Channel 1 (reference) Gain", juce::dontSendNotification);
        if(ch == 1)
            gainLabels[ch]->setText("Channel 2 (divisor) Gain", juce::dontSendNotification);
        gainLabels[ch]->setJustificationType(juce::Justification::horizontallyCentred);
        gainLabels[ch]->attachToComponent(gainKnobs[ch].get(), true);
        addAndMakeVisible(gainKnobs[ch].get());
    }

    // ymax slider
    yMaxKnob.setColour(juce::Slider::ColourIds::thumbColourId, palette[3]);
    yMaxKnob.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    yMaxKnob.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 100, 20);
    yMaxKnob.setTextValueSuffix(" dBFS");
    yMaxAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getParameters(),"YMAX",yMaxKnob);
    yMaxLabel.setText("Y max", juce::dontSendNotification);
    yMaxLabel.setJustificationType(juce::Justification::horizontallyCentred);
    yMaxLabel.attachToComponent(&yMaxKnob, true);
    addAndMakeVisible(yMaxKnob);

    // ymin slider
    yMinKnob.setColour(juce::Slider::ColourIds::thumbColourId, palette[3]);
    yMinKnob.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    yMinKnob.setTextValueSuffix(" dBFS");
    yMinKnob.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 100, 20);
    yMinAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getParameters(),"YMIN",yMinKnob);
    yMinLabel.setText("Y min", juce::dontSendNotification);
    yMinLabel.setJustificationType(juce::Justification::horizontallyCentred);
    yMinLabel.attachToComponent(&yMinKnob, true);
    addAndMakeVisible(yMinKnob);

    // compression mode checkbox
    compressionButton.onStateChange = [this]
    {
        // show the appropriate slider
        auto compMode = compressionButton.getToggleStateValue().getValue();
        auto freezeMode = freezeButton.getToggleStateValue().getValue();
        gainKnobs[0]->setVisible(!compMode);
        gainKnobs[1]->setVisible(!compMode);
        yMinKnob.setVisible(compMode);
        yMaxKnob.setVisible(compMode);
        filterKnob.setVisible(compMode && !freezeMode);
    };
    auto compMode = compressionButton.getToggleStateValue().getValue();
    auto freezeMode = freezeButton.getToggleStateValue().getValue();
    gainKnobs[0]->setVisible(!compMode);
    gainKnobs[1]->setVisible(!compMode);
    yMinKnob.setVisible(compMode);
    yMaxKnob.setVisible(compMode);
    filterKnob.setVisible(compMode && !freezeMode);

    compressionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.getParameters(),"COMPMODE",compressionButton);
    compressionLabel.setText("Compression Mode", juce::dontSendNotification);
    compressionLabel.setJustificationType(juce::Justification::horizontallyCentred);
    compressionLabel.attachToComponent(&compressionButton, true);
    addAndMakeVisible(compressionButton);

    // freeze checkbox
    freezeButton.onStateChange = [this] {
        auto compMode = compressionButton.getToggleStateValue().getValue();
        auto freezeMode = freezeButton.getToggleStateValue().getValue();
        timeKnob.setVisible(!freezeMode);
        filterKnob.setVisible(compMode && !freezeMode);
    };
    freezeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.getParameters(),"FREEZE",freezeButton);
    freezeLabel.setText("Freeze", juce::dontSendNotification);
    freezeLabel.setJustificationType(juce::Justification::horizontallyCentred);
    freezeLabel.attachToComponent(&freezeButton, true);
    addAndMakeVisible(freezeButton);

    //==========================================================================================//

    /* set positions */
    int spacing = 60;
    int gap = spacing*3/4;

    timeKnob.setBounds(          getWidth()/3,   getHeight()-spacing*4-gap, 400, 50 );
    filterKnob.setBounds(        getWidth()/3,   getHeight()-spacing*1-gap, 400, 50 );
    gainKnobs[0]->setBounds(     getWidth()/3,   getHeight()-spacing*3-gap, 400, 50 );
    gainKnobs[1]->setBounds(     getWidth()/3,   getHeight()-spacing*2-gap, 400, 50 );
    yMaxKnob.setBounds(          getWidth()/3,   getHeight()-spacing*3-gap, 400, 50 );
    yMinKnob.setBounds(          getWidth()/3,   getHeight()-spacing*2-gap, 400, 50 );
    compressionButton.setBounds( getWidth()-100, getHeight()-spacing*2-gap, 25,  25 );
    freezeButton.setBounds(      getWidth()-100, getHeight()-spacing*3-gap, 25,  25 );

    audioProcessor.setReady(true);
}

NewProjectAudioProcessorEditor::~NewProjectAudioProcessorEditor()
{
}

//==============================================================================
void NewProjectAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.setFont(f);
    g.fillAll (juce::Colours::black);
    g.setColour(juce::Colours::lightgrey);

    plot(g);
}

void NewProjectAudioProcessorEditor::resized()
{
}

void NewProjectAudioProcessorEditor::timerCallback()
{
    repaint();
}

void NewProjectAudioProcessorEditor::plot(juce::Graphics& g)
{

    float w        = window.getWidth();
    float h        = window.getHeight();
    float x_left   = window.getX();
    float x_right  = window.getRight();
    float y_bottom = window.getY();
    float y_top    = y_bottom + h;
    int   fh       = f.getHeight();
    auto compMode = compressionButton.getToggleStateValue().getValue();
    juce::String txt;

    txt = "Zoom = ";
    txt += juce::String(100*1/audioProcessor.getRatio(),1);
    txt += "%";
    g.drawText(juce::String(txt), x_right-100, y_top + 50, f.getStringWidth(txt), fh, juce::Justification::left);

    // draw x axis
    float numXTicks = 5;
    for(float i = 0; i < numXTicks; i++)
    {
        float scale = i/(numXTicks-1);
        int xPos = x_right - scale * (w-1);
        g.drawLine(xPos - 0.5, y_top, xPos - 0.5, y_top+10);
        txt = juce::String(scale * timeKnob.getValue(), 4);
        g.drawText(txt, xPos - 17, y_top + 15, f.getStringWidth("0.0000"), fh, juce::Justification::horizontallyCentred);
    }
    txt = "Time (s)";
    g.drawText(txt, x_left + w/2 - 25, y_top + 40, f.getStringWidth(txt), fh, juce::Justification::horizontallyCentred);

    // read data
    if(audioProcessor.displayCollector.getNumUnread() >= windowBuffer.getNumSamples() && !freezeButton.getToggleStateValue().getValue())
    {
        audioProcessor.displayCollector.readHead(windowBuffer);
    }

    if(!compMode)
    {
        /* oscilloscope mode */

        // draw y-axis
        float numYTicks = 5;
        for(float i = 0; i < numYTicks; i++)
        {
            int yPos = y_bottom + i*(h-1)/(numYTicks-1);
            g.drawLine(x_left-10, yPos + 0.5, x_left, yPos + 0.5);
            txt = juce::String(1 - i * 0.5);
            g.drawText(txt, x_left - 35, yPos - fh/2, f.getStringWidth("-0.5"), fh, juce::Justification::right);
        }
        txt = "Magnitude";
        g.drawText(txt, x_left - 100, y_bottom + h/2 - fh/2, f.getStringWidth(txt), fh, juce::Justification::right);


        // draw data
        for(int ch = 0; ch < audioProcessor.getTotalNumInputChannels(); ch++)
        {
            g.setColour(palette[ch]);
            auto data = windowBuffer.getReadPointer(ch);
            auto gain = juce::Decibels::decibelsToGain(gainKnobs[ch]->getValue());

            for (int i = 1; i < w-1; i++)
            {
                float x1 = i + x_left;
                float y1 = juce::jlimit(y_bottom, y_top, y_bottom + float(h/2.f - h * data[i]     * gain));
                float x2 = i + 1 + x_left;
                float y2 = juce::jlimit(y_bottom, y_top, y_bottom + float(h/2.f - h * data[i + 1] * gain));

                g.drawLine (x1,y1,x2,y2);
            }
        }
    }
    else
    {
        /* compression mode */
        float yMin = yMinKnob.getValue();
        float yMax = yMaxKnob.getValue();
        if(yMin == yMax)
        {
            yMax += 0.0001;
        }

        // draw y-axis
        float numYTicks = 10;
        for(float i = 0; i < numYTicks; i++)
        {
            int yPos = y_bottom + i*(h-1)/(numYTicks-1);
            g.drawLine(x_left-10, yPos + 0.5, x_left, yPos + 0.5);
            float curTick = (numYTicks-1-i) * yMax / (numYTicks - 1) + i * yMin / (numYTicks - 1);
            txt = juce::String(curTick, 3);
            g.drawText(txt, x_left - 55, yPos - fh/2, f.getStringWidth("-00.000"), fh, juce::Justification::right);
        }
        txt = "Amplitude";
        g.drawText(txt, x_left - 125, y_bottom + h/2 - fh/2, f.getStringWidth(txt), fh, juce::Justification::horizontallyCentred);
        g.drawText("(dBFS)", x_left - 125, y_bottom + h/2 + fh*2/3, f.getStringWidth(txt), fh, juce::Justification::horizontallyCentred);



        // draw data
        g.setColour(palette[2]);
        auto comp = windowBuffer.getReadPointer(2);

        for (int i = 1; i < w-1; i++)
        {
            float val1 = juce::jmap(juce::Decibels::gainToDecibels(comp[i]    ), yMax, yMin, 0.f, h);
            float val2 = juce::jmap(juce::Decibels::gainToDecibels(comp[i + 1]), yMax, yMin, 0.f, h);

            float x1 = i + x_left;
            float y1 = juce::jlimit(y_bottom, y_top, y_bottom + val1);
            float x2 = i + 1 + x_left;
            float y2 = juce::jlimit(y_bottom, y_top, y_bottom + val2);

            if(comp[i] >= 0 && comp[i + 1] >= 0)
            {
                g.drawLine (x1,y1,x2,y2);
            }
        }
    }
    g.setColour(juce::Colours::lightgrey);
    g.drawRect(window);
}
