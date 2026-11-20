#include <stdlib.h>
#include <string.h>

#include "path_results.h"
#include "../../interfaces/pathfinding.h"

int PathResultsAdd(PathResults* results, const PathfindingEventHeader* event, unsigned int size, const PathPoint* points, int capacity)
{
    if (results->count == results->capacity) {
        unsigned int grown = results->capacity == 0 ? 64 : results->capacity * 2;
        PathResult* items = (PathResult*)realloc(results->items, grown * sizeof(PathResult));
        if (items == nullptr)
            return 0;
        results->items = items;
        results->capacity = grown;
    }
    PathResult* r = &results->items[results->count++];
    memset(r, 0, sizeof *r);
    r->kind = event->kind;
    int status = 0;
    int count = 0;
    if (event->kind == PATH_EVENT_STARTED && size >= sizeof(PathfindingStarted))
        status = ((const PathfindingStarted*)event)->start;
    else if (event->kind == PATH_EVENT_PROGRESS && size >= sizeof(PathfindingProgress))
        count = ((const PathfindingProgress*)event)->count;
    else if (event->kind == PATH_EVENT_FINISHED && size >= sizeof(PathfindingFinished)) {
        status = ((const PathfindingFinished*)event)->status;
        count = ((const PathfindingFinished*)event)->count;
    }
    r->status = status;
    r->unitRef = event->unit;
    r->tick = event->tick;
    r->count = count;
    int delivered = count < 0 ? 0 : count;
    if (delivered > capacity)
        delivered = capacity;
    if (delivered > PATH_RESULT_POINTS)
        delivered = PATH_RESULT_POINTS;
    r->delivered = delivered;
    if (delivered > 0)
        memcpy(r->points, points, delivered * sizeof(PathPoint));
    return 1;
}

void PathResultsClear(PathResults* results)
{
    results->count = 0;
}
