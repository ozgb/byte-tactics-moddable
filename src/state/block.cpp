#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "block.h"
#include "../harness/replay.h"
#include "../harness/replay_services.h"
#include "../../interfaces/host.h"
#include "../../lib/bitexact/bitexact.h"

extern void (*g_outOfMemoryHandler)();

#pragma init_seg(lib)

const SimHeap kSim = SimHeap();
const AllocHooks* g_allocHooks;

namespace {

struct BlockHeader {
    unsigned int ready;
    unsigned int freeHead;
    unsigned int inUse;
    unsigned int highWater;
    bx_rand_state streams[2];
    unsigned int size;
    unsigned int searchSteps;
    unsigned int reserved[8];
};

static_assert(sizeof(BlockHeader) == 64, "the block header is 64 bytes");
static_assert(offsetof(BlockHeader, streams) == 0x10, "the rand streams sit at 0x10 and 0x14");

struct BlockChunk {
    unsigned int size;
    unsigned int aux;
};

const unsigned int kFreeBit = 1;
const unsigned int kChunkHeader = sizeof(BlockChunk);
const unsigned int kMinChunk = 16;
const unsigned int kFirstChunk = sizeof(BlockHeader);

char* g_base;
bool g_growable;
unsigned int g_insideThread;
void* g_lock = host_mutex_create();
void** g_tracked[8];
unsigned int g_trackedCount;

void Fatal(const char* what)
{
    unsigned int size = g_base != nullptr && ((BlockHeader*)g_base)->size != 0 ? ((BlockHeader*)g_base)->size : kSimBlockBytes;
    fprintf(stderr, "sim block: %s (size %u, ceiling %u)\n", what, size, kSimBlockCeilingBytes);
    abort();
}

void OutOfBlock()
{
    Fatal("out of memory, the match setup's block size is too small");
}

void Reserve()
{
    void* p = SimBlockNew(kSimBlockBytes);
    if (p == 0)
        OutOfBlock();
    g_base = (char*)p;
    g_growable = true;
}

BlockHeader* Head()
{
    return (BlockHeader*)g_base;
}

BlockChunk* At(unsigned int off)
{
    return (BlockChunk*)(g_base + off);
}

unsigned int Size()
{
    unsigned int size = Head()->size;
    return size != 0 ? size : kSimBlockBytes;
}

void Prepare()
{
    if (g_base == 0)
        Reserve();
    BlockHeader* h = Head();
    if (h->ready == 0) {
        h->ready = 1;
        h->size = kSimBlockBytes;
        h->streams[0].holdrand = 1;
        h->streams[1].holdrand = 1;
        h->freeHead = kFirstChunk;
        BlockChunk* first = At(kFirstChunk);
        first->size = ((kSimBlockBytes - kFirstChunk) & ~7u) | kFreeBit;
        first->aux = 0;
    }
}

struct Guard {
    Guard()
    {
        if (g_lock == 0)
            Fatal("used before its static initialisers ran");
        host_mutex_lock(g_lock);
        if (g_insideThread != 0)
            Fatal("re-entered the allocator");
        g_insideThread = host_thread_id();
        Prepare();
    }
    ~Guard()
    {
        g_insideThread = 0;
        host_mutex_unlock(g_lock);
    }
};

unsigned int ChunkSizeFor(unsigned int size)
{
    if (size > Size())
        return Size() + kMinChunk;
    unsigned int need = ((size + 7) & ~7u) + kChunkHeader;
    return need < kMinChunk ? kMinChunk : need;
}

void CountUse(unsigned int bytes)
{
    BlockHeader* h = Head();
    h->inUse += bytes;
    if (h->inUse > h->highWater)
        h->highWater = h->inUse;
}

BlockChunk* Lookup(void* p)
{
    unsigned int off = (unsigned int)((char*)p - g_base) - kChunkHeader;
    if (g_base == 0 || off >= Size() || off < kFirstChunk || (off & 7) != 0)
        Fatal("pointer is not a sim block");
    BlockChunk* c = At(off);
    if ((c->size & kFreeBit) != 0)
        Fatal("pointer is already free");
    return c;
}

void* AllocLocked(unsigned int size)
{
    unsigned int need = ChunkSizeFor(size);
    for (int attempt = 0;; attempt++) {
        unsigned int prev = 0;
        unsigned int off = Head()->freeHead;
        while (off != 0) {
            BlockChunk* c = At(off);
            unsigned int sz = c->size & ~kFreeBit;
            if (sz >= need) {
                unsigned int next = c->aux;
                unsigned int link = next;
                if (sz - need >= kMinChunk) {
                    unsigned int restOff = off + need;
                    BlockChunk* rest = At(restOff);
                    rest->size = (sz - need) | kFreeBit;
                    rest->aux = next;
                    link = restOff;
                } else {
                    need = sz;
                }
                if (prev != 0)
                    At(prev)->aux = link;
                else
                    Head()->freeHead = link;
                c->size = need;
                c->aux = size;
                memset((char*)c + kChunkHeader, 0, need - kChunkHeader);
                CountUse(need);
                return (char*)c + kChunkHeader;
            }
            prev = off;
            off = c->aux;
        }
        if (attempt != 0 || g_outOfMemoryHandler == 0)
            OutOfBlock();
        unsigned int owner = g_insideThread;
        g_insideThread = 0;
        g_outOfMemoryHandler();
        g_insideThread = owner;
    }
    return 0;
}

void FreeLocked(void* p)
{
    BlockChunk* c = Lookup(p);
    unsigned int off = (unsigned int)((char*)c - g_base);
    unsigned int sz = c->size;
    Head()->inUse -= sz;
    unsigned int prev = 0;
    unsigned int next = Head()->freeHead;
    while (next != 0 && next < off) {
        prev = next;
        next = At(next)->aux;
    }
    unsigned int link = next;
    if (next != 0 && off + sz == next) {
        sz += At(next)->size & ~kFreeBit;
        link = At(next)->aux;
    }
    c->size = sz | kFreeBit;
    c->aux = link;
    if (prev == 0) {
        Head()->freeHead = off;
        return;
    }
    BlockChunk* before = At(prev);
    unsigned int beforeSize = before->size & ~kFreeBit;
    if (prev + beforeSize == off) {
        before->size = (beforeSize + sz) | kFreeBit;
        before->aux = link;
    } else {
        before->aux = off;
    }
}

void* ReallocLocked(void* p, unsigned int size)
{
    if (p == 0)
        return AllocLocked(size);
    if (size == 0) {
        FreeLocked(p);
        return 0;
    }
    BlockChunk* c = Lookup(p);
    unsigned int off = (unsigned int)((char*)c - g_base);
    unsigned int sz = c->size;
    unsigned int old = c->aux;
    unsigned int need = ChunkSizeFor(size);
    char* payload = (char*)c + kChunkHeader;
    if (need <= sz) {
        if (sz - need >= kMinChunk) {
            BlockChunk* rest = At(off + need);
            rest->size = sz - need;
            rest->aux = 0;
            c->size = need;
            FreeLocked((char*)rest + kChunkHeader);
        }
        if (size > old)
            memset(payload + old, 0, size - old);
        c->aux = size;
        return p;
    }
    unsigned int nextOff = off + sz;
    if (nextOff < Size() && (At(nextOff)->size & kFreeBit) != 0) {
        BlockChunk* n = At(nextOff);
        unsigned int combined = sz + (n->size & ~kFreeBit);
        if (combined >= need) {
            unsigned int prev = 0;
            unsigned int walk = Head()->freeHead;
            while (walk != nextOff) {
                prev = walk;
                walk = At(walk)->aux;
            }
            unsigned int link = n->aux;
            unsigned int total = combined;
            if (combined - need >= kMinChunk) {
                BlockChunk* rest = At(off + need);
                rest->size = (combined - need) | kFreeBit;
                rest->aux = n->aux;
                link = off + need;
                total = need;
            }
            if (prev != 0)
                At(prev)->aux = link;
            else
                Head()->freeHead = link;
            c->size = total;
            CountUse(total - sz);
            memset(payload + old, 0, total - kChunkHeader - old);
            c->aux = size;
            return p;
        }
    }
    void* moved = AllocLocked(size);
    memcpy(moved, p, old);
    FreeLocked(p);
    return moved;
}

}

