/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CompressOScopeAudioProcessorEditor::CompressOScopeAudioProcessorEditor (CompressOScopeAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    //==========================================================================================//

    /* initialize */

    setSize (1000, 800);
    startTimerHz(60);

    f = juce::Font("Gill Sans", "Regular", 15.f);
    float rescale = 1/8.f;
    logo = juce::ImageCache::getFromMemory (BinaryData::logo_light_png, BinaryData::logo_light_pngSize);
    logo = logo.rescaled(int(logo.getWidth()*rescale), int(logo.getHeight()*rescale));

    // oscilloscope window
    int padding = 50;
    window.setSize(getWidth()-padding, getHeight()*1/2);
    window.setLeft(padding*3);
    window.setTop(padding);

    // initialize display buffer
    displayBuffer.setSize(audioProcessor.displayCollector.getNumChannels(), window.getWidth());
    displayBuffer.clear();

    // talk to audio thread
    audioProcessor.setNumPixels(window.getWidth());

    // set global colour preferences
    getLookAndFeel().setColour(juce::ToggleButton::ColourIds::textColourId, juce::Colours::lightgrey);
    getLookAndFeel().setColour(juce::ToggleButton::ColourIds::tickColourId, juce::Colours::lightgrey);
    getLookAndFeel().setColour(juce::ToggleButton::ColourIds::tickDisabledColourId, juce::Colours::lightgrey);
    getLookAndFeel().setColour(juce::Label::ColourIds::textColourId, juce::Colours::lightgrey);
    getLookAndFeel().setColour(juce::Slider::ColourIds::textBoxTextColourId, juce::Colours::lightgrey);

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
    filterLabel.setText("Filter Length", juce::dontSendNotification);
    filterLabel.setJustificationType(juce::Justification::horizontallyCentred);
    filterLabel.attachToComponent(&filterKnob, true);
    addAndMakeVisible(filterKnob);

    for(size_t ch = 0; ch < size_t(audioProcessor.NUM_CH); ch++)
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
            gainLabels[ch]->setText("Input Channel (L/1) Gain", juce::dontSendNotification);
        if(ch == 1)
            gainLabels[ch]->setText("Output Channel (R/2) Gain", juce::dontSendNotification);
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
        auto smoothingMode = smoothingButton.getToggleStateValue().getValue();

        gainKnobs[0]->setVisible(!compMode);
        gainKnobs[1]->setVisible(!compMode);
        yMinKnob.setVisible(compMode);
        yMaxKnob.setVisible(compMode);
        smoothingButton.setVisible(compMode);
        filterKnob.setVisible(compMode && smoothingMode);
        filterKnob.setEnabled(compMode && !freezeMode && smoothingMode);
    };
    compressionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.getParameters(),"COMPMODE",compressionButton);
    compressionLabel.setText("Compression Mode", juce::dontSendNotification);
    compressionLabel.setJustificationType(juce::Justification::horizontallyCentred);
    compressionLabel.attachToComponent(&compressionButton, true);
    addAndMakeVisible(compressionButton);

    // freeze checkbox
    freezeButton.onStateChange = [this] {
        auto compMode = compressionButton.getToggleStateValue().getValue();
        auto freezeMode = freezeButton.getToggleStateValue().getValue();
        auto smoothingMode = smoothingButton.getToggleStateValue().getValue();
        timeKnob.setEnabled(!freezeMode);
        filterKnob.setEnabled(compMode && !freezeMode && smoothingMode);
    };
    freezeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.getParameters(),"FREEZE",freezeButton);
    freezeLabel.setText("Freeze", juce::dontSendNotification);
    freezeLabel.setJustificationType(juce::Justification::horizontallyCentred);
    freezeLabel.attachToComponent(&freezeButton, true);
    addAndMakeVisible(freezeButton);

    // smoothing checkbox
    smoothingButton.onStateChange = [this] {
        auto compMode = compressionButton.getToggleStateValue().getValue();
        auto freezeMode = freezeButton.getToggleStateValue().getValue();
        auto smoothingMode = smoothingButton.getToggleStateValue().getValue();
        filterKnob.setVisible(compMode && smoothingMode);
        filterKnob.setEnabled(compMode && !freezeMode && smoothingMode);
        audioProcessor.setUpdate();
    };
    smoothingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.getParameters(),"SMOOTHING",smoothingButton);
    smoothingLabel.setText("Smoothing", juce::dontSendNotification);
    smoothingLabel.setJustificationType(juce::Justification::horizontallyCentred);
    smoothingLabel.attachToComponent(&smoothingButton, true);
    addAndMakeVisible(smoothingButton);

    // set visibility and enabled
    auto compMode = compressionButton.getToggleStateValue().getValue();
    auto freezeMode = freezeButton.getToggleStateValue().getValue();
    auto smoothingMode = smoothingButton.getToggleStateValue().getValue();
    timeKnob.setEnabled(!freezeMode);
    gainKnobs[0]->setVisible(!compMode);
    gainKnobs[1]->setVisible(!compMode);
    yMinKnob.setVisible(compMode);
    yMaxKnob.setVisible(compMode);
    smoothingButton.setVisible(compMode);
    filterKnob.setVisible(compMode && smoothingMode);
    filterKnob.setEnabled(compMode && !freezeMode && smoothingMode);

    //==========================================================================================//

    /* set positions */
    int spacing = 60;
    int gap = spacing*3/4;

    timeKnob.setBounds(          getWidth()/3  , getHeight()-spacing*4-gap, 400, 50);
    filterKnob.setBounds(        getWidth()/3  , getHeight()-spacing*1-gap, 400, 50);
    gainKnobs[0]->setBounds(     getWidth()/3  , getHeight()-spacing*3-gap, 400, 50);
    gainKnobs[1]->setBounds(     getWidth()/3  , getHeight()-spacing*2-gap, 400, 50);
    yMaxKnob.setBounds(          getWidth()/3  , getHeight()-spacing*3-gap, 400, 50);
    yMinKnob.setBounds(          getWidth()/3  , getHeight()-spacing*2-gap, 400, 50);
    compressionButton.setBounds( getWidth()-100, getHeight()-spacing*2-gap, 25 , 25);
    freezeButton.setBounds(      getWidth()-100, getHeight()-spacing*3-gap, 25 , 25);
    smoothingButton.setBounds(   getWidth()-100, getHeight()-spacing*1-gap, 25 , 25);

    audioProcessor.setGuiReady(true);
}

