#ifndef FEATURE_SPOT_H
#define FEATURE_SPOT_H

#include "../graphics/handle.h"
#include "../util/angles.h"
#include "../util/vec3.h"

#pragma pack(push, 1)

struct FeatureSpot {
    short next;
    short prev;
    union {
        struct {
            Handle anim;
            Handle shadow;
        };
        struct {
            void* state;
            Vec3 pos;
            Vec3 vel;
        };
    };
    Angles16 rot;
    char unknown_26[0x2f - 0x26];
    unsigned char spotFlags;
};

#pragma pack(pop)

#endif
