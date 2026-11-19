#include <vector>

#include "map_cache_entry.h"

typedef std::vector<MapCacheEntry> MapCacheEntryVectorInsert;
typedef void (MapCacheEntryVectorInsert::*MapCacheEntryVectorInsertFn)(
    MapCacheEntryVectorInsert::iterator, MapCacheEntryVectorInsert::size_type, const MapCacheEntry&);

// FUNCTION: 0x437580 ?insert@?$vector@VMapCacheEntry@@V?$allocator@VMapCacheEntry@@@std@@@std@@QAEXPAVMapCacheEntry@@IABV3@@Z
MapCacheEntryVectorInsertFn g_insert_00437580 = &MapCacheEntryVectorInsert::insert;

#pragma auto_inline(off)
// FUNCTION: 0x437800
MapCacheEntry& MapCacheEntry::operator=(const MapCacheEntry& other)
{
    ((StringRef*)this)->Assign(&other.handle);
    tntChecksum = other.tntChecksum;
    return *this;
}

// FUNCTION: 0x437820
MapCacheEntry::MapCacheEntry(const MapCacheEntry& other)
    : handle(other.handle), tntChecksum(other.tntChecksum)
{
}
#pragma auto_inline()
