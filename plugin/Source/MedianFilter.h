/*
 ==============================================================================

 MedianFilter.h
 Created: 31 Oct 2020 12:00:21pm
 Author:  Michael Nuzzo

 ==============================================================================
 */

#pragma once

#include <JuceHeader.h>
#include "ASyncBuffer.h"

class MedianFilter
{
public:
    MedianFilter(int order);
    ~MedianFilter();

    void setOrder(int newOrder);

    void push(float val);
    void pop();
    float getMedian();
    inline bool hasEvenLength() {return numValidNodes % 2 == 0;};
    inline bool isReady() {return numValidNodes > 0;};

private:
    struct llNode
    {
        llNode() : data(NAN), prev(nullptr), next(nullptr) {}
        float data;
        llNode* prev;
        llNode* next;
    };
    void updateMedian(llNode* changedllNode, bool justPushed);
    void swapNodes(llNode* A, llNode* B);

    juce::Array<llNode> linkedList;
    juce::AbstractFifo abstractFifo;
    llNode* lowest;
    llNode* highest;
    llNode* median;
    llNode* lowMedian;
    llNode* highMedian;
    int numValidNodes;
    int counter = 0;

    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MedianFilter)
};
