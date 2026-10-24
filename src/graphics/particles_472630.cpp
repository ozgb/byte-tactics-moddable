#include "win_types.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <ctype.h>

#include "object_pool.h"
#include "nano_particle_vector.h"

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
        {
            int* q = (int*)p;
            int n = size >> 2;
            while (n-- > 0)
                *q++ = 0;
            if (size & 3)
            {
                char* c = (char*)p + (size & ~3);
                int m = size & 3;
                while (m-- > 0)
                    *c++ = 0;
            }
        }
        return p;
    }

    static void __stdcall operator delete(void* p);

    void SetLifetime(int ticks);
};

#include "particle_lists.h"

#include "../util/vec3.h"

#include "smoke_particle.h"

struct Vec3_00473560 {
    int x;
    int y;
    int z;
    Vec3_00473560& operator+=(const Vec3_00473560& o)
    {
        x += o.x;
        y += o.y;
        z += o.z;
        return *this;
    }
};

struct Vec3i_00474130 {
    int x;
    int y;
    int z;
    Vec3i_00474130& operator+=(const Vec3i_00474130& o)
    {
        x += o.x;
        y += o.y;
        z += o.z;
        return *this;
    }
};

struct Vec3_00474580 {
    int x, y, z;
    void operator+=(const Vec3_00474580& o) { x += o.x; y += o.y; z += o.z; }
};

#pragma pack(push, 1)
struct Pos_00473590 {
    short x;
    char unknown_2[0x4 - 0x2];
    short h;
    char unknown_6[0x8 - 0x6];
    short y;
};

struct Pos_004739b0 {
    char unknown_0[0x2];
    short x;
    char unknown_4[0x2];
    short height;
    char unknown_8[0x2];
    short y;
};

#include "rect.h"

struct Position_00475150 {
    short xFrac;
    short x;
    short yFrac;
    short y;
    short zFrac;
    short z;
};
#pragma pack(pop)

#include "../network/player.h"

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

extern Game* g_game;

void* __stdcall GetGafFrame(void* a, int b);
void __stdcall DrawFrameBlended(void* dest, void* src, int x, int y);
int __stdcall GetGafFrameCount(void* ptr);
void __stdcall FillRectangle(void* surface, Rect* rect, int color);
int __stdcall GetGroundHeight(void* pos);

#pragma pack(push, 1)
struct Pos_004745e0 {
    short x;
    char unknown_2[2];
    short height;
    char unknown_6[2];
    short y;
};
#pragma pack(pop)

struct TimedSubParticle {
    void* data;
    union {
        Vec3 pos;
        Position_00475150 posw;
    };
    int limit;
    int count;
    int period;
    int timer;

    void Step();
    void DrawParticle(void* dest, short px, short py);
    int IsExpired(int unused);
};

class SmokeParticles : public ParticleSystem {
public:
    int time;
    std::vector<SmokeParticle> records;
    int emitPeriod;
    int holdPeriod;
    int maxFrame;
    int altAnimation;
    Vec3 pos;

    SmokeParticles();
    virtual void Update();
    virtual void Render(int);
    virtual int IsFinished();
    virtual void Emit();
    virtual int IsEmitDue();
    virtual void Init(Vec3* pos, int limit, int a, int b, int c,
                              int alt);
};

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

#pragma pack(push, 1)
class TeleportParticle {
public:
    void* data;
    union {
        struct {
            Vec3_00473560 pos;
            Vec3_00473560 pos2;
            Vec3_00473560 vel;
        };
        struct {
            char unknown_4[0x6 - 0x4];
            Pos_00473590 posw;
        };
    };
    int count;
    int frame;
    int endTime;

    void Step();
    void DrawParticle(void* dest, short px, short py);
    int IsExpired(int currentTick);
};
#pragma pack(pop)

#pragma pack(push, 1)
struct NanoParticle {
public:
    union {
        Vec3 pos;
        Pos_004739b0 posw;
    };
    char unknown_c[0xc];
    Vec3 vel;
    char unknown_24[4];
    int flags;
    int endTime;

    void Step();
    void DrawParticle(int dest, short x, short y);
    int IsExpired(int value);
};
#pragma pack(pop)

#pragma pack(push, 1)
class ThrustParticle {
public:
    void* data;
    union {
        struct {
            Vec3i_00474130 pos;
            Vec3i_00474130 pos1;
            Vec3i_00474130 vel;
        };
        struct {
            char unknown_4[0x6 - 0x4];
            Pos_00473590 posw;
        };
    };
    int mod2_28;
    int frame;
    int val_30;
    int mod_34;
    int endTime;

    void Step();
    void DrawParticle(void* dest, short px, short py);
    int IsExpired(int value);
};
#pragma pack(pop)

#pragma pack(push, 1)
class WakeParticle {
public:
    int data;
    union {
        struct {
            Vec3_00474580 pos;
            Vec3_00474580 pos2;
            Vec3_00474580 vel;
        };
        struct {
            char unknown_4[0x6 - 0x4];
            Pos_004745e0 posw;
        };
    };
    int min;
    int max;
    int value;
    int step;
    int tick;
    int period;
    int endTime;

    void Step();
    void DrawParticle(void* surface, short px, short py);
    int IsExpired(int currentTick);
};
#pragma pack(pop)

struct Shape_00472ab0;

class ThrustParticles : public ParticleSystem {
public:
    int time;
    std::vector<ThrustParticle> items;
    int period;
    Vec3 pos0;
    Vec3 pos1;
    Vec3 pos2;

