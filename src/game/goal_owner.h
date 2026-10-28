#ifndef GOAL_OWNER_H
#define GOAL_OWNER_H

#include "../orders/order_target_types.h"
#include "../../interfaces/ref.h"
#include "../util/angles.h"
#include "../util/vec3.h"

struct UnitDef;
struct Order;
class UnitMotion;

struct GoalMotionFlags {
    char unknown_0[0x2e];
    union {
        unsigned char flags;
        unsigned char occupyFlags;
        struct {
            unsigned char mode : 2;
        };
    };
};

struct GoalCoordTriple {
    Coord_0044f2a0 x;
    Coord_0044f2a0 y;
    Coord_0044f2a0 z;
};

#pragma pack(push, 2)
struct GoalOwnerUnit {
    union {
        Ref<GoalMotionFlags> target;
        Ref<UnitMotion> obj;
    };
    char pad4[0x5c - 0x4];
    Ref<Order> list;
    char pad60[0x64 - 0x60];
    Angles16 angles;
    union {
        Vec3 pos;
        GoalCoordTriple fixedPos;
    };
    char pad76[0x82 - 0x76];
    Ref<unsigned char> spatialBucket;
    char pad86[0x92 - 0x86];
    Ref<UnitDef> def;
};
#pragma pack(pop)

#endif
