/*
  ==============================================================================

    MedianFilter.cpp
    Created: 31 Oct 2020 12:00:21pm
    Author:  Michael Nuzzo

  ==============================================================================
*/

#include "MedianFilter.h"

MedianFilter::MedianFilter(int order) : abstractFifo(order+1), lowest(nullptr), highest(nullptr), median(nullptr), lowMedian(nullptr), highMedian(nullptr), numValidNodes(0)
{
    linkedList.resize(order+1);
    linkedList.fill(llNode());
}

MedianFilter::~MedianFilter()
{
}

void MedianFilter::setOrder(int newOrder)
{
    if(newOrder - abstractFifo.getTotalSize() != 0)
    {
        abstractFifo.setTotalSize(newOrder+1);
        linkedList.resize(newOrder+1);
        linkedList.fill(llNode());
        lowest = nullptr;
        highest = nullptr;
        median = nullptr;
        lowMedian = nullptr;
        highMedian = nullptr;
        numValidNodes = 0;
    }
}

void MedianFilter::push(float val)
{
    if(abstractFifo.getFreeSpace() == 0)
    {
        pop();
    }
    int start1, size1, start2, size2;
    abstractFifo.prepareToWrite(1, start1, size1, start2, size2);
    jassert(size1 == 1);
    jassert(size2 == 0);
    abstractFifo.finishedWrite(1);

    if(!isnan(val))
    {
        numValidNodes++;
        llNode* newNode = &linkedList.getRawDataPointer()[start1];
        jassert(isnan(newNode->data));
        jassert(newNode->next == nullptr);
        jassert(newNode->prev == nullptr);


        newNode->data = val;

        if(lowest == nullptr) // first time through
        {
            lowest = newNode;
            highest = newNode;
            median = newNode;
        }
        else
        {
            llNode* cur = lowest;
            while(cur->next != nullptr && newNode->data > cur->data)
            {
                cur = cur->next;
            }
            jassert(newNode->next == nullptr);

            if(newNode->data <= cur->data) // base case
            {
                if(cur != lowest)
                {
                    jassert(cur->prev != nullptr);
                    cur->prev->next = newNode;
                    newNode->prev = cur->prev;
                }
                else
                {
                    jassert(cur->prev == nullptr);
                    jassert(newNode->prev == nullptr);
                    lowest = newNode;
                }
                newNode->next = cur;
                cur->prev = newNode;
                jassert(lowest->prev == nullptr);
            }
            else // newNode is the highest node
            {
                jassert(cur->next == nullptr);
                jassert(newNode->next == nullptr);
                newNode->prev = cur;
                cur->next = newNode;
                highest = newNode;
                jassert(highest->next == nullptr);

            }
            jassert(lowest->prev == nullptr);
            jassert(highest->next == nullptr);
            updateMedian(newNode, true);
        }
    }

    auto cur = lowest;
    if(hasEvenLength())
    {
        for(int i = 0; i < numValidNodes; i++)
        {
            if(i == int(numValidNodes / 2) - 1)
            {
                jassert(cur == lowMedian);
            }
            else if(i == int(numValidNodes / 2) - 1)
            {
                jassert(cur == highMedian);
            }
            cur = cur->next;
        }
    }
    else
    {
        for(int i = 0; i < numValidNodes; i++)
        {
            if(i == int(numValidNodes / 2))
            {
                jassert(cur == median);
            }
            cur = cur->next;
        }
    }
}

