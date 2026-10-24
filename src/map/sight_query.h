#ifndef SIGHT_QUERY_H
#define SIGHT_QUERY_H

#include "../util/vec3.h"
#include "../../interfaces/ref.h"

struct Player;

struct SightQuery {
    Ref<Player> player;
    Ref<short> cacheCell;
    short sightDistance;
    unsigned char eyeHeight;
    char unknown_b;
    Ref<unsigned char> frameIdx;
    Vec3 pos;
    int unknown_1c;
    int unknown_20;
};

#endif
