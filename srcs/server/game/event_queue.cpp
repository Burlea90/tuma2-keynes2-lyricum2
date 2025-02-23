#include "stdafx.h"
#include "event_queue.h"

CEventQueue::CEventQueue()
{
}

CEventQueue::~CEventQueue()
{
    Destroy();
}

void CEventQueue::Destroy()
{
    while (!m_pq_queue.empty())
    {
        TQueueElement * pElem = m_pq_queue.top();
        m_pq_queue.pop();

        Delete(pElem);
    }
}

TQueueElement * CEventQueue::Enqueue(LPEVENT pvData, int32_t duration, int32_t pulse)
{
    TQueueElement * pElem = new TQueueElement;

    pElem->pvData = pvData;
    pElem->iStartTime = pulse;
    pElem->iKey = duration + pulse;
    pElem->bCancel = false;

    m_pq_queue.push(pElem);
    return pElem;
}

TQueueElement * CEventQueue::Dequeue()
{
    if (m_pq_queue.empty())
    {
        return nullptr;
    }

    TQueueElement* pElem = m_pq_queue.top();
    m_pq_queue.pop();
    return pElem;
}

void CEventQueue::Delete(TQueueElement * pElem)
{
    if (pElem)
    {
        delete pElem;
    }
}

int32_t CEventQueue::GetTopKey()
{
    if (m_pq_queue.empty())
    {
        return INT_MAX;
    }

    return m_pq_queue.top()->iKey;
}

int32_t CEventQueue::Size()
{
    return m_pq_queue.size();
}
