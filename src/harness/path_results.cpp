#include <stdlib.h>
#include <string.h>

#include "path_results.h"

static const unsigned long long kPrime = 0x100000001b3ull;

static unsigned long long HashByte(unsigned long long h, unsigned char v)
{
    return (h ^ v) * kPrime;
}

static unsigned long long HashU32(unsigned long long h, unsigned int v)
{
    h = HashByte(h, (unsigned char)v);
    h = HashByte(h, (unsigned char)(v >> 8));
    h = HashByte(h, (unsigned char)(v >> 16));
    return HashByte(h, (unsigned char)(v >> 24));
}

static unsigned long long HashRun(unsigned long long h, const unsigned char* p, unsigned int n)
{
    while (n >= 8) {
        unsigned long long word;
        memcpy(&word, p, 8);
        h = (h ^ word) * kPrime;
        p += 8;
        n -= 8;
    }
    while (n > 0) {
        h = HashByte(h, *p++);
        n--;
    }
    return h;
}

int PathResultsAdd(PathResults* results, const PathEvent* event, const PathPoint* points, int capacity, unsigned int unitTag,
                   unsigned int unitId)
{
    if (results->count == results->capacity) {
        unsigned int grown = results->capacity == 0 ? 64 : results->capacity * 2;
        PathResult* items = (PathResult*)realloc(results->items, grown * sizeof(PathResult));
        if (items == 0)
            return 0;
        results->items = items;
        results->capacity = grown;
    }
    PathResult* r = &results->items[results->count++];
    memset(r, 0, sizeof *r);
    r->kind = event->kind;
    r->status = event->status;
    r->unitTag = unitTag;
    r->unitId = unitId;
    r->tick = event->tick;
    r->count = event->count;
    int delivered = event->count < 0 ? 0 : event->count;
    if (delivered > capacity)
        delivered = capacity;
    if (delivered > PATH_RESULT_POINTS)
        delivered = PATH_RESULT_POINTS;
    r->delivered = delivered;
    if (delivered > 0)
        memcpy(r->points, points, delivered * sizeof(PathPoint));
    return 1;
}

void PathResultsClear(PathResults* results)
{
    results->count = 0;
}

unsigned long long PathResultsHash(const PathResults* results, unsigned long long h)
{
    h = HashByte(h, PATH_RESULT_SECTION);
    h = HashU32(h, results->count);
    for (unsigned int k = 0; k < results->count; k++) {
        const PathResult* r = &results->items[k];
        h = HashU32(h, (unsigned int)r->kind);
        h = HashU32(h, (unsigned int)r->status);
        h = HashByte(h, (unsigned char)r->unitTag);
        h = HashU32(h, r->unitId);
        h = HashU32(h, r->tick);
        h = HashU32(h, (unsigned int)r->count);
        h = HashRun(h, (const unsigned char*)r->points, r->delivered * sizeof(PathPoint));
    }
    return HashU32(h, 0xffffffffu);
}
