#include "../../interfaces/host.h"
#include <vector>
#include "free_block_map.h"

#include "../../link/globals.h"
extern void (*g_outOfMemoryHandler)();

class BlockInfo {
public:
    unsigned int base;
    unsigned int size;
    unsigned int count;
    char unknown_c[0x20];
    unsigned int tag;

    BlockInfo(int a, int b, int c, int d, const char* e);
};

struct LiveNode {
    LiveNode* left;
    LiveNode* parent;
    LiveNode* right;
    BlockInfo value;
};

class BlockMapIter {
public:
    LiveNode* ptr;

    BlockMapIter() {}
    BlockMapIter(LiveNode* q) : ptr(q) {}
    bool operator==(const BlockMapIter& o) const { return ptr == o.ptr; }
    BlockInfo& operator*() const { return ptr->value; }
    BlockInfo* operator->() const { return &ptr->value; }
};

class BlockMap {
public:
    char unknown_0[4];
    LiveNode* head;

    BlockMapIter end() { return BlockMapIter(head); }
    BlockMapIter Find(const unsigned int& key);
    BlockMapIter erase(BlockMapIter it);
    BlockMapIter Remove(BlockMapIter it) { return erase(it); }
};

struct Elem_004dd8c0 {
    unsigned int w[0xc];
};

class Alloc_004dd8c0 {
public:
    typedef unsigned int size_type;
    typedef int difference_type;
    typedef Elem_004dd8c0* pointer;
    typedef const Elem_004dd8c0* const_pointer;
    typedef Elem_004dd8c0& reference;
    typedef const Elem_004dd8c0& const_reference;
    typedef Elem_004dd8c0 value_type;

    pointer allocate(size_type _N, const void* = 0)
    {
        pointer _P;
        do {
            _P = (pointer)host_mem_alloc(_N * sizeof(value_type));
            if (_P == 0 && g_outOfMemoryHandler != 0)
                g_outOfMemoryHandler();
        } while (_P == 0 && g_outOfMemoryHandler != 0);
        return _P;
    }
    void deallocate(pointer _P, size_type)
    {
        if (_P != 0)
            host_mem_free(_P);
    }
    void construct(pointer _P, const value_type& _V)
    {
        std::_Construct(_P, _V);
    }
    void destroy(pointer) {}
    size_type max_size() const { return (size_type)(-1) / sizeof(value_type); }
};

struct Arena_004da9f0 {
    std::vector<Elem_004dd8c0, Alloc_004dd8c0> ring;
    unsigned int count;
};

inline void FreeBlockMap::AddFreeBlock(Pair_004db000 p)
{
    FreeBlockIter it;
    FreeBlockIter n = UpperBound(p.offset);
    it = n;
    if (it == begin())
        it = end();
    else
        it.Previous(0);
    if (n != end()) {
        if (n->offset == p.offset + p.length) {
            p.length = p.length + n->length;
            EraseCopyIter(n);
        }
    }
    if (it != end()) {
        if (it->length + it->offset == p.offset) {
            p.length = p.length + it->length;
            p.offset = it->offset;
            EraseCopyIter(it);
        }
    }
    InsertOrFind(p);
}

#include "debug_lib.h"
BlockMap* GetBlockMap();
Arena_004da9f0* GetFreedBlockRing();
#include "debug_lib_4db610.h"
#include "debug_lib_4d8310.h"

// FUNCTION: 0x4db7d0
void __cdecl FreeDebugBlock(void* p, int flags)
{
    if (p == 0)
        return;
    void* lock = GetAllocLock();
    host_mutex_lock(lock);
    BlockMap* live = GetBlockMap();
    BlockInfo rec((unsigned int)p, 0, 0, 0, 0);
    BlockMapIter it = live->Find(rec.base);
    if (it == GetBlockMap()->end()) {
        host_mutex_unlock(lock);
        return;
    }
    unsigned int size = it->size;
    unsigned int pad = (0 - (size & 0xfff)) & 0xfff;
    if (IsBackAlign())
        CheckFillPattern((unsigned int *)((char*)p - pad), GetDebugFillPattern(), pad);
    else
        CheckFillPattern((unsigned int *)((char*)p + size), GetDebugFillPattern(), pad);
    Elem_004dd8c0* old = (Elem_004dd8c0*)&*it;
    Arena_004da9f0* arena = GetFreedBlockRing();
    if (arena->count < 0x2000)
        arena->ring.push_back(*old);
    else
        arena->ring[arena->count & 0x1fff] = *old;
    arena->count++;
    GetBlockMap()->Remove(it);
    CountFree(size);
    g_committedBytes -= (size + 0xfff) & 0xfffff000;
    p = (void*)((unsigned int)p & 0xfffff000);
    if (size == 0)
        size = 1;
    unsigned int commit = RoundUpToPage(size);
    host_mem_decommit(p, commit);
    unsigned int reserve = RoundUpToDoublePage(size);
    FreeBlockMap* blocks = GetFreeBlockSet();
    blocks->AddFreeBlock(Pair_004db000((unsigned int)p, reserve));
    host_mutex_unlock(lock);
}
