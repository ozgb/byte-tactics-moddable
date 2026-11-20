#ifndef MAP_WORLD_H
#define MAP_WORLD_H

#include "../map/unit_visitor.h"

struct Vec3;

void MapInbox(const void* event, unsigned int size);
void MapOnFeaturesPassMaps(const void* event, unsigned int size);

#endif
