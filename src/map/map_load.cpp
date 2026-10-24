// FLAGS: /Gi
#include <vector>

#include "../util/string_ref.h"

class MapCacheEntry {
public:
    StringRef handle;
    int tntChecksum;

    MapCacheEntry(const MapCacheEntry& other);
    MapCacheEntry& operator=(const MapCacheEntry& other);
};

typedef std::vector<MapCacheEntry> Vec_00437580;
typedef void (Vec_00437580::*InsertFn_00437580)(
    Vec_00437580::iterator, Vec_00437580::size_type, const MapCacheEntry&);

// FUNCTION: 0x437580 ?insert@?$vector@VMapCacheEntry@@V?$allocator@VMapCacheEntry@@@std@@@std@@QAEXPAVMapCacheEntry@@IABV3@@Z
InsertFn_00437580 g_insert_00437580 = &Vec_00437580::insert;

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
