#ifndef HARNESS_WEAPONS_RESULTS_H
#define HARNESS_WEAPONS_RESULTS_H

#include <stdlib.h>
#include <string.h>

#include "../../interfaces/weapons.h"

struct WeaponsResult {
    WeaponsEvent event;
    unsigned int unitTag;
    unsigned int unitId;
    unsigned int targetTag;
    unsigned int targetId;
    int outcome;
};

struct WeaponsResults {
    WeaponsResult* items;
    unsigned int count;
    unsigned int capacity;
};

static inline int WeaponsResultsAdd(WeaponsResults* results, const WeaponsEvent* event, int outcome)
{
    if (results->count == results->capacity) {
        unsigned int grown = results->capacity == 0 ? 4096 : results->capacity * 2;
        WeaponsResult* items = (WeaponsResult*)realloc(results->items, grown * sizeof(WeaponsResult));
        if (items == nullptr)
            return 0;
        results->items = items;
        results->capacity = grown;
    }
    WeaponsResult* r = &results->items[results->count++];
    memset(r, 0, sizeof *r);
    r->event = *event;
    r->outcome = outcome;
    return 1;
}

static inline void WeaponsResultsClear(WeaponsResults* results)
{
    results->count = 0;
}

#endif
