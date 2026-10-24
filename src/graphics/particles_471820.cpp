#include <stddef.h>
#include <string.h>
#include <vector>

#include "object_pool.h"

extern ObjectPool g_particlePool;
extern char g_fxEventPoolBlocked;

class ParticleSystem {
public:
    int deadline;

    ParticleSystem() { deadline = 0; }
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

class SmokeParticles;

struct Elem_00473500 {
    SmokeParticles* p;
};

#include "smoke_particle.h"

class SmokeParticles : public ParticleSystem {
public:
    int time;
    std::vector<SmokeParticle> records;
    char unknown_1c[0x38 - 0x1c];

    SmokeParticles();
    virtual void Update();
    virtual void Render(int);
    virtual int IsFinished();
    virtual void Emit();
    virtual int IsEmitDue();
    virtual void Init(Vec3* pos, int limit, int a, int b, int c,
                              int alt);
};

class ParticleLists {
public:
    std::vector<Elem_00473500> lists[1];

    void Add(short index, SmokeParticles* p)
    {
        if (lists[index].size() > 400) {
            delete lists[index][0].p;
            lists[index].erase(lists[index].begin());
        }
        lists[index].push_back(*(Elem_00473500*)&p);
    }

    void AddSmoke(Vec3* pos, int limit, int emitPeriod, int holdPeriod,
                      int lifetime, short index, int alt);
};

// FUNCTION: 0x471820
void ParticleLists::AddSmoke(Vec3* pos, int limit, int emitPeriod,
                                  int holdPeriod, int lifetime, short index, int alt)
{
    SmokeParticles* p = new SmokeParticles;
    if (p) {
        p->Init(pos, limit, emitPeriod, holdPeriod, lifetime, alt);
        Add(index, p);
    }
}
