#include "stdafx.h"
#include "event_queue.h"

static CEventQueue event_queue;

LPEVENT event_create_ex(TEVENTFUNC func, event_info_data* info, int32_t when)
{
    LPEVENT new_event = nullptr;

    if (when < 1)
    {
        when = 1;
    }

    new_event = new event;

    assert(nullptr != new_event);

    new_event->func = func;
    new_event->info = info;
    new_event->q_el = event_queue.Enqueue(new_event, when, thecore_heart->pulse);
    new_event->is_processing = false;
    new_event->is_force_to_end = false;

    return new_event;
}

void event_cancel(LPEVENT* pEvent)
{
    if (!pEvent)
    {
        return;
    }

    LPEVENT event = *pEvent;

    if (!event)
    {
        return;
    }

    if (event->is_processing)
    {
        event->is_force_to_end = true;

        if (event->q_el)
        {
            event->q_el->bCancel = true;
        }

        *pEvent = nullptr;
        return;
    }

    if (!event->q_el)
    {
        *pEvent = nullptr;
        return;
    }

    if (event->q_el->bCancel)
    {
        *pEvent = nullptr;
        return;
    }

    event->q_el->bCancel = true;
    *pEvent = nullptr;
}

void event_reset_time(LPEVENT event, int32_t when)
{
    if (!event->is_processing)
    {
        if (event->q_el)
        {
            event->q_el->bCancel = true;
        }

        event->q_el = event_queue.Enqueue(event, when, thecore_heart->pulse);
    }
}


int32_t event_process(int32_t pulse)
{
    int32_t new_time;
    int num_events = 0;

    while (pulse >= event_queue.GetTopKey())
    {
        TQueueElement * pElem = event_queue.Dequeue();
        if (pElem->bCancel)
        {
            event_queue.Delete(pElem);
            continue;
        }

        new_time = pElem->iKey;

        LPEVENT the_event = pElem->pvData;
        int32_t processing_time = event_processing_time(the_event);

        event_queue.Delete(pElem);

        the_event->q_el = nullptr;
        the_event->is_processing = true;

        assert(the_event->info);

        new_time = (the_event->func)(the_event, processing_time);
        if (new_time <= 0
            || the_event->is_force_to_end)
        {
            the_event->q_el = nullptr;
        }
        else
        {
            the_event->q_el = event_queue.Enqueue(the_event, new_time, pulse);
            the_event->is_processing = false;
        }

        ++num_events;
    }

    return num_events;
}


int32_t event_processing_time(LPEVENT event)
{
    int32_t start_time;
    if (!event->q_el)
    {
        return 0;
    }

    start_time = event->q_el->iStartTime;
    return (thecore_heart->pulse - start_time);
}


int32_t event_time(LPEVENT event)
{
    int32_t when;
    if (!event->q_el)
    {
        return 0;
    }

    when = event->q_el->iKey;
    return (when - thecore_heart->pulse);
}

void event_destroy(void)
{
    TQueueElement * pElem;
    while ((pElem = event_queue.Dequeue()))
    {
        LPEVENT the_event = (LPEVENT) pElem->pvData;
        if (!pElem->bCancel)
        {
            
        }

        event_queue.Delete(pElem);
    }
}

int32_t event_count()
{
    return event_queue.Size();
}

void intrusive_ptr_add_ref(EVENT* p)
{
    ++(p->ref_count);
}

void intrusive_ptr_release(EVENT* p)
{
    if (--(p->ref_count) == 0)
    {
        delete p;
    }
}
