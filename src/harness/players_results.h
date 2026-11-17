#ifndef HARNESS_PLAYERS_RESULTS_H
#define HARNESS_PLAYERS_RESULTS_H

#include <stdlib.h>
#include <string.h>

#include "../../interfaces/players.h"

enum { PLAYERS_RESULT_SECTION = 41 };

struct PlayersResult {
    PlayersEvent event;
    unsigned int unitTag;
    unsigned int unitId;
    int outcome;
};

struct PlayersResults {
    PlayersResult* items;
    unsigned int count;
    unsigned int capacity;
};

static inline int PlayersResultsAdd(PlayersResults* results, const PlayersEvent* event, int outcome)
{
    if (results->count == results->capacity) {
        unsigned int grown = results->capacity == 0 ? 4096 : results->capacity * 2;
        PlayersResult* items = (PlayersResult*)realloc(results->items, grown * sizeof(PlayersResult));
        if (items == nullptr)
            return 0;
        results->items = items;
        results->capacity = grown;
    }
    PlayersResult* r = &results->items[results->count++];
    memset(r, 0, sizeof *r);
    r->event = *event;
    r->outcome = outcome;
    return 1;
}

static inline void PlayersResultsClear(PlayersResults* results)
{
    results->count = 0;
}

#endif
