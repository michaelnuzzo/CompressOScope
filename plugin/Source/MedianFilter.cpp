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
    if((newOrder+1) - abstractFifo.getTotalSize() != 0)
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
        // overwrite fifo circularly
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
        newNode->data = val;

        if(lowest == nullptr) // first time through
        {
            lowest = newNode;
            highest = newNode;
            median = newNode;
        }
        else
        {
            // do a quick check to see if we are higher than the highest or the medians
            llNode* cur = nullptr;// = lowest;
            if(newNode->data > highest->data)
            {
                cur = highest;
            }
            else if(lowMedian != nullptr)
            {
                if(newNode->data > lowMedian->data)
                {
                    cur = lowMedian;
                }
                else
                {
                    cur = lowest;
                }
            }
            else if(median != nullptr)
            {
                if(newNode->data > median->data)
                {
                    cur = median;
                }
                else
                {
                    cur = lowest;
                }
            }
            // search through until we find a node higher than or equal to our new node
            while(cur->next != nullptr && newNode->data > cur->data)
            {
                cur = cur->next;
            }

            if(newNode->data <= cur->data) // base case, this node is the first higher or equal node
            {
                if(cur != lowest)
                {
                    cur->prev->next = newNode;
                    newNode->prev = cur->prev;
                }
                else
                {
                    lowest = newNode;
                }
                newNode->next = cur;
                cur->prev = newNode;
            }
            else // newNode is the highest node
            {
                newNode->prev = cur;
                cur->next = newNode;
                highest = newNode;
            }
            updateMedian(newNode, true); // mark the new median
        }
    }
//    checkAndDebugMedian();
}

void MedianFilter::pop()
{
    int start1, size1, start2, size2;
    abstractFifo.prepareToRead(1, start1, size1, start2, size2);
    jassert(size1 == 1);
    jassert(size2 == 0);
    abstractFifo.finishedRead(1);

    llNode* oldNode = &linkedList.getRawDataPointer()[start1];

    if(!isnan(oldNode->data))
    {
        numValidNodes--;

        updateMedian(oldNode, false);

        if(oldNode->next == nullptr && oldNode->prev != nullptr) // oldNode->next == nullptr
        {
            highest = oldNode->prev;
        }
        if(oldNode->next != nullptr)
        {
            oldNode->next->prev = oldNode->prev;
        }
        if(oldNode == lowest && oldNode->next != nullptr) // oldNode->prev == nullptr
        {
            lowest = oldNode->next;
        }
        else if(oldNode->prev != nullptr)
        {
            oldNode->prev->next = oldNode->next;
        }

        *oldNode = llNode(); // reset

//        checkAndDebugMedian();
    }
}

void MedianFilter::updateMedian(llNode* changedNode, bool justPushed)
{
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
            else // changedNode->data < median->data or changedNode->data == median->data
            {
                lowMedian = median->prev;
                highMedian = median;
            }
            median = nullptr;
        }
        else // is now odd
        {
            if(numValidNodes == 1) // first time through
            {
                median = changedNode;
            }
            else if(changedNode->data > highMedian->data)
            {
                median = highMedian;
            }
            else if(changedNode->data < lowMedian->data)
            {
                median = lowMedian;
            }
            else
            {
                median = highMedian->prev;
            }
            lowMedian = nullptr;
            highMedian = nullptr;
        }
    }
    else // just popped
    {
        if(isNowEven)
        {
            if(numValidNodes == 0) // popped last node
            {
                lowest = nullptr;
                highest = nullptr;
                median = nullptr;
            }
            else if(median->data == changedNode->data) // popped something with the same value as the median
            {
                if(median != changedNode)
                {
                    // update the changed node to be the median so we know where to move
                    swapNodes(changedNode, median);
                }
                lowMedian = changedNode->prev;
                highMedian = changedNode->next;
            }
            else if(changedNode->data > median->data) // popped above the median
            {
                lowMedian = median->prev;
                highMedian = median;
            }
            else if(changedNode->data < median->data) // popped below the median
            {
                lowMedian = median;
                highMedian = median->next;
            }
            median = nullptr;
        }
        else // is now odd
        {
            if(lowMedian->data == changedNode->data && highMedian->data == changedNode->data && lowMedian != changedNode && highMedian != changedNode)
            {
                swapNodes(changedNode, highMedian);
                median = lowMedian;
            }
            else if(changedNode->data <= lowMedian->data && highMedian != changedNode) // popped the low median or equivalent
            {
                median = highMedian;
            }
            else if(changedNode->data >= highMedian->data && lowMedian != changedNode)  // popped the high median or equivalent
            {
                median = lowMedian;
            }
            lowMedian = nullptr;
            highMedian = nullptr;
        }
    }
}

void MedianFilter::checkAndDebugMedian()
{
    /* objective check for median */
    // use this to verify the algorithm if you make changes,
    // but disable before releasing — it adds lots of time on
    // to the calculation
        auto cur = lowest;
        if(hasEvenLength())
            for(int i = 0; i < numValidNodes; i++)
            {
                jassert(!isnan(cur->data));
                if(i == int(numValidNodes / 2) - 1)
                    jassert(cur == lowMedian);
                else if(i == int(numValidNodes / 2) - 1)
                    jassert(cur == highMedian);
                cur = cur->next;
            }
        else
            for(int i = 0; i < numValidNodes; i++)
            {
                jassert(!isnan(cur->data));
                if(i == int(numValidNodes / 2))
                    jassert(cur == median);
                cur = cur->next;
            }
}

void MedianFilter::swapNodes(llNode* a, llNode* b)
{
    if(b->prev == nullptr || b->next == nullptr || b->next == a)
    {
        // doing this vastly reduces the number of cases we need to check for
        std::swap(a,b);
    }

    if(a->next == b) // adjacent
    {
        if(b->next == nullptr)
        {
            a->next = nullptr;
        }
        else
        {
            b->next->prev = a;
            a->next = b->next;
        }
        a->prev->next = b;
        b->prev = a->prev;
        b->next = a;
        a->prev = b;
    }
    else if(a->prev == nullptr) // a == beginning of list
    {
        a->next->prev = b;
        b->prev->next = a;

        if(b->next == nullptr) // b == end of list
        {
            a->prev = b->prev;
            b->next = a->next;
            a->next = nullptr;
            highest = a;
        }
        else // b has prev and next pointers
        {
            b->next->prev = a;
            a->prev = b->prev;
            llNode* tmp = a->next;
            a->next = b->next;
            b->next = tmp;
        }
        b->prev = nullptr;

        lowest = b;
    }
    else if(a->next == nullptr) // a == end of list
    {
        a->prev->next = b;
        b->next->prev = a;

        if(b->prev == nullptr) // b == beginning of list
        {
            b->prev = a->prev;
            a->next = b->next;
            a->prev = nullptr;
            lowest = a;
        }
        else
        {
            b->prev->next = a;
            llNode* tmp = a->prev;
            a->prev = b->prev;
            b->prev = tmp;
            a->next = b->next;
        }
        b->next = nullptr;
        highest = b;
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
        output = -1;
    }
    return output;
}
