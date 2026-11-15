#include <stdlib.h>
#include <string.h>

#include "wind_meteors_results.h"

int WindMeteorsResultsAdd(WindMeteorsResults* results, const void* record, unsigned int size)
{
    if (size < sizeof(int) || size > sizeof(WindMeteorsResult))
        return 0;
    if (results->count == results->capacity) {
        unsigned int grown = results->capacity == 0 ? 256 : results->capacity * 2;
        WindMeteorsResult* items = (WindMeteorsResult*)realloc(results->items, grown * sizeof(WindMeteorsResult));
        if (items == nullptr)
            return 0;
        results->items = items;
        results->capacity = grown;
    }
    WindMeteorsResult* r = &results->items[results->count++];
    memset(r, 0, sizeof *r);
    memcpy(r, record, size);
    r->size = size;
    return 1;
}

void WindMeteorsResultsClear(WindMeteorsResults* results)
{
    results->count = 0;
}
