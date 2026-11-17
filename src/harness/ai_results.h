#ifndef HARNESS_AI_RESULTS_H
#define HARNESS_AI_RESULTS_H

#include <stdlib.h>
#include <string.h>

#include "../../interfaces/ai.h"

struct AiResult {
    AiEvent event;
    unsigned int unitTag;
    unsigned int unitId;
    unsigned int targetTag;
    unsigned int targetId;
    int outcome;
};

struct AiResults {
    AiResult* items;
    unsigned int count;
    unsigned int capacity;
};

static inline int AiResultsAdd(AiResults* results, const AiEvent* event, int outcome)
{
    if (results->count == results->capacity) {
        unsigned int grown = results->capacity == 0 ? 4096 : results->capacity * 2;
        AiResult* items = (AiResult*)realloc(results->items, grown * sizeof(AiResult));
        if (items == nullptr)
            return 0;
        results->items = items;
        results->capacity = grown;
    }
    AiResult* r = &results->items[results->count++];
    memset(r, 0, sizeof *r);
    r->event = *event;
    r->outcome = outcome;
    return 1;
}

static inline void AiResultsClear(AiResults* results)
{
    results->count = 0;
}

#endif