ReplayGate& ReplayGateRef()
{
    static ReplayGate gate;
    return gate;
}

__declspec(noinline) void* __cdecl SimAlloc(unsigned int size)
{
    ReplayImport call(RI_SIM_ALLOC);
    call.Int((int)size);
    if (call.Answered())
        return call.AnsweredPointer();
    AllocSite site(_ReturnAddress(), 0);
    Guard guard;
    void* p = AllocLocked(size);
    if (g_allocHooks != 0)
        g_allocHooks->added(p, size);
    return call.Result(p);
}

__declspec(noinline) void* __cdecl SimRealloc(void* p, unsigned int size)
{
    AllocSite site(_ReturnAddress(), 0);
    Guard guard;
    void* q = ReallocLocked(p, size);
    if (g_allocHooks != 0) {
        if (p != 0)
            g_allocHooks->removed(p);
        if (q != 0)
            g_allocHooks->added(q, size);
    }
    return q;
}

void __cdecl SimFree(void* p)
{
    if (p == 0)
        return;
    ReplayImport call(RI_SIM_FREE);
    call.Ptr(p);
    if (call.Answered())
        return;
    Guard guard;
    if (g_allocHooks != 0)
        g_allocHooks->removed(p);
    FreeLocked(p);
}

void __cdecl SimBlockForEach(void(__cdecl* fn)(void* payload, unsigned int size, void* ctx), void* ctx)
{
    Guard guard;
    unsigned int off = kFirstChunk;
    while (off + kMinChunk <= Size()) {
        BlockChunk* c = At(off);
        unsigned int sz = c->size & ~kFreeBit;
        if (sz < kMinChunk || (sz & 7) != 0 || sz > Size() - off)
            break;
        if ((c->size & kFreeBit) == 0 && c->aux <= sz - kChunkHeader)
            fn((char*)c + kChunkHeader, c->aux, ctx);
        off += sz;
    }
}

