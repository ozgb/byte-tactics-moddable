#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "wind_meteors_results.h"

int WindMeteorsResultsAdd(WindMeteorsResults* results, const void* record, unsigned int size)
{
    if (size < sizeof(int) || size > offsetof(WindMeteorsResult, size))
        return WIND_METEORS_RESULT_TOO_LARGE;
    if (results->count == results->capacity) {
        unsigned int grown = results->capacity == 0 ? 256 : results->capacity * 2;
        WindMeteorsResult* items = (WindMeteorsResult*)realloc(results->items, grown * sizeof(WindMeteorsResult));
        if (items == nullptr)
            return WIND_METEORS_RESULT_NO_MEMORY;
        results->items = items;
        results->capacity = grown;
    }
    WindMeteorsResult* r = &results->items[results->count++];
    memset(r, 0, sizeof *r);
    memcpy(r, record, size);
    r->size = size;
    return WIND_METEORS_RESULT_ADDED;
}

void WindMeteorsResultsClear(WindMeteorsResults* results)
{
    results->count = 0;
}
