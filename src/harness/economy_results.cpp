#include <stdlib.h>
#include <string.h>

#include "economy_results.h"

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

static unsigned int FloatBits(float v)
{
    unsigned int bits;
    memcpy(&bits, &v, sizeof bits);
    return bits;
}

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
    if (event->kind == ECONOMY_LOAD && event->account != nullptr)
        memcpy(r->account, event->account, sizeof r->account);
    return 1;
}

void EconomyResultsClear(EconomyResults* results)
{
    results->count = 0;
}

unsigned long long EconomyResultsHash(const EconomyResults* results, unsigned long long h)
{
    h = HashByte(h, ECONOMY_RESULT_SECTION);
    h = HashU32(h, results->count);
    for (unsigned int k = 0; k < results->count; k++) {
        const EconomyResult* r = &results->items[k];
        h = HashU32(h, (unsigned int)r->kind);
        h = HashU32(h, (unsigned int)r->player);
        h = HashByte(h, (unsigned char)r->unitTag);
        h = HashU32(h, r->unitId);
        h = HashU32(h, r->resources);
        h = HashU32(h, FloatBits(r->energy));
        h = HashU32(h, FloatBits(r->metal));
        h = HashU32(h, (unsigned int)r->value);
        h = HashU32(h, (unsigned int)r->outcome);
        if (r->kind == ECONOMY_LOAD)
            for (int w = 0; w < ECONOMY_RESULT_ACCOUNT_WORDS; w++)
                h = HashU32(h, r->account[w]);
    }
    return HashU32(h, 0xffffffffu);
}