    ThrustParticles() {}
    virtual void Update();
    virtual void Render(int);
    virtual int IsFinished();
    virtual void Emit();
    virtual int IsEmitDue();
    virtual void Init(Shape_00472ab0* p, Shape_00472ab0* q, int a, int b);
    void Init(Vec3* p, Vec3* q, int a, int b);
};

class Class_00476210 {
public:
    void insert(std::vector<SmokeParticle>::iterator p, unsigned int m,
                 const SmokeParticle& x);
};

class Vec_00476490 {
public:
    void insert(TimedSubParticle* pos, int count, const TimedSubParticle* src);
};

struct Elem_00473500 {
    int unknown_0;
};

typedef std::vector<Elem_00473500> Vec_00473500;
typedef Vec_00473500::size_type (Vec_00473500::*SizeFn_00473500)() const;
typedef Vec_00473500::iterator (Vec_00473500::*UcopyFn_00473500)(
    Vec_00473500::const_iterator, Vec_00473500::const_iterator, Vec_00473500::iterator);
typedef void (Vec_00473500::*UfillFn_00473500)(
    Vec_00473500::iterator, Vec_00473500::size_type, const Elem_00473500&);
typedef void (Vec_00473500::*DestroyFn_00473500)(Vec_00473500::iterator, Vec_00473500::iterator);

struct Access_00473500 : Vec_00473500 {
    static UcopyFn_00473500 ucopy;
    static UfillFn_00473500 ufill;
    static DestroyFn_00473500 destroy;
};

SizeFn_00473500 g_size_00473500 = &Vec_00473500::size;

typedef std::vector<NanoParticle> Vec_004739b0;
typedef void (Vec_004739b0::*ReserveFn_004739b0)(Vec_004739b0::size_type);
typedef Vec_004739b0::iterator (Vec_004739b0::*UcopyFn_004739b0)(
    Vec_004739b0::const_iterator, Vec_004739b0::const_iterator, Vec_004739b0::iterator);
typedef void (Vec_004739b0::*UfillFn_004739b0)(
    Vec_004739b0::iterator, Vec_004739b0::size_type, const NanoParticle&);
typedef void (Vec_004739b0::*DestroyFn_004739b0)(Vec_004739b0::iterator, Vec_004739b0::iterator);

struct Access_00475770 : Vec_004739b0 {
    static ReserveFn_004739b0 fn;
};

struct Access_00475870 : Vec_004739b0 {
    static DestroyFn_004739b0 fn;
};

struct Access_00475880 : Vec_004739b0 {
    static UcopyFn_004739b0 fn;
};

struct Access_00476710 : Vec_004739b0 {
    static UfillFn_004739b0 fn;
};

// FUNCTION: 0x472630
void __stdcall EmitSmoke(Vec3* pos, int emitPeriod, int lifetime, short index)
{
    ParticleLists* owner = (ParticleLists *)g_game->lists_38d77;
    SmokeParticles* e = new SmokeParticles;
    if (e) {
        e->Init((Vec3*)pos, 0, emitPeriod, 0, lifetime, 0);
        owner->Add(index, e);
    }
}

// FUNCTION: 0x472720
void __stdcall EmitTimedBlackSmoke(Vec3* pos, int emitPeriod, int lifetime, short index)
{
    ParticleLists* owner = (ParticleLists *)g_game->lists_38d77;
    SmokeParticles* e = new SmokeParticles;
    if (e) {
        e->Init((Vec3*)pos, 0, emitPeriod, 0, lifetime, 1);
        owner->Add(index, e);
    }
}

// FUNCTION: 0x472810
void __stdcall EmitWhiteSmoke(Vec3* pos, short index)
{
    ParticleLists* owner = (ParticleLists *)g_game->lists_38d77;
    SmokeParticles* e = new SmokeParticles;
    if (e) {
        e->Init((Vec3*)pos, 0, 1, 0, 0, 0);
        owner->Add(index, e);
    }
}

// FUNCTION: 0x4728f0
void __stdcall EmitBlackSmoke(Vec3* p, short index)
{
    ParticleLists& l = *(ParticleLists *)g_game->lists_38d77;
    SmokeParticles* e = new SmokeParticles;
    if (e) {
        e->Init((Vec3*)p, 0, 1, 0, 0, 1);
        l.Add(index, e);
    }
}

// FUNCTION: 0x4729d0
void __stdcall EmitWeaponSmoke(Vec3* p, short index)
{
    ParticleLists& l = *(ParticleLists *)g_game->lists_38d77;
    SmokeParticles* e = new SmokeParticles;
    if (e) {
        e->Init((Vec3*)p, 3, 1, 0x1e, 0, 0);
        l.Add(index, e);
    }
}

// FUNCTION: 0x472c50
void __stdcall EmitTimedSubParticles(Vec3* p, short index)
{
    ParticleLists& l = *(ParticleLists *)g_game->lists_38d77;
    TimedSubParticles* e = new TimedSubParticles;
    if (e) {
        e->Init(p, 5, 0, 0x96);
        l.Add(index, e);
    }
}

#pragma auto_inline(off)
// FUNCTION: 0x474cd0
// FUNCTION: 0x474d10 ??_GSmokeParticles@@UAEPAXI@Z
SmokeParticles::SmokeParticles()
{
    time = g_game->ticks;
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x4750b0
// FUNCTION: 0x475110 ??_GTimedSubParticles@@UAEPAXI@Z
TimedSubParticles::TimedSubParticles()
{
    time = g_game->ticks;
}
#pragma auto_inline(on)
