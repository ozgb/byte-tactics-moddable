#include "sim_events.h"
#include "sim_globals.h"
#include "../game/game.h"
#include "../../link/globals.h"

SimEventQueue* g_simEvents;

void SimEventsCreate()
{
    g_simEvents = static_cast<SimEventQueue*>(SimAllocTagged("SIM EVENTS", sizeof(SimEventQueue)));
    SimBlockTrack(reinterpret_cast<void**>(&g_simEvents));
}

SimEventQueue& SimEvents()
{
    if (g_simEvents == 0)
        SimCreate();
    return *g_simEvents;
}

void SimEventsClear()
{
    SimEventQueue& q = SimEvents();
    q.head = 0;
    q.tail = 0;
    q.dropped = 0;
}

void __stdcall RecordSimEvent(SimEventKind kind, unsigned int flags, const unsigned int* args, unsigned int count)
{
    SimEventQueue* queue = g_simEvents;
    if (queue == 0) {
        if (HarnessTestMode())
            FatalError("The sim event queue does not exist.");
        return;
    }
    SimEventQueue& q = *queue;
    if (q.tail - q.head >= SIM_EVENT_CAPACITY) {
        if (HarnessTestMode())
            FatalError("The sim event queue is full.");
        q.dropped++;
        return;
    }
    SimEvent& e = q.records[q.tail % SIM_EVENT_CAPACITY];
    e.kind = kind;
    e.flags = (unsigned short)flags;
    e.tick = (unsigned int)g_game->ticks;
    for (unsigned int i = 0; i < 6; i++)
        e.args[i] = i < count ? args[i] : 0;
    q.tail++;
}
