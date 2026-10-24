#include <map>
#include "../../interfaces/host.h"
#include "debug_lib_4db610.h"

class FreeBlockAllocator {
public:
    void* Allocate(unsigned int n);
};

class PoolAlloc_004db610 : public std::allocator<int> {
public:
    char* _Charalloc(size_t n)
    {
        return (char*)((FreeBlockAllocator*)this)->Allocate(n);
    }
};

struct Pair_004db610 {
    unsigned int offset;
    int length;
};

typedef std::map<unsigned int, int, std::less<unsigned int>, PoolAlloc_004db610>
    Map_004db610;

class FreeBlockMap {
public:
    Map_004db610 freeMap;
    int total;

    void AddFreeBlock(Pair_004db610 p);

    void* operator new(size_t n) { return host_mem_alloc(n); }

    FreeBlockMap() : total(0)
    {
        int n = 4;
        do {
            unsigned int len = 0x10000000;
            void* m = host_mem_reserve(len + 0x2000);
            for (;;) {
                if (m != 0 && (unsigned int)m + len <= 0x80000000u)
                    break;
                if (m != 0)
                    host_mem_release(m);
                len = (len >> 1) & 0x7fffe000;
                if (len < 0x10000)
                    goto next;
                m = host_mem_reserve(len + 0x2000);
            }
            total = total + len;
            Pair_004db610 pair;
            pair.offset = (unsigned int)m;
            pair.length = len;
            AddFreeBlock(pair);
        next:
            ;
        } while (--n);
    }
};

#include "../../link/globals.h"

// FUNCTION: 0x4db610
FreeBlockMap* GetFreeBlockSet()
{
    if (g_freeBlockSet == 0)
        g_freeBlockSet = new FreeBlockMap;
    return g_freeBlockSet;
}