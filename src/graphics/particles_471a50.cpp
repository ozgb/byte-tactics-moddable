#include <stddef.h>
#include <string.h>
#include <vector>

#include "object_pool.h"

extern ObjectPool g_particlePool;
extern char g_fxEventPoolBlocked;

class ParticleSystem {
public:
    int deadline;

    ParticleSystem();
    virtual ~ParticleSystem();
    virtual void Update() = 0;
    virtual void Render(int) = 0;
    virtual int IsFinished() = 0;

    static void* __stdcall operator new(size_t size)
    {
        if (g_fxEventPoolBlocked)
            return 0;
        void* p = (void*)g_particlePool.AllocSlot(size);
        if (p)
            memset(p, 0, size);
        return p;
    }

    static void __stdcall operator delete(void* p)
    {
        g_particlePool.FreeSlot((int)p);
    }
};

class TimedSubParticles;

struct Elem_00473500 {
    TimedSubParticles* p;
};

#include "../util/vec3.h"

struct TimedSubParticle {
    int unknown[8];
};

class TimedSubParticles : public ParticleSystem {
public:
    int time;
    std::vector<TimedSubParticle> records;
    char unknown_1c[0x34 - 0x1c];

    TimedSubParticles();
    virtual void Update();
    virtual void Render(int);
    virtual int IsFinished();
    virtual void Emit();
    virtual int IsEmitDue();
    virtual void Init(Vec3* p, int a, int b, int c);
};

class ParticleLists {
public:
    std::vector<Elem_00473500> lists[1];

    void Add(short index, TimedSubParticles* p)
    {
        if (lists[index].size() > 400) {
            delete lists[index][0].p;
            lists[index].erase(lists[index].begin());
        }
        lists[index].push_back(*(Elem_00473500*)&p);
    }

    void AddTimedSubParticles(Vec3* pos, int emitPeriod, int holdPeriod,
                              int lifetime, short index);
};

// FUNCTION: 0x471a50
void ParticleLists::AddTimedSubParticles(Vec3* pos, int emitPeriod,
                                        int holdPeriod, int lifetime, short index)
{
    TimedSubParticles* e = new TimedSubParticles;
    if (e) {
        e->Init(pos, emitPeriod, holdPeriod, lifetime);
        Add(index, e);
    }
}
