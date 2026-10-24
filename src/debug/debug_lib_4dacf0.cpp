#include "../../interfaces/host.h"
#include <set>
#include "free_block_map.h"

extern unsigned int g_lastAllocOffset;
extern unsigned int g_freeBlockWraps;
extern unsigned int g_committedBytes;
extern unsigned int g_committedBytesPeak;
extern unsigned int g_allocSerial;

class BlockInfo {
public:
    unsigned int base;
    unsigned int size;
    unsigned int count;
    char unknown_c[0x20];
    unsigned int tag;

    BlockInfo(int a, int b, int c, int d, const char* e);
};

class BlockMap { public: MapInsertResult Insert(const BlockInfo& v); };

void* GetAllocLock();
unsigned int __cdecl RoundUpToDoublePage(unsigned int size);
unsigned int __cdecl RoundUpToPage(unsigned int size);
FreeBlockMap* GetFreeBlockSet();
BlockMap* GetBlockMap();
void __cdecl CountAlloc(unsigned int size);
char IsBackAlign();
int GetDebugFillPattern();
void __cdecl FillPattern(void* at, int value, unsigned int count);

inline unsigned int FreeBlockMap::TakeFreeBlock(unsigned int bytes)
{
    if (size() > 0) {
        Pair_004db000 k;
        k.offset = g_lastAllocOffset;
        k.length = 0;
        FreeBlockIter lb = lower_bound(k);
        if (lb != begin()) {
            FreeBlockIter it = lb;
            it.Previous(0);
            if (g_lastAllocOffset >= it->offset && g_lastAllocOffset + bytes <= it->offset + it->length)
                lb = it;
        }
        FreeBlockIter cur = lb;
        int tries = 0;
        do {
            if (cur == end()) {
                cur = begin();
                g_lastAllocOffset = 0;
                g_freeBlockWraps++;
                tries++;
            }
            if (cur->length >= bytes) {
                Pair_004db000 b = *cur;
                EraseCopyIter(cur);
                if (g_lastAllocOffset == 0)
                    g_lastAllocOffset = b.offset;
                unsigned int mark;
                if (g_lastAllocOffset >= b.offset && g_lastAllocOffset + bytes <= b.offset + b.length)
                    mark = g_lastAllocOffset;
                else
                    mark = b.offset;
                if (mark > b.offset)
                    InsertOrFind(Pair_004db000(b.offset, mark - b.offset));
                unsigned int end = mark + bytes;
                if (end < b.offset + b.length)
                    InsertOrFind(Pair_004db000(end, b.offset - mark + b.length - bytes));
                g_lastAllocOffset = end;
                return mark;
            }
            cur.Next(0);
        } while (tries < 2);
    }
    if (GrowReservation(bytes))
        return TakeFreeBlock(bytes);
    return 0;
}

// FUNCTION: 0x4dacf0
unsigned int __cdecl AllocDebugBlock(unsigned int n, unsigned int arg2)
{
    void* lock = GetAllocLock();
    host_mutex_lock(lock);
    unsigned int size = n;
    unsigned int res = 0;
    if (size == 0)
        size = 1;
    unsigned int need = RoundUpToPage(size);
    unsigned int want = RoundUpToDoublePage(size);
    if (want < need) {
        host_mutex_unlock(lock);
        return 0;
    }
    unsigned int base = GetFreeBlockSet()->TakeFreeBlock(want);
    if (base != 0) {
        res = (unsigned int)host_mem_commit((void*)base, need);
        if (res == 0) {
            FreeBlockMap* pool = GetFreeBlockSet();
            pool->AddFreeBlock(Pair_004db000(base, want));
        }
    }
    if (res == 0) {
        host_mutex_unlock(lock);
        return 0;
    }
    unsigned int pad = (0 - (n & 0xfff)) & 0xfff;
    if (IsBackAlign()) {
        FillPattern((void*)res, GetDebugFillPattern(), pad);
        res += pad;
    } else {
        FillPattern((void*)(res + n), GetDebugFillPattern(), pad);
    }
    BlockInfo rec(res, n, g_allocSerial, arg2, 0);
    BlockMap* blocks = GetBlockMap();
    blocks->Insert(rec);
    CountAlloc(n);
    g_committedBytes += (n + 0xfff) & 0xfffff000;
    if (g_committedBytes > g_committedBytesPeak)
        g_committedBytesPeak = g_committedBytes;
    host_mutex_unlock(lock);
    return res;
}
