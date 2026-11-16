#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/features.h"
#include "../util/vec3.h"
#include "../game/map_events.h"
#include "../seams/map_world.h"
#include "map_features_effects.h"

void MapOnFeaturesPassMaps(const void* event)
{
    const FeaturesEvent* effect = static_cast<const FeaturesEvent*>(event);
    Point16 at;
    at.x = (short)effect->x;
    at.y = (short)effect->z;
    Point16 footprint;
    footprint.x = (short)effect->width;
    footprint.y = (short)effect->height;
    MapEvent passMaps = MapRectEventOf(MAP_PASS_MAPS, 0, at, footprint);
    MapInbox(&passMaps);
}
