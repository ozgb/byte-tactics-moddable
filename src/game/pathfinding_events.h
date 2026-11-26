#ifndef GAME_PATHFINDING_EVENTS_H
#define GAME_PATHFINDING_EVENTS_H

#include <string.h>

#include "../../interfaces/pathfinding.h"
#include "../util/engine_services.h"

inline void PathfindingPublishSetSearch(int outbox, unsigned int steps, int hasScale, int scale)
{
    PathfindingSetSearch event;
    memset(&event, 0, sizeof(event));
    event.kind = PATHFINDING_SET_SEARCH;
    event.steps = steps;
    event.hasScale = hasScale;
    event.scale = scale;
    SchedPublish(outbox, &event, sizeof(event));
}

#endif
