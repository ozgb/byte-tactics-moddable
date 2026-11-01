#include "sim_events.h"
#include "sim_globals.h"
#include "block.h"
#include "../../interfaces/services.h"
#include "../game/game.h"
#include "../../link/globals.h"

static SimEventQueue* g_simEvents;

void SimEventsCreate()
{
    g_simEvents = static_cast<SimEventQueue*>(SimAllocTagged("SIM EVENTS", sizeof(SimEventQueue)));
    SimBlockTrack(reinterpret_cast<void**>(&g_simEvents));
}

SimEventQueue& SimEvents()
{
    if (g_simEvents == nullptr)
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
    if (queue == nullptr) {
        if (HarnessTestMode())
            FatalError("The sim event queue does not exist.");
        return;
    }
    SimEventQueue& q = *queue;
    unsigned int records = count > 6 ? (count + 5) / 6 : 1;
    if (q.tail - q.head + records > SIM_EVENT_CAPACITY) {
        if (HarnessTestMode())
            FatalError("The sim event queue is full.");
        q.dropped++;
        return;
    }
    for (unsigned int r = 0; r < records; r++) {
        SimEvent& e = q.records[q.tail % SIM_EVENT_CAPACITY];
        e.kind = kind;
        e.flags = (unsigned short)(r == 0 ? flags : (flags | SIM_EVENT_CONTINUED));
        e.tick = (unsigned int)g_game->ticks;
        for (unsigned int i = 0; i < 6; i++) {
            unsigned int n = r * 6 + i;
            e.args[i] = n < count ? args[n] : 0;
        }
        q.tail++;
    }
}
