#ifndef TIMED_SUB_PARTICLES_H
#define TIMED_SUB_PARTICLES_H

#include <vector>

#include "particle_system.h"

struct TimedSubParticle;

#include "../util/vec3.h"

class TimedSubParticles : public ParticleSystem {
public:
    int time;
    std::vector<TimedSubParticle> records;
    int emitPeriod;
    int holdPeriod;
    int maxFrame;
    Vec3 pos;

    TimedSubParticles();
    virtual void Update();
    virtual void Render(int);
    virtual int IsFinished();
    virtual void Emit();
    virtual int IsEmitDue();
    virtual void Init(Vec3* pos, int a, int b, int c);
};

#endif
