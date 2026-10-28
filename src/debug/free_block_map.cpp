#include "../../interfaces/host.h"
#include <string>
#include <yvals.h>
#include <map>
#include <list>
#include <time.h>
#include <malloc.h>

#include "free_block_map.h"
#include "../../link/globals.h"

// FUNCTION: 0x4db1c0
unsigned int FreeBlockMap::TakeFreeBlock(unsigned int bytes)
{
    FreeBlockRange k;
    if (size() > 0) {
        k.offset = g_lastAllocOffset;
        k.length = 0;
        FreeBlockIter lb = upper_bound(k);
        if (lb != begin()) {
            FreeBlockIter it = lb;
            it--;
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
                unsigned int len, base;
                base = cur->offset;
                len = cur->length;
                Erase(cur);
                if (g_lastAllocOffset == 0)
                    g_lastAllocOffset = base;
                unsigned int mark;
                if (g_lastAllocOffset >= base && g_lastAllocOffset + bytes <= base + len)
                    mark = g_lastAllocOffset;
                else
                    mark = base;
                if (mark > base)
                    InsertOrFindInline(FreeBlockRange(base, mark - base));
                unsigned int end = mark + bytes;
                if (end < base + len)
                    InsertOrFindInline(FreeBlockRange(end, base - mark + len - bytes));
                g_lastAllocOffset = end;
                return mark;
            }
            cur++;
        } while (tries < 2);
    }
    if (Grow(bytes))
        return TakeFreeBlock(bytes);
    return 0;
}
