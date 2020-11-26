/*
  ==============================================================================

    ASyncBuffer.cpp
    Created: 31 Oct 2020 12:00:21pm
    Author:  Michael Nuzzo

  ==============================================================================
*/

#include "ASyncBuffer.h"

ASyncBuffer::ASyncBuffer()
{
    circularBuffer.setSize(2,MAX_LENGTH);
}

ASyncBuffer::~ASyncBuffer()
{
}

void ASyncBuffer::push(const juce::dsp::AudioBlock<float> inBuffer, int numToWrite, int numToMark, int ID)
{
    if(numToWrite < 0)
    {
        numToWrite = (int)inBuffer.getNumSamples();
    }
    if(numToMark < 0)
    {
        numToMark = numToWrite;
    }

    int start1, size1, start2, size2;
    abstractFifo.prepareToWrite(numToWrite, start1, size1, start2, size2);
    jassert(numToWrite == size1+size2);

    if(size1 > 0)
    {
        auto circularChunk = juce::dsp::AudioBlock<float>(circularBuffer).getSubBlock(start1, size1);
        auto bufferChunk = inBuffer.getSubBlock(0, size1);
        circularChunk.copyFrom(bufferChunk);
    }
    if(size2 > 0)
    {
        auto circularChunk = juce::dsp::AudioBlock<float>(circularBuffer).getSubBlock(start2, size2);
        auto bufferChunk = inBuffer.getSubBlock(size1, size2);
        circularChunk.copyFrom(bufferChunk);
    }

    abstractFifo.finishedWrite(numToMark);
}

void ASyncBuffer::pop(const juce::dsp::AudioBlock<float> outBuffer, int numToRead, int numToMark, int ID)
{
    if(numToRead < 0)
    {
        numToRead = (int)outBuffer.getNumSamples();
    }
    if(numToMark < 0)
    {
        numToMark = numToRead;
    }

    int start1, size1, start2, size2;
    abstractFifo.prepareToRead(numToRead, start1, size1, start2, size2);
    jassert(numToRead == size1+size2);

    if(size1 > 0)
    {
        auto circularChunk = juce::dsp::AudioBlock<float>(circularBuffer).getSubBlock(start1, size1);
        auto bufferChunk = outBuffer.getSubBlock(0, size1);
        bufferChunk.copyFrom(circularChunk);
    }
    if(size2 > 0)
    {
        auto circularChunk = juce::dsp::AudioBlock<float>(circularBuffer).getSubBlock(start2, size2);
        auto bufferChunk = outBuffer.getSubBlock(size1, size2);
        bufferChunk.copyFrom(circularChunk);
    }

    abstractFifo.finishedRead(numToMark);
}

void ASyncBuffer::readHead(const juce::dsp::AudioBlock<float> outBuffer, int numToRead, int ID)
{
    if(numToRead < 0)
    {
        numToRead = (int)outBuffer.getNumSamples();
    }

    int start1, size1, start2, size2;
    int fullSize = abstractFifo.getNumReady();
    abstractFifo.prepareToRead(fullSize, start1, size1, start2, size2);

    // while the circular buffer is not maxed out
    // read from size1 + everything in size2
    if(numToRead > size2)
    {
        size1 = numToRead - size2;
        start1 = fullSize - size1;
    }
    // while the circular buffer is at max capacity
    // only read from size2
    else
    {
        start1 = 0;
        size1 = 0;
        start2 = size2 - numToRead;
        size2 = numToRead;
    }
    jassert(numToRead == size1+size2);

    if(size1 > 0)
    {
        auto circularChunk = juce::dsp::AudioBlock<float>(circularBuffer).getSubBlock(start1, size1);
        auto bufferChunk = outBuffer.getSubBlock(0, size1);
        bufferChunk.copyFrom(circularChunk);
    }
    if(size2 > 0)
    {
        auto circularChunk = juce::dsp::AudioBlock<float>(circularBuffer).getSubBlock(start2, size2);
        auto bufferChunk = outBuffer.getSubBlock(size1, size2);
        bufferChunk.copyFrom(circularChunk);
    }
}

void ASyncBuffer::trim(int numToTrim)
{
    abstractFifo.finishedRead(numToTrim);
}

void ASyncBuffer::reset()
{
    circularBuffer.clear();
    abstractFifo.reset();
}