void MedianFilter::pop()
{
    auto ll = linkedList.getRawDataPointer();
    int start1, size1, start2, size2;
    abstractFifo.prepareToRead(1, start1, size1, start2, size2);
    jassert(size1 == 1);
    jassert(size2 == 0);
    abstractFifo.finishedRead(1);

    llNode* oldNode = &ll[start1];

    if(!isnan(oldNode->data))
    {
        numValidNodes--;

        updateMedian(oldNode, false);

        if(oldNode == highest) // oldNode->next == nullptr
        {
            highest = oldNode->prev;
        }
        else
        {
            oldNode->next->prev = oldNode->prev;
        }
        if(oldNode == lowest) // oldNode->prev == nullptr
        {
            lowest = oldNode->next;
        }
        else
        {
            oldNode->prev->next = oldNode->next;
        }
        jassert(lowest->prev == nullptr);
//        jassert(highest->next == nullptr);
//        jassert(oldNode != highMedian && oldNode != lowMedian && oldNode != median);

        *oldNode = llNode(); // reset
        auto cur = lowest;
        if(hasEvenLength())
        {
            for(int i = 0; i < numValidNodes; i++)
            {
                if(i == int(numValidNodes / 2) - 1)
                {
                    jassert(cur == lowMedian);
                }
                else if(i == int(numValidNodes / 2) - 1)
                {
                    jassert(cur == highMedian);
                }
                cur = cur->next;
            }
        }
        else
        {
            for(int i = 0; i < numValidNodes; i++)
            {
                if(i == int(numValidNodes / 2))
                {
                    jassert(cur == median);
                }
                cur = cur->next;
            }
        }
    }
}

void MedianFilter::updateMedian(llNode* changedNode, bool justPushed)
{
    if(median != nullptr)
    {
        jassert(!isnan(median->data));
    }
    else
    {
        jassert(!isnan(lowMedian->data) && !isnan(highMedian->data));
    }

    counter++;
    auto isNowEven = hasEvenLength();
    if(justPushed)
    {
        if(isNowEven)
        {
            if(changedNode->data > median->data)
            {
                lowMedian = median;
                highMedian = median->next;
            }
            else if(changedNode->data < median->data)
            {
                lowMedian = median->prev;
                highMedian = median;
            }
            else
            {
                lowMedian = median->prev;
                highMedian = median;
            }
            jassert(lowMedian->data <= highMedian->data);
            jassert(lowMedian != nullptr && highMedian != nullptr);
            median = nullptr;
        }
        else // is now odd
        {
            if(changedNode->data > highMedian->data)
            {
                median = highMedian;
                jassert(median != nullptr && median->next != nullptr && median->prev != nullptr);
            }
            else if(changedNode->data < lowMedian->data)
            {
                median = lowMedian;
                jassert(median != nullptr && median->next != nullptr && median->prev != nullptr);
            }
            else
            {
                median = highMedian->prev;
                jassert(median != nullptr);
            }
            lowMedian = nullptr;
            highMedian = nullptr;
        }
    }
    else // just popped
    {
        if(isNowEven)
        {
            if(median == changedNode)
            {
                lowMedian = median->prev;
                highMedian = median->next;
                jassert(lowMedian != changedNode && highMedian != changedNode);
                jassert(lowMedian->data <= highMedian->data);
            }
            else if(median->data == changedNode->data) // popped something with the same value as the median
            {
                swapNodes(changedNode, median);
                lowMedian = changedNode->prev;
                highMedian = changedNode->next;
                jassert(lowMedian != changedNode && highMedian != changedNode);
                jassert(lowMedian->data <= highMedian->data);
            }
            else if(changedNode->data > median->data) // popped above the median
            {
                lowMedian = median->prev;
                highMedian = median;
                jassert(lowMedian != changedNode && highMedian != changedNode);
                jassert(lowMedian->data <= highMedian->data);
            }
            else if(changedNode->data < median->data) // popped below the median
            {
                lowMedian = median;
                highMedian = median->next;
                jassert(lowMedian != changedNode && highMedian != changedNode);
                jassert(lowMedian->data <= highMedian->data);
            }
            median = nullptr;
            jassert(lowMedian->data <= highMedian->data);
        }
        else // is now odd
        {
            // TODO: add swapNodes here
            auto trueMedian = (lowMedian->data + highMedian->data)/2.f;
            if(lowMedian->data == changedNode->data && highMedian->data == changedNode->data && lowMedian != changedNode && highMedian != changedNode)
            {
                swapNodes(changedNode, highMedian);
                median = lowMedian;
                jassert(median != changedNode && lowMedian != changedNode);
            }
            else if(lowMedian->data == changedNode->data && highMedian != changedNode) // popped the low median or equivalent
            {
                median = highMedian;
                jassert(median != changedNode && highMedian != changedNode);
            }
            else if(highMedian->data == changedNode->data && lowMedian != changedNode)  // popped the high median or equivalent
            {
                median = lowMedian;
                jassert(median != changedNode && lowMedian != changedNode);
            }
            else if(changedNode->data > trueMedian) // popped above the median
            {
                median = lowMedian;
                jassert(median != changedNode);
            }
            else if(changedNode->data < trueMedian) // popped below the median
            {
                median = highMedian;
                jassert(median != changedNode);
            }
            lowMedian = nullptr;
            highMedian = nullptr;
        }
//        jassert(lowMedian != changedNode && highMedian != changedNode && median != changedNode);
    }
}

