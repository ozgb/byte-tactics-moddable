#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <stddef.h>

class ParticleSystem {
public:
    int deadline;

    ParticleSystem();
    virtual ~ParticleSystem();
    virtual void Update() = 0;
    virtual void Render(int) = 0;
    virtual int IsFinished() = 0;
    static void* __stdcall operator new(size_t size);
    static void __stdcall operator delete(void* p);
    void SetLifetime(int ticks);
};

#endif
