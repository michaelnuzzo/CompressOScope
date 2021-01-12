/*
  ==============================================================================

    ASyncBuffer.cpp
    Created: 31 Oct 2020 12:00:21pm
    Author:  Michael Nuzzo

  ==============================================================================
*/

#include "ASyncBuffer.h"

ASyncBuffer::ASyncBuffer(int numChannels, int size) : abstractFifo(size)
{
    circularBuffer.setSize(numChannels,size);
}

ASyncBuffer::~ASyncBuffer()
{
}

void ASyncBuffer::push(juce::dsp::AudioBlock<float> inBuffer, int numToWrite, int numToMark)
{
    if(numToWrite < 0)
    {
        numToWrite = int(inBuffer.getNumSamples());
    }
    if(numToMark < 0)
    {
        numToMark = numToWrite;
    }

    auto circBuffer = juce::dsp::AudioBlock<float>(circularBuffer);
    if(inBuffer.getNumChannels() > circBuffer.getNumChannels())
    {
        inBuffer = inBuffer.getSubsetChannelBlock(0, circBuffer.getNumChannels());
    }
    else if(inBuffer.getNumChannels() < circBuffer.getNumChannels())
    {
        circBuffer = circBuffer.getSubsetChannelBlock(0, inBuffer.getNumChannels());
    }

    if(canOverwrite && numToWrite > abstractFifo.getFreeSpace())
    {
        int cut = numToWrite - abstractFifo.getFreeSpace();
        trim(cut);
        jassert(abstractFifo.getFreeSpace() >= numToWrite);
    }

    int start1, size1, start2, size2;
    abstractFifo.prepareToWrite(numToWrite, start1, size1, start2, size2);
    jassert(numToWrite == size1+size2);

    if(size1 > 0)
    {
        auto circularChunk = circBuffer.getSubBlock(size_t(start1), size_t(size1));
        auto bufferChunk = inBuffer.getSubBlock(0, size_t(size1));
        circularChunk.copyFrom(bufferChunk);
    }
    if(size2 > 0)
    {
        auto circularChunk = circBuffer.getSubBlock(size_t(start2), size_t(size2));
        auto bufferChunk = inBuffer.getSubBlock(size_t(size1), size_t(size2));
        circularChunk.copyFrom(bufferChunk);
    }

    abstractFifo.finishedWrite(numToMark);
}

void ASyncBuffer::pop(juce::dsp::AudioBlock<float> outBuffer, int numToRead, int numToMark)
{
    if(numToRead < 0)
    {
        numToRead = (int)outBuffer.getNumSamples();
    }
    if(numToMark < 0)
    {
        numToMark = numToRead;
    }

    auto circBuffer = juce::dsp::AudioBlock<float>(circularBuffer);
    if(outBuffer.getNumChannels() > circBuffer.getNumChannels())
    {
        outBuffer = outBuffer.getSubsetChannelBlock(0, circBuffer.getNumChannels());
    }
    else if(outBuffer.getNumChannels() < circBuffer.getNumChannels())
    {
        circBuffer = circBuffer.getSubsetChannelBlock(0, outBuffer.getNumChannels());
    }

    int start1, size1, start2, size2;
    abstractFifo.prepareToRead(numToRead, start1, size1, start2, size2);
    jassert(numToRead == size1+size2);

    if(size1 > 0)
    {
        auto circularChunk = circBuffer.getSubBlock(size_t(start1), size_t(size1));
        auto bufferChunk = outBuffer.getSubBlock(0, size_t(size1));
        bufferChunk.copyFrom(circularChunk);
    }
    if(size2 > 0)
    {
        auto circularChunk = circBuffer.getSubBlock(size_t(start2), size_t(size2));
        auto bufferChunk = outBuffer.getSubBlock(size_t(size1), size_t(size2));
        bufferChunk.copyFrom(circularChunk);
    }

    abstractFifo.finishedRead(numToMark);
}

void ASyncBuffer::readHead(juce::dsp::AudioBlock<float> outBuffer, int numToRead)
{
    if(numToRead < 0)
    {
        numToRead = (int)outBuffer.getNumSamples();
    }

    auto circBuffer = juce::dsp::AudioBlock<float>(circularBuffer);
    if(outBuffer.getNumChannels() > circBuffer.getNumChannels())
    {
        outBuffer = outBuffer.getSubsetChannelBlock(0, circBuffer.getNumChannels());
    }
    else if(outBuffer.getNumChannels() < circBuffer.getNumChannels())
    {
        circBuffer = circBuffer.getSubsetChannelBlock(0, outBuffer.getNumChannels());
    }

    int start1, size1, start2, size2;
    int allReady = abstractFifo.getNumReady();
    int capacity = abstractFifo.getTotalSize();
    abstractFifo.prepareToRead(allReady, start1, size1, start2, size2);
    
    allReady = abstractFifo.getNumReady();

    if(size2 == 0)
    {
        start1 += size1 - numToRead;
        size1 = numToRead;
    }
    else if(numToRead > size2)
    {
        size1 = numToRead - size2;
        start1 = capacity - size1;
    }
    else if(numToRead <= size2)
    {
        start1 = 0;
        size1 = 0;
        start2 = start2 + size2 - numToRead;
        size2 = numToRead;
    }

    if(size1 > 0)
    {
        auto circularChunk = circBuffer.getSubBlock(size_t(start1), size_t(size1));
        auto bufferChunk = outBuffer.getSubBlock(0, size_t(size1));
        bufferChunk.copyFrom(circularChunk);
    }
    if(size2 > 0)
    {
        auto circularChunk = circBuffer.getSubBlock(size_t(start2), size_t(size2));
        auto bufferChunk = outBuffer.getSubBlock(size_t(size1), size_t(size2));
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

void ASyncBuffer::resize(int newSize)
{
    abstractFifo.setTotalSize(newSize);
    circularBuffer.setSize(circularBuffer.getNumChannels(), newSize);
}

void ASyncBuffer::resize(int numChannels, int newSize)
{
    abstractFifo.setTotalSize(newSize);
    circularBuffer.setSize(numChannels, newSize);
}
