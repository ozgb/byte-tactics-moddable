#ifndef STATE_SIM_EVENTS_H
#define STATE_SIM_EVENTS_H

#include "../../interfaces/services.h"

enum : unsigned int { SIM_EVENT_CAPACITY = 256 };

struct SimEvent {
    unsigned short kind;
    unsigned short flags;
    unsigned int tick;
    unsigned int args[6];
};

struct SimEventQueue {
    unsigned int head;
    unsigned int tail;
    unsigned int dropped;
    unsigned int reserved;
    SimEvent records[SIM_EVENT_CAPACITY];
};

static_assert(sizeof(SimEvent) == 32, "SimEvent size");

extern SimEventQueue* g_simEvents;

SimEventQueue& SimEvents();

void SimEventsCreate();

void SimEventsClear();

#endif
