#include <stdlib.h>
#include <string.h>

#include "event_results.h"

int EventResultsAdd(EventResults* results, const void* event, unsigned int size, int outcome)
{
    if (size < sizeof(int) || size > EVENT_RESULT_BYTES)
        return EVENT_RESULT_BAD_SIZE;
    if (results->count == results->capacity) {
        unsigned int grown = results->capacity == 0 ? 256 : results->capacity * 2;
        EventResult* items = (EventResult*)realloc(results->items, grown * sizeof(EventResult));
        if (items == nullptr)
            return EVENT_RESULT_NO_MEMORY;
        results->items = items;
        results->capacity = grown;
    }
    EventResult* r = &results->items[results->count++];
    memset(r, 0, sizeof *r);
    memcpy(r->data, event, size);
    r->size = size;
    r->outcome = outcome;
    return EVENT_RESULT_ADDED;
}

void EventResultsClear(EventResults* results)
{
    results->count = 0;
}
