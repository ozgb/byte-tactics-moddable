#ifndef HARNESS_TARGETING_RESULTS_H
#define HARNESS_TARGETING_RESULTS_H

#include <stdlib.h>
#include <string.h>

#include "../../interfaces/targeting.h"

enum { TARGETING_RESULT_SECTION = 38 };

struct TargetingResult {
    TargetingEvent event;
    unsigned int unitTag;
    unsigned int unitId;
    unsigned int targetTag;
    unsigned int targetId;
    int outcome;
};

struct TargetingResults {
    TargetingResult* items;
    unsigned int count;
    unsigned int capacity;
};

static inline int TargetingResultsAdd(TargetingResults* results, const TargetingEvent* event, int outcome)
{
    if (results->count == results->capacity) {
        unsigned int grown = results->capacity == 0 ? 256 : results->capacity * 2;
        TargetingResult* items = (TargetingResult*)realloc(results->items, grown * sizeof(TargetingResult));
        if (items == nullptr)
            return 0;
        results->items = items;
        results->capacity = grown;
    }
    TargetingResult* r = &results->items[results->count++];
    memset(r, 0, sizeof *r);
    r->event = *event;
    r->outcome = outcome;
    return 1;
}

static inline void TargetingResultsClear(TargetingResults* results)
{
    results->count = 0;
}

#endif
