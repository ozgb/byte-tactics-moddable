#ifndef HARNESS_LOS_RADAR_RESULTS_H
#define HARNESS_LOS_RADAR_RESULTS_H

#include <stdlib.h>
#include <string.h>

#include "../../interfaces/los_radar.h"

enum { LOS_RADAR_RESULT_SECTION = 39 };

struct LosRadarResult {
    LosRadarEvent event;
    unsigned int unitTag;
    unsigned int unitId;
    int outcome;
};

struct LosRadarResults {
    LosRadarResult* items;
    unsigned int count;
    unsigned int capacity;
};

static inline int LosRadarResultsAdd(LosRadarResults* results, const LosRadarEvent* event, int outcome)
{
    if (results->count == results->capacity) {
        unsigned int grown = results->capacity == 0 ? 4096 : results->capacity * 2;
        LosRadarResult* items = (LosRadarResult*)realloc(results->items, grown * sizeof(LosRadarResult));
        if (items == nullptr)
            return 0;
        results->items = items;
        results->capacity = grown;
    }
    LosRadarResult* r = &results->items[results->count++];
    memset(r, 0, sizeof *r);
    r->event = *event;
    r->outcome = outcome;
    return 1;
}

static inline void LosRadarResultsClear(LosRadarResults* results)
{
    results->count = 0;
}

#endif
