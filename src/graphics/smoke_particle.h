#ifndef SMOKE_PARTICLE_H
#define SMOKE_PARTICLE_H

#include "../util/vec3.h"

struct SmokeParticle {
    void* data;
    union {
        Vec3 pos;
        Vec3Words posw;
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
