/*
 ==============================================================================

 ASyncBuffer.h
 Created: 31 Oct 2020 12:00:21pm
 Author:  Michael Nuzzo

 ==============================================================================
 */

#pragma once

#include <JuceHeader.h>

class ASyncBuffer
{
public:
    ASyncBuffer(int size);
    ~ASyncBuffer();

    void push(const juce::dsp::AudioBlock<float> inBuffer, int numToWrite = -1, int numToMark = -1, int ID = -1);
    void pop(juce::dsp::AudioBlock<float> outBuffer, int numToRead = -1, int numToMark = -1, int ID = -1);
    void readHead(juce::dsp::AudioBlock<float> outBuffer, int numToRead = -1, int ID = -1);
    void trim(int numToTrim);
    void reset();
    void resize(int newSize);
    inline int getNumUnread() {return abstractFifo.getNumReady();}
    inline int getSpaceLeft() {return abstractFifo.getFreeSpace();}
    inline int getTotalSize() {return abstractFifo.getTotalSize();}
    inline void setIsOverwritable(bool overwrite) {canOverwrite = overwrite;}

private:
    juce::AbstractFifo abstractFifo;
    juce::AudioBuffer<float> circularBuffer;
    bool canOverwrite = false;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ASyncBuffer)
};