void MedianFilter::swapNodes(llNode* a, llNode* b)
{
    if(a->next == b) // adjacent
    {
        a->prev->next = b;
        b->next->prev = a;
        a->next = b->next;
        b->prev = a->prev;
        b->next = a;
        a->prev = b;
    }
    else if(b->next == a) // reverse adjacent
    {
        a->next->prev = b;
        b->prev->next = a;
        b->next = a->next;
        a->prev = b->prev;
        a->next = b;
        b->prev = a;
    }
    else if(a->prev == nullptr) // a == beginning of list
    {
        if(b->next == nullptr) // b == end of list
        {
            a->next->prev = b;
            b->prev->next = a;
            a->prev = b->prev;
            b->next = a->next;
            a->next = nullptr;
            b->prev = nullptr;
            highest = a;
        }
        else // b has prev and next pointers
        {
            a->next->prev = b;
            b->prev->next = a;
            b->next->prev = a;
            a->prev = b->prev;
            llNode* tmp = a->next;
            a->next = b->next;
            b->next = tmp;
            b->prev = nullptr;
        }
        lowest = b;
    }
    else if(a->next == nullptr) // a == end of list
    {
        if(b->prev == nullptr) // b == beginning of list
        {
            a->prev->next = b;
            b->next->prev = a;
            b->prev = a->prev;
            a->next = b->next;
            a->prev = nullptr;
            b->next = nullptr;
            lowest = a;
        }
        else
        {
            a->prev->next = b;
            b->prev->next = a;
            b->next->prev = a;
            llNode* tmp = a->prev;
            a->prev = b->prev;
            b->prev = tmp;
            a->next = b->next;
            b->next = nullptr;
        }
        highest = b;
    }
    else if(b->prev == nullptr) // b == lowest, we know here that a->next != nullptr
    {
        a->prev->next = b;
        a->next->prev = b;
        b->next->prev = a;
        llNode* tmp = a->next;
        a->next = b->next;
        b->next = tmp;
        b->prev = a->prev;
        a->prev = nullptr;
        lowest = a;
    }
    else if(b->next == nullptr) // b == highest, we know here that a->prev != nullptr
    {
        a->prev->next = b;
        a->next->prev = b;
        b->prev->next = a;
        llNode* tmp = a->prev;
        a->prev = b->prev;
        b->prev = tmp;
        b->next = a->next;
        a->next = nullptr;
        highest = a;
    }
    else // base case — nodes aren't related or at edges
    {
        a->prev->next = b;
        a->next->prev = b;
        b->prev->next = a;
        b->next->prev = a;
        llNode* tmp = a->prev;
        a->prev = b->prev;
        b->prev = tmp;
        tmp = a->next;
        a->next = b->next;
        b->next = tmp;
    }
}

float MedianFilter::getMedian()
{
    float output;
    if(isReady())
    {
        if(hasEvenLength())
        {
            output = (lowMedian->data + highMedian->data)/2.f;
        }
        else
        {
            output = median->data;
        }
    }
    else
    {
        output = 0;
    }
    return output;
}
