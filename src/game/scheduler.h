#ifndef GAME_SCHEDULER_H
#define GAME_SCHEDULER_H

#include "../../interfaces/rand_stream.h"

typedef void (*SchedHandler)(const void* event);

struct SchedRoute {
    int outbox;
    int kind;
    int subscriber;
    SchedHandler handler;
    int synchronous;
};

const SchedRoute* SchedRoutes(int* count);

void __stdcall SchedDeliver(int module);
void SchedBeginTick();
void SchedPhaseBoundary();
void SchedEndTick();
void SchedUnitsPhase();
void SchedPlayersPhase(RandStream stream);

#endif
