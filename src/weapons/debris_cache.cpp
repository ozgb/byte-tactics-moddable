#include "../state/block.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"
#include "../graphics/memory_cache.h"

DebrisCache* DebrisCache::ClearPointers()
{
    base = nullptr;
    handle = nullptr;
    return this;
}

int DebrisCache::Install(void* block, unsigned int newSize)
{
    Chunk* chunk = (Chunk*)block;
    base = chunk;
    cap = newSize;
    cur = chunk;
    chunk->owner = nullptr;
    chunk->size = cap;
    return 1;
}

int DebrisCache::InitSimCache(unsigned int newSize)
{
    FreeCache();
    return Install(SimAllocTagged("CMemoryCache CCH", newSize), newSize);
}

void DebrisCache::FreeCache()
{
    if (base.get() != nullptr) {
        GameFreeThunk((int*)base.get());
        base = nullptr;
    }
}

void DebrisCache::FreeBuffer()
{
    GameFreeThunk((int*)base.get());
}

static void Release(Ref<char>* owner, int block)
{
    if (owner != nullptr && (int)owner->get() == block + 8)
        *owner = nullptr;
}

int DebrisCache::AllocBytes(Ref<char>* slot, int need)
{
    typedef DebrisCacheChunk Chunk;
    int total = 0;
    if (need <= 0) {
        *slot = nullptr;
        return 0;
    }
    int size = need + 8;
    int existing = (int)slot->get();
    if (existing != 0)
        existing = *(int*)(existing - 4);
    if (existing >= size)
        return 1;
    if (size > cap) {
        *slot = nullptr;
        return 0;
    }
    Chunk* start;
    int first = (int)base.get();
    int next = (int)cur.get();
    if (first == 0 || next == 0) {
        *slot = nullptr;
        return 0;
    }
    int end = first + cap;
    if (next + size > end) {
        int q = next;
        start = (Chunk*)first;
        while (q < end) {
            Release(((Chunk*)q)->owner.get(), q);
            q += *(int*)(q + 4);
            end = first + cap;
        }
    } else {
        start = cur.get();
    }
    int q = (int)start;
    while (total < size) {
        total += *(int*)(q + 4);
        Release(((Chunk*)q)->owner.get(), q);
        q += *(int*)(q + 4);
    }
    if ((unsigned int)(total - size) > 8u) {
        Chunk* nc = (Chunk*)((char*)start + size);
        nc->owner = nullptr;
        nc->size = total - size;
        cur = nc;
    } else {
        size = total;
        int e = (int)start + total;
        if (e >= first + cap)
            e = first;
        cur = (Chunk*)e;
    }
    start->owner = slot;
    start->size = size;
    *slot = (char*)((char*)start + 8);
    return 1;
}