bool __cdecl SimOwns(const void* p)
{
    return g_base != 0 && (unsigned int)((const char*)p - g_base) < Size();
}

unsigned int __cdecl SimHighWater()
{
    Guard guard;
    return Head()->highWater;
}

void __cdecl SimBlockInit()
{
    Guard guard;
}

void* __cdecl SimBlockNew(unsigned int size)
{
    void* p = host_mem_reserve(kSimBlockCeilingBytes);
    if (p != nullptr && host_mem_commit(p, size) == nullptr) {
        host_mem_release(p);
        return nullptr;
    }
    return p;
}

unsigned int __cdecl SimBlockSize()
{
    Guard guard;
    return Size();
}

void __cdecl SimBlockGrow(unsigned int bytes)
{
    Guard guard;
    bytes = (bytes + kSimBlockPage - 1) & ~(kSimBlockPage - 1);
    unsigned int old = Size();
    if (bytes <= old)
        return;
    if (bytes > kSimBlockCeilingBytes)
        Fatal("the block would pass its ceiling");
    if (g_growable && host_mem_commit(g_base + old, bytes - old) == nullptr)
        OutOfBlock();
    unsigned int last = 0;
    for (unsigned int off = Head()->freeHead; off != 0; off = At(off)->aux)
        last = off;
    unsigned int added = bytes - old;
    if (last != 0 && last + (At(last)->size & ~kFreeBit) == old) {
        At(last)->size += added;
    } else {
        BlockChunk* tail = At(old);
        tail->size = added | kFreeBit;
        tail->aux = 0;
        if (last != 0)
            At(last)->aux = old;
        else
            Head()->freeHead = old;
    }
    Head()->size = bytes;
}

unsigned int __cdecl SimSearchSteps()
{
    Guard guard;
    return Head()->searchSteps;
}

void __cdecl SimSetSearchSteps(unsigned int steps)
{
    Guard guard;
    Head()->searchSteps = steps;
}

unsigned int* __cdecl SimRandStream(int stream)
{
    if (g_base == 0) {
        Guard guard;
    }
    return &Head()->streams[stream].holdrand;
}

void* __cdecl SimBlockBase()
{
    return g_base;
}

void __cdecl SimBlockAttach(void* base, bool growable)
{
    g_growable = growable;
    for (unsigned int i = 0; i < g_trackedCount; i++) {
        unsigned int off = (unsigned int)((char*)*g_tracked[i] - g_base);
        if (g_base != 0 && *g_tracked[i] != 0 && off < Size())
            *g_tracked[i] = (char*)base + off;
    }
    g_base = (char*)base;
}

void __cdecl SimBlockReset()
{
    for (unsigned int i = 0; i < g_trackedCount; i++)
        *g_tracked[i] = 0;
    if (g_base != 0)
        host_mem_release(g_base);
    g_base = 0;
}

void __cdecl SimBlockTrack(void** slot)
{
    for (unsigned int i = 0; i < g_trackedCount; i++)
        if (g_tracked[i] == slot)
            return;
    if (g_trackedCount == sizeof(g_tracked) / sizeof(g_tracked[0]))
        Fatal("too many tracked pointers");
    g_tracked[g_trackedCount++] = slot;
}

__declspec(noinline) void* __cdecl operator new(size_t size, const SimHeap&)
{
    AllocSite site(_ReturnAddress(), 0);
    return SimAlloc(size);
}

__declspec(noinline) void* __cdecl operator new[](size_t size, const SimHeap&)
{
    AllocSite site(_ReturnAddress(), 0);
    return SimAlloc(size);
}

void __cdecl operator delete(void* p, const SimHeap&)
{
    SimFree(p);
}

void __cdecl operator delete[](void* p, const SimHeap&)
{
    SimFree(p);
}

void* __cdecl SimAllocRegion(const char* tag, unsigned int size)
{
    return SimAllocTagged(tag, size);
}
