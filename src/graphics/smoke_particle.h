#ifndef SMOKE_PARTICLE_H
#define SMOKE_PARTICLE_H

#include "../util/vec3.h"

struct Position_00475470 {
    short xFrac;
    short x;
    short yFrac;
    short y;
    short zFrac;
    short z;
};

struct SmokeParticle {
    void* data;
    union {
        Vec3 pos;
        Position_00475470 posw;
    };
    int limit;
    int count;
    int period;
    int timer;

    void Step();
    void DrawParticle(void* dest, short px, short py);
    int IsExpired(int unused);
};

#endif
