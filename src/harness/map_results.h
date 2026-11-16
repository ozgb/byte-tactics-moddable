#ifndef HARNESS_MAP_RESULTS_H
#define HARNESS_MAP_RESULTS_H

#include <stdlib.h>
#include <string.h>

#include "../../interfaces/map.h"

enum { MAP_RESULT_SECTION = 40 };

struct MapResult {
    MapEvent event;
    unsigned int unitTag;
    unsigned int unitId;
    int outcome;
};

struct MapResults {
    MapResult* items;
    unsigned int count;
    unsigned int capacity;
};

static inline int MapResultsAdd(MapResults* results, const MapEvent* event, int outcome)
{
    if (results->count == results->capacity) {
        unsigned int grown = results->capacity == 0 ? 4096 : results->capacity * 2;
        MapResult* items = (MapResult*)realloc(results->items, grown * sizeof(MapResult));
        if (items == nullptr)
            return 0;
        results->items = items;
        results->capacity = grown;
    }
    MapResult* r = &results->items[results->count++];
    memset(r, 0, sizeof *r);
    r->event = *event;
    r->outcome = outcome;
    return 1;
}

static inline void MapResultsClear(MapResults* results)
{
    results->count = 0;
}

#endif
