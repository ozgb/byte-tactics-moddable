#include <stdlib.h>
#include <string.h>

#include "path_results.h"
#include "../../interfaces/pathfinding.h"

int PathResultsAdd(PathResults* results, const PathEvent* event, const PathPoint* points, int capacity)
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
    r->status = event->status;
    r->unitRef = event->unit;
    r->tick = event->tick;
    r->count = event->count;
    int delivered = event->count < 0 ? 0 : event->count;
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
