#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/features.h"
#include "../util/vec3.h"
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
    g_seams.map->RefreshAllPassMaps(at, footprint);
}
