#ifndef HARNESS_FEATURES_RESULTS_H
#define HARNESS_FEATURES_RESULTS_H

#include <stdlib.h>
#include <string.h>

#include "../../interfaces/features.h"

enum { FEATURES_RESULT_SECTION = 36 };

struct FeaturesResult {
    FeaturesEvent event;
    unsigned int unitTag;
    unsigned int unitId;
    int outcome;
};

struct FeaturesResults {
    FeaturesResult* items;
    unsigned int count;
    unsigned int capacity;
};

static inline int FeaturesResultsAdd(FeaturesResults* results, const FeaturesEvent* event, int outcome)
{
    if (results->count == results->capacity) {
        unsigned int grown = results->capacity == 0 ? 256 : results->capacity * 2;
        FeaturesResult* items = (FeaturesResult*)realloc(results->items, grown * sizeof(FeaturesResult));
        if (items == nullptr)
            return 0;
        results->items = items;
        results->capacity = grown;
    }
    FeaturesResult* r = &results->items[results->count++];
    memset(r, 0, sizeof *r);
    r->event = *event;
    r->outcome = outcome;
    return 1;
}

static inline void FeaturesResultsClear(FeaturesResults* results)
{
    results->count = 0;
}

#endif
