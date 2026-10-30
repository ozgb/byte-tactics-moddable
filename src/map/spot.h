#ifndef SPOT_H
#define SPOT_H

#include "../../interfaces/ref.h"
#include "../graphics/handle.h"
#include "../util/rot16.h"
#include "../util/vec3.h"

struct SpotState;

#pragma pack(push, 1)

struct Spot {
    short prev;
    short next;
    union {
        struct {
            Handle anim;
            Handle shadow;
        };
        struct {
            Ref<SpotState> state;
            Vec3 pos;
            Vec3 vel;
        };
        struct {
            unsigned char frame;
            char unknown_5[3];
            union {
                struct {
                    Vec3 pos2;
                };
                struct {
                    int unknown_8;
                    int animIndex;
                };
            };
        };
    };
    Rot16 rot;
    unsigned short damage;
    union {
        struct {
            short x;
            short z;
        };
        Point16 cell;
    };
    unsigned short feature;
    union {
        unsigned char burnTime;
        unsigned char timer;
        unsigned char animBits;
        struct {
            unsigned char animLo : 4;
            unsigned char animHi : 4;
        };
    };
    union {
        unsigned char flags;
        struct {
            unsigned char used : 1;
            unsigned char reclaimed : 1;
            unsigned char hasShadow : 1;
            unsigned char noSend : 1;
            unsigned char bit4 : 1;
            unsigned char bits5 : 3;
        };
    };
};

#pragma pack(pop)

#endif
