#ifndef MAP_CACHE_ENTRY_H
#define MAP_CACHE_ENTRY_H

#include "../../interfaces/services.h"

class Mission;

class MapCacheEntry {
public:
    StringRef handle;
    int tntChecksum;

    MapCacheEntry(const StringRef& other) : handle(other) {}
    MapCacheEntry(const MapCacheEntry& other);
    MapCacheEntry& operator=(const MapCacheEntry& other);
    MapCacheEntry& SetChecksum(Mission* self);
};

#endif