CompressOScopeAudioProcessorEditor::~CompressOScopeAudioProcessorEditor()
{
}

//==============================================================================
void CompressOScopeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.setFont(f);
    g.fillAll (juce::Colours::black);
    g.setColour(juce::Colours::lightgrey);

    plot(g);

    int xPos = 20;
    int yPos = -20 + getHeight() - int(logo.getHeight()) - int(f.getHeight());
    g.drawImageAt(logo, xPos, yPos);
    write(ProjectInfo::versionString, xPos + logo.getWidth(), yPos + logo.getHeight() + 5, juce::Justification::right, g);
}

void CompressOScopeAudioProcessorEditor::resized()
{
}

void CompressOScopeAudioProcessorEditor::timerCallback()
{
    repaint();
}

void CompressOScopeAudioProcessorEditor::write(juce::String txt, int xPos, int yPos, juce::Justification j, juce::Graphics& g)
{
    if(j == juce::Justification::right)
    {
        xPos -= f.getStringWidth(txt);
    }
    g.drawText(txt, xPos, yPos, f.getStringWidth(txt), int(f.getHeight()), j);
}

void CompressOScopeAudioProcessorEditor::plot(juce::Graphics& g)
{
    //==========================================================================================//

    /* read data */
    if(audioProcessor.displayCollector.getNumUnread() >= displayBuffer.getNumSamples() &&
       !freezeButton.getToggleStateValue().getValue() &&
       audioProcessor.isDoneProcessing())
    {
        audioProcessor.displayCollector.readHead(displayBuffer);
    }

    //==========================================================================================//

    /* initialize variables*/

    int w  = window.getWidth() - 1;  // window width
    int h  = window.getHeight() - 1; // window height
    int l  = window.getX();          // window left
    int r  = l + w;                  // window right
    int b  = window.getY();          // window bottom
    int t  = b + h;                  // window top
    int fh = int(f.getHeight());     // font height
    int tickSize = 10;
    auto compMode = compressionButton.getToggleStateValue().getValue();
    auto yMin = float(yMinKnob.getValue());
    auto yMax = float(yMaxKnob.getValue());
    auto jLeft  = juce::Justification::left;
    auto jCtr   = juce::Justification::horizontallyCentred;
    auto jRight = juce::Justification::right;
    juce::String txt;

    //==========================================================================================//

    /* draw axes */

    // draw zoom level
    txt = "Zoom = ";
    txt += juce::String(100.f * 1.f / audioProcessor.getNumSamplesPerPixel(), 1);
    txt += "%";
    write(txt, r - 130, t + 50, jLeft, g);

    // draw x axis
    float numXTicks = 5;
    for(float i = 0; i < numXTicks; i++)
    {
        float scale = i / (numXTicks - 1);
        int xPos = r - int(scale * w);
        g.drawRect(xPos, t, 1, tickSize); // tick
        write(juce::String(scale * timeKnob.getValue(), 4), xPos - 17, t + 15, jCtr, g);
    }
    write("Time (s)", l + w/2 - 25, t + 40, jCtr, g);

    // draw y-axis
    if(!compMode) // oscilloscope mode
    {
        float numYTicks = 5;
        for(float i = 0; i < numYTicks; i++)
        {
            int yPos = b + int(i * h / (numYTicks - 1));
            g.drawRect(l - tickSize, yPos, tickSize, 1); // tick
            write(juce::String(1 - i * 0.5), l - 15, yPos - fh / 2, jRight, g);
        }
        write("Magnitude", l - 100, b + int(h/2.f - fh/2.f), jCtr, g);
    }
    else // compression mode
    {
        if(yMin == yMax)
        {
            yMax += 0.0001f;
        }

        // draw y-axis
        float numYTicks = 10;
        for(float i = 0; i < numYTicks; i++)
        {
            int yPos = b + int(i * h / (numYTicks - 1));
            g.drawRect(l - tickSize, yPos, tickSize, 1); // tick
            float curTick = (numYTicks - 1 - i) * yMax / (numYTicks - 1) + i * yMin / (numYTicks - 1);
            txt = juce::String(curTick, 3);
            write(txt, l - 15, yPos - fh / 2, jRight, g);
        }
        write("Amplitude", l - 125, b + int(h / 2.f) - int(fh / 2.f)      , jCtr, g);
        write("(dBFS)"   , l - 125, b + int(h / 2.f) + int(fh * 2.f / 3.f), jCtr, g);
    }

    //==========================================================================================//

    /* draw data */

    if(!compMode) // oscilloscope mode
    {
        // lambda function to convert audio data into plot coordinate
        auto valToCoord = [&](float v)
        {
            return juce::jlimit(b, t, b + int(juce::jmap(v, 1.f, -1.f, 0.f, float(h))));
        };

        for(size_t ch = 0; ch < size_t(audioProcessor.NUM_CH); ch++)
        {
            g.setColour(palette[ch]);
            auto data = displayBuffer.getReadPointer(int(ch));
            auto data_min = displayBuffer.getReadPointer(int(ch) + 3);
            float gain = juce::Decibels::decibelsToGain(float(gainKnobs[ch]->getValue()));

            for (int i = 1; i < w; i++)
            {
                float d1, d2;

                prepareFilledLine(data[i], data[i + 1], data_min[i], data_min[i + 1], d1, d2);

                int x1 = i + l;
                auto y1 = valToCoord(d1 * gain);
                auto y2 = valToCoord(d2 * gain);

                if(y1 < y2)
                {
                    std::swap(y1, y2);
                }

                g.fillRect(x1, y2, 1, 1 + (y1 - y2));
            }
        }
    }
    else // compression mode
    {
        // lambda function to convert compression data into plot coordinate
        auto valToCoord = [&](float v)
        {
            return juce::jlimit(b, t, b + int(juce::jmap(juce::Decibels::gainToDecibels(v), yMax, yMin, 0.f, float(h))));
        };
        
        g.setColour(palette[2]);
        auto comp = displayBuffer.getReadPointer(2);
        auto comp_min = displayBuffer.getReadPointer(2 + 3);

        for (int i = 1; i < w; i++)
        {
            float d1, d2;

            prepareFilledLine(comp[i], comp[i + 1], comp_min[i], comp_min[i + 1], d1, d2);

            int x1 = i + l;
            int y1 = valToCoord(d1);
            int y2 = valToCoord(d2);

            if(comp[i] > 0 && comp[i + 1] > 0)
            {
                if(y1 < y2)
                {
                    std::swap(y1, y2);
                }
                g.fillRect(x1, y2, 1, 1 + (y1 - y2));
            }
        }
    }

    g.setColour(juce::Colours::lightgrey);
    g.drawRect(window);
}

void CompressOScopeAudioProcessorEditor::prepareFilledLine(float v, float vNext, float vMin, float vMinNext, float &out1, float &out2)
{
    if(!isnan(vMin))
    {
        out1 = v;
        out2 = vMin;

        if(!isnan(vMinNext))
        {
            if(v < vMinNext)
            {
                out1 = vMinNext;
            }
            if(vMin > vNext)
            {
                out2 = vNext;
            }
        }
        else
        {
            if(v < vNext)
            {
                out1 = vNext;
            }
            if(vMin > vNext)
            {
                out2 = vNext;
            }
        }
    }
    else
    {
        out1 = v;
        out2 = vNext;

        if(!isnan(vMinNext))
        {
            if(v < vMinNext)
            {
                out2 = vMinNext;
                if(v > vNext)
                {
                    out1 = vNext;
                }
            }
        }
    }
}
