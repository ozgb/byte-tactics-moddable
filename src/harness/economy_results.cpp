#include <stdlib.h>
#include <string.h>

#include "economy_results.h"
#include "../../interfaces/economy.h"

int EconomyResultsAdd(EconomyResults* results, const EconomyEvent* event, int outcome)
{
    if (results->count == results->capacity) {
        unsigned int grown = results->capacity == 0 ? 256 : results->capacity * 2;
        EconomyResult* items = (EconomyResult*)realloc(results->items, grown * sizeof(EconomyResult));
        if (items == nullptr)
            return 0;
        results->items = items;
        results->capacity = grown;
    }
    EconomyResult* r = &results->items[results->count++];
    memset(r, 0, sizeof *r);
    r->kind = event->kind;
    r->player = event->player;
    r->unitRef = event->unit;
    r->resources = event->resources;
    r->energy = event->energy;
    r->metal = event->metal;
    r->value = event->value;
    r->outcome = outcome;
    if (event->kind == ECONOMY_LOAD)
        memcpy(r->account, &event->account, sizeof r->account);
    return 1;
}

void EconomyResultsClear(EconomyResults* results)
{
    results->count = 0;
}
