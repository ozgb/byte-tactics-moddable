#include "win_types.h"
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <vector>
#include <cstdlib>
#include <ctype.h>

#include "object_pool.h"

#include "../../interfaces/ref.h"
#include "../debug/debug_lib.h"
#include "../harness/harness.h"
#include "../util/vec3.h"
#include "rect.h"
#include "nano_particle_vector.h"
#include "particle_system.h"
#include "particle_lists.h"
#include "smoke_particle.h"
#include "timed_sub_particles.h"
#include "particles.h"
#include "../network/player.h"

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

#pragma pack(push, 2)
struct Elem_00470a40 {
    int a;
    int b;
    int c;
    short d;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Vec3_004736e0 {
    int x;
    int y;
    int z;
    Vec3_004736e0 operator-(const Vec3_004736e0& o) const
    {
        Vec3_004736e0 r;
        r.x = x - o.x;
        r.y = y - o.y;
        r.z = z - o.z;
        return r;
    }
    int Length() const
    {
        float fx = x;
        float fy = y;
        float fz = z;
        return (int)sqrt(fx * fx + fy * fy + fz * fz);
    }
    Vec3_004736e0& operator+=(const Vec3_004736e0& o)
    {
        x += o.x;
        y += o.y;
        z += o.z;
        return *this;
    }
};

union Fixed16 {
    int whole;
    short half[2];
};

struct Pos_00473590 {
    short x;
    char unknown_2[0x4 - 0x2];
    short h;
    char unknown_6[0x8 - 0x6];
    short y;
};

struct Pos_004745e0 {
    short x;
    char unknown_2[2];
    short height;
    char unknown_6[2];
    short y;

    int Visible();
};

struct TeleportParticle {
    void* data;
    union {
        struct {
            Vec3_004736e0 pos1;
            Vec3_004736e0 pos2;
            Vec3_004736e0 dir;
        };
        struct {
            char unknown_4[0x6 - 0x4];
            Pos_00473590 posw;
        };
    };
    int frameCount;
    int frame;
    int endTime;

    void Step();
    void DrawParticle(void* dest, short px, short py);
    int IsExpired(int currentTick);
};

struct Pos_004739b0 {
    char unknown_0[0x2];
    short x;
    char unknown_4[0x2];
    short height;
    char unknown_8[0x2];
    short y;
};

struct NanoParticle {
public:
    union {
        Vec3 pos;
        Pos_004739b0 posw;
    };
    Vec3 tgt;
    Vec3 vel;
    int unused;
    int flags;
    int endTime;

    void Step();
    void DrawParticle(int dest, short x, short y);
    int IsExpired(int value);
};

struct Vec3_00473b50 {
    int x;
    int y;
    int z;
};

struct Seg_00473b50 {
    Vec3_00473b50 start;
    Vec3_00473b50 end;
};

#define SPLIT_SEG(g)                         \
    {                                        \
        int v, d;                            \
        v = (g).start.x;                     \
        d = (g).end.x - (g).start.x;         \
        (g).start.x = v + d * 4 / 11;        \
        (g).end.x = v + d * 7 / 11;          \
        v = (g).start.y;                     \
        d = (g).end.y - (g).start.y;         \
        (g).start.y = v + d * 4 / 11;        \
        (g).end.y = v + d * 7 / 11;          \
        v = (g).start.z;                     \
        d = (g).end.z - (g).start.z;         \
        (g).start.z = v + d * 4 / 11;        \
        (g).end.z = v + d * 7 / 11;          \
        (g).end.x -= (g).start.x;            \
        (g).end.y -= (g).start.y;            \
        (g).end.z -= (g).start.z;            \
    }

struct ThrustParticle {
    unsigned short* bitmask;
    union {
        struct {
            Vec3 pos0;
            Vec3 pos1;
            Vec3 pos2;
        };
        struct {
            char unknown_4[0x6 - 0x4];
            Pos_00473590 posw;
        };
    };
    int frameCount;
    int frame;
    int tick;
    int period;
    int endTime;

    void Step();
    void DrawParticle(void* dest, short px, short py);
    int IsExpired(int currentTick);
};

struct Pair_00474880 {
    short lo;
    short hi;
};

struct Vec3_00474760 {
    union {
        int x;
        Pair_00474880 xp;
    };
    union {
        int y;
        Pair_00474880 yp;
    };
    union {
        int z;
        Pair_00474880 zp;
    };
    int Length() const
    {
        double fx = x;
        double fy = y;
        double fz = z;
        return (int)sqrt(fx * fx + fy * fy + fz * fz);
    }
    void Scale(int s)
    {
        x = (int)(((__int64)x * s) >> 16);
        y = (int)(((__int64)y * s) >> 16);
        z = (int)(((__int64)z * s) >> 16);
    }
    Vec3_00474760& operator+=(const Vec3_00474760& o)
    {
        x += o.x;
        y += o.y;
        z += o.z;
        return *this;
    }
};

static inline Vec3_00474760 operator-(const Vec3_00474760& p, const Vec3_00474760& q)
{
    Vec3_00474760 r;
    r.x = p.x - q.x;
    r.y = p.y - q.y;
    r.z = p.z - q.z;
    return r;
}

struct WakeParticle {
    void* data;
    union {
        struct {
            Vec3_00474760 pos;
            Vec3_00474760 pos2;
            Vec3_00474760 vel;
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

class TeleportParticles : public ParticleSystem {
public:
    int time;
    std::vector<TeleportParticle> items;
    int sparkLifetime;
    Vec3_004736e0 pos1;
    Vec3_004736e0 pos2;
    Vec3_004736e0 dir;

    TeleportParticles() {}
    virtual void Update();
    virtual void Render(int);
    virtual int IsFinished();
    virtual void Emit();
    virtual int IsEmitDue();
    virtual void Init(Vec3_004736e0* a, Vec3_004736e0* b, int c);
};

class NanoParticles : public ParticleSystem {
public:
    int time;
    std::vector<NanoParticle> items;
    Seg_00473b50 seg_1c;
    Seg_00473b50 seg_34;

    NanoParticles() {}
    virtual void Update();
    virtual void Render(int);
    virtual int IsFinished();
    virtual void Emit();
    virtual int IsEmitDue();
    virtual void Init(Seg_00473b50* a, Seg_00473b50* b, int c);
};

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

class WakeParticles : public ParticleSystem {
public:
    int time;
    std::vector<WakeParticle> items;
    int period;
    Vec3_00474760 pos_a;
    Vec3_00474760 pos_b;
    Vec3_00474760 dir;
    int ascending;

    WakeParticles() {}
    virtual void Update();
    virtual void Render(int);
    virtual int IsFinished();
    virtual void Emit();
    virtual int IsEmitDue();
    virtual void Init(Vec3_00474760* a, Vec3_00474760* b, int newPeriod,
                              int lifetime, int isAscending);
};

class Listener_00471d90 {
public:
    virtual void Slot0();
    virtual void Slot1();
    virtual void Slot2(void* msg);
};

struct Lists_00471d90 {
    std::vector<Listener_00471d90*> lists[10];
};

class Listener_00471eb0 {
public:
    virtual ~Listener_00471eb0();
    virtual void Slot1();
    virtual void Slot2(void* msg);
    virtual int Slot3();
};

struct Lists_00471eb0 {
    std::vector<Listener_00471eb0*> lists[10];
};

class Listener_471f40 {
public:
    virtual void Slot0();
    virtual void Slot1();
    virtual void Slot2(int arg);
};

struct Lists_471f40 {
    std::vector<Listener_471f40*> lists[10];
};

class Listener_00471f90 {
public:
    virtual void Slot0();
    virtual void Slot1();
    virtual void Slot2(void* msg);
};

struct Lists_00471f90 {
    std::vector<Listener_00471f90*> lists[10];
};

struct Lists_00471fd0 {
    std::vector<ParticleSystem*> lists[10];
};

struct Vec3_004720d0 {
    int x, y, z;
};

struct Ctx_004720d0 {
    Vec3_004720d0 a;
    Vec3_004720d0 b;
};

struct Lists_004720d0 {
    std::vector<ParticleSystem*> lists[1];
};

struct Pos_00472200 {
    Vec3 a;
    Vec3 b;
};

class Lists_00472330 {
public:
    std::vector<ParticleSystem*> lists[10];

    void Add(short index, ParticleSystem* p)
    {
        if (lists[index].size() > 400) {
            delete lists[index][0];
            lists[index].erase(lists[index].begin());
        }
        lists[index].push_back(p);
    }
};

class Lists_00472430 {
public:
    std::vector<ParticleSystem*> lists[10];

    void Add(short index, ParticleSystem* p)
    {
        if (lists[index].size() > 400) {
            delete lists[index][0];
            lists[index].erase(lists[index].begin());
        }
        lists[index].push_back(p);
    }
};

struct Lists_00472530 {
    std::vector<ParticleSystem*> lists[10];

    void Add(short index, ParticleSystem* p)
    {
        if (lists[index].size() > 400) {
            delete lists[index][0];
            lists[index].erase(lists[index].begin());
        }
        lists[index].push_back(p);
    }
};

struct Position_00475150 {
    short xFrac;
    short x;
    short yFrac;
    short y;
    short zFrac;
    short z;
};

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

struct MapSize_004745e0 {
    unsigned int width;
    unsigned int height;

    int Contains(int col, int row)
    {
        return (unsigned int)col < width && (unsigned int)row < height;
    }
};

struct ByteMap_004745e0 {
    Ref<unsigned char> data;
    MapSize_004745e0 size;

    unsigned char Get(int tx, int ty) { return data[size.width * ty + tx]; }
};

struct Map_004745e0 {
    char unknown_0[0x7c];
    ByteMap_004745e0 explored;
    char unknown_88[0x14b - 0x88];
};

#pragma pack(pop)

extern Game* g_game;
extern ObjectPool g_particlePool;
extern char g_fxEventPoolBlocked;
extern unsigned char g_particlePoolDestroyed;
void* __stdcall GetGafFrame(void* a, int b);
void __stdcall DrawFrameBlended(void* dest, void* src, int x, int y);
int __stdcall GetGafFrameCount(void* ptr);
void __stdcall FillRectangle(void* surface, Rect* rect, int color);
int __stdcall GetGroundHeight(void* pos);

static inline int InsideExplored(Player* p, int col, int row)
{
    return (unsigned int)col < (unsigned int)p->exploredWidth &&
           (unsigned int)row < (unsigned int)p->exploredHeight;
}

static inline int IsSeen_00473590(Player* p, Player* q, int col, int row)
{
    if (!InsideExplored(p, col, row))
        return 0;
    return (g_game->visibilityMask[q->exploredWidth * row + col] &
            (1 << g_game->playerIndex)) != 0;
}

static inline int Identity_00473590(int v) { return v; }

static inline int Identity_00473a00(int v) { return v; }

static inline int IsSeen_00473a00(Player* p, Player* q, int col, int row)
{
    if (!InsideExplored(p, col, row))
        return 0;
    return (g_game->visibilityMask[q->exploredWidth * row + col] &
            (1 << g_game->playerIndex)) != 0;
}

static inline int Identity_00474170(int v) { return v; }

static inline int IsSeen_00474170(Player* p, Player* q, int col, int row)
{
    if (!InsideExplored(p, col, row))
        return 0;
    return (g_game->visibilityMask[q->exploredWidth * row + col] &
            (1 << g_game->playerIndex)) != 0;
}

static inline int Identity_004745e0(int v) { return v; }

static inline int IsSeen_004745e0(Map_004745e0* p, Map_004745e0* q, int col, int row)
{
    if (!p->explored.size.Contains(col, row))
        return 0;
    return (g_game->visibilityMask[q->explored.size.width * row + col] &
            (1 << g_game->playerIndex)) != 0;
}

static inline int Identity_00474b80(int v) { return v; }

static inline int IsSeen_00474b80(Player* p, Player* q, int col, int row)
{
    if (!InsideExplored(p, col, row))
        return 0;
    return (g_game->visibilityMask[q->exploredWidth * row + col] &
            (1 << g_game->playerIndex)) != 0;
}

inline int Pos_004745e0::Visible()
{
    Map_004745e0* p = &(*(Map_004745e0 (*)[10])&g_game->players)[g_game->playerIndex];
    Map_004745e0* p2 = &(*(Map_004745e0 (*)[10])&g_game->players)[g_game->playerIndex];
    int visible;
    if ((g_game->mapFlags & 2) == 2) {
        int col = x >> 5;
        int row = (y - (height >> 1)) >> 5;
        if (p->explored.size.Contains(col, row) &&
            p->explored.Get(col, row) != 0)
            visible = 1;
        else
            visible = 0;
    } else {
        int col = x >> 5;
        int row = (y - (height >> 1)) >> 5;
        visible = Identity_004745e0(IsSeen_004745e0(p, p2, col, row));
    }
    return visible;
}

// FUNCTION: 0x470a40
Elem_00470a40* __stdcall CopyStructRange(Elem_00470a40* first, Elem_00470a40* last, Elem_00470a40* dest)
{
    for (; first != last; ++dest, ++first)
        *dest = *first;
    return dest;
}

#pragma auto_inline(off)
// FUNCTION: 0x470a90
ObjectPool* ObjectPool::Construct(int initialCapacity, int initialSlotSize)
{
    this->ObjectPool::ObjectPool(initialCapacity, initialSlotSize);
    return (ObjectPool*)this;
}
#pragma auto_inline(on)

// FUNCTION: 0x470ae0 ??_GObjectPool@@UAEPAXI@Z
static ObjectPool s_obj(1000, 0x4c);

#pragma auto_inline(off)
// FUNCTION: 0x470b80
void ObjectPool::Destroy()
{
    this->ObjectPool::~ObjectPool();
}
#pragma auto_inline(on)

// FUNCTION: 0x470e50
void ObjectPool::FreeBlocks()
{
    if (slots != 0) {
        GameFreeThunk(slots);
    }
    std::vector<PoolBlock*>::iterator it = items.begin();
    while (it != items.end()) {
        GameFreeThunk(*it);
        items.erase(it);
    }
}

#pragma auto_inline(off)
// FUNCTION: 0x470eb0
int ObjectPool::AllocSlot(int unused)
{
    int edx = used;
    int esi = capacity;
    int eax = 0;

    if (edx < esi) {
        void* ptr = slots;
        edx++;
        eax = *(int*)((char*)ptr + edx * 4 - 4);
        used = edx;
    }

    return eax;
}

// FUNCTION: 0x470ed0
void ObjectPool::FreeSlot(int slot)
{
    int eax = used - 1;
    used = eax;
    ((int*)slots)[eax] = slot;
}
#pragma auto_inline(on)

// FUNCTION: 0x470f60
void __stdcall CopyPointer(int* dest, int* src)
{
    if (dest != 0) {
        *dest = *src;
    }
}

// FUNCTION: 0x470f80
ParticleLists::ParticleLists()
{
}

// FUNCTION: 0x470fb0
ParticleLists::~ParticleLists()
{
    for (int i = 0; i < 10; i++) {
        std::vector<ParticleSystem*>::iterator it = lists[i].begin();
        while (it != lists[i].end()) {
            delete *it;
            lists[i].erase(it);
        }
    }
}

// FUNCTION: 0x471050
void ParticleLists::UpdateAll()
{
    for (int i = 0; i < 10; i++) {
        std::vector<ParticleSystem*>::iterator it = lists[i].begin();
        while (it != lists[i].end()) {
            ParticleSystem* l = *it;
            if (l->IsFinished()) {
                delete l;
                lists[i].erase(it);
            } else {
                l->Update();
                it++;
            }
        }
    }
}

// FUNCTION: 0x4710e0
void ParticleLists::DrawAll(void* param)
{
    for (int i = 0; i < 10; i++) {
        for (std::vector<ParticleSystem*>::iterator it = lists[i].begin(); it != lists[i].end(); it++)
            (*it)->Render((int)param);
    }
}

// FUNCTION: 0x471120
void ParticleLists::DrawList(void* param, short index)
{
    std::vector<ParticleSystem*>* s = &lists[index];
    for (std::vector<ParticleSystem*>::iterator p = s->begin(); p != s->end(); p++)
        (*p)->Render((int)param);
}

// FUNCTION: 0x471340
void ParticleLists::AddTeleportParticles(int posA, int posB, int lifetime, short index)
{
    TeleportParticles* p = new TeleportParticles;
    if (p) {
        p->Init((Vec3_004736e0*)posA, (Vec3_004736e0*)posB, lifetime);
        Add(index, p);
    }
}

// FUNCTION: 0x471430 ??_GTeleportParticles@@UAEPAXI@Z
static TeleportParticles* s_teleport = new TeleportParticles;

// FUNCTION: 0x471470
void ParticleLists::AddNanoParticles(int posA, int posB, int lifetime, short index)
{
    NanoParticles* p = new NanoParticles;
    if (p) {
        p->Init((Seg_00473b50*)posA, (Seg_00473b50*)posB, lifetime);
        Add(index, p);
    }
}

// FUNCTION: 0x471560 ??_GNanoParticles@@UAEPAXI@Z
static NanoParticles* s_nano = new NanoParticles;

// FUNCTION: 0x4715a0
void ParticleLists::AddThrustParticles(int posA, int posB, int period, int lifetime, short index)
{
    ThrustParticles* p = new ThrustParticles;
    if (p) {
        p->Init((Shape_00472ab0*)posA, (Shape_00472ab0*)posB, period, lifetime);
        Add(index, p);
    }
}

// FUNCTION: 0x4716a0 ??_GThrustParticles@@UAEPAXI@Z
static ThrustParticles* s_thrust = new ThrustParticles;

// FUNCTION: 0x4716e0
void ParticleLists::AddWakeParticles(int posA, int posB, int period,
                                  int lifetime, short index, int isAscending)
{
    WakeParticles* p = new WakeParticles;
    if (p) {
        p->Init((Vec3_00474760*)posA, (Vec3_00474760*)posB, period, lifetime, isAscending);
        Add(index, p);
    }
}

// FUNCTION: 0x4717e0 ??_GWakeParticles@@UAEPAXI@Z
static WakeParticles* s_wake = new WakeParticles;

// FUNCTION: 0x471c80
void InitParticlePool()
{
    g_particlePool.Construct(0x3e8, 0x4c);
    atexit((void (__cdecl*)(void))ExitParticlePool);
}

// FUNCTION: 0x471ca0
void ExitParticlePool()
{
    if ((g_particlePoolDestroyed & 1) == 0) {
        g_particlePoolDestroyed |= 1;
        g_particlePool.Destroy();
    }
}

// FUNCTION: 0x471cc0
// FUNCTION: 0x471cd0 ??_GParticleSystem@@UAEPAXI@Z
ParticleSystem::ParticleSystem()
{
    deadline = 0;
}

// FUNCTION: 0x471d00
ParticleSystem::~ParticleSystem()
{
}

// FUNCTION: 0x471d10
void* __stdcall ParticleSystem::operator new(size_t size)
{
    if (g_fxEventPoolBlocked)
        return 0;
    void* p = (void*)g_particlePool.AllocSlot(size);
    if (p)
        memset(p, 0, size);
    return p;
}

// FUNCTION: 0x471d50
void __stdcall ParticleSystem::operator delete(void* p)
{
    g_particlePool.FreeSlot((int)p);
}

#pragma auto_inline(off)
// FUNCTION: 0x471d70
void ParticleSystem::SetLifetime(int ticks)
{
    deadline = g_game->ticks + ticks;
}
#pragma auto_inline(on)

// FUNCTION: 0x471d90
void CreateParticleLists()
{
    (*(Lists_00471d90 **)&g_game->lists_38d77) = new Lists_00471d90;
}

// FUNCTION: 0x471eb0
void UpdateParticles()
{
    Lists_00471eb0* l = (Lists_00471eb0 *)g_game->lists_38d77;
    for (int i = 0; i < 10; i++) {
        std::vector<Listener_00471eb0*>& v = l->lists[i];
        std::vector<Listener_00471eb0*>::iterator it = v.begin();
        while (it != v.end()) {
            Listener_00471eb0* p = *it;
            if (p->Slot3()) {
                delete p;
                v.erase(it);
            } else {
                p->Slot1();
                ++it;
            }
        }
    }
}

// FUNCTION: 0x471f40
void __stdcall DrawParticles(int arg)
{
    Lists_471f40* l = (Lists_471f40 *)g_game->lists_38d77;
    for (int i = 0; i < 10; i++) {
        for (std::vector<Listener_471f40*>::iterator it = l->lists[i].begin(); it != l->lists[i].end(); ++it) {
            (*it)->Slot2(arg);
        }
    }
}

// FUNCTION: 0x471f90
void __stdcall DrawParticleList(void* msg, short kind)
{
    std::vector<Listener_00471f90*>& v = ((Lists_00471f90 *)g_game->lists_38d77)->lists[kind];
    for (std::vector<Listener_00471f90*>::iterator it = v.begin(); it != v.end(); ++it) {
        (*it)->Slot2(msg);
    }
}

static void __stdcall Add(Lists_00471fd0* l, short index, ParticleSystem* p)
{
    if (l->lists[index].size() > 400) {
        delete l->lists[index][0];
        l->lists[index].erase(l->lists[index].begin());
    }
    l->lists[index].push_back(p);
}

// FUNCTION: 0x471fd0
void __stdcall EmitTeleportParticles(int posA, int posB, int lifetime, short index)
{
    Lists_00471fd0* l = (Lists_00471fd0 *)g_game->lists_38d77;
    TeleportParticles* p = new TeleportParticles;
    if (p) {
        p->Init((Vec3_004736e0*)posA, (Vec3_004736e0*)posB, lifetime);
        Add(l, index, p);
    }
}

static void Add_004720d0(Lists_004720d0* lists, short index, ParticleSystem* p)
{
    if (lists->lists[index].size() > 400) {
        delete lists->lists[index][0];
        lists->lists[index].erase(lists->lists[index].begin());
    }
    lists->lists[index].push_back(p);
}

// FUNCTION: 0x4720d0
void __stdcall EmitNanoParticles(Vec3_004720d0* p, void* posB, short index)
{
    Ctx_004720d0 ctx;
    ctx.a = *p;
    ctx.b = *p;
    Lists_004720d0* lists = (Lists_004720d0 *)g_game->lists_38d77;
    NanoParticles* q = new NanoParticles;
    if (q) {
        q->Init((Seg_00473b50*)&ctx.a, (Seg_00473b50*)posB, 1);
        Add_004720d0(lists, index, q);
    }
}

// FUNCTION: 0x472200
void __stdcall EmitReverseNanoParticles(Pos_00472200* targetBounds, Vec3* origin, short index)
{
    Pos_00472200 pos;
    pos.a = *origin;
    pos.b = *origin;
    ParticleLists* lists = (ParticleLists *)g_game->lists_38d77;
    NanoParticles* p = new NanoParticles;
    if (p) {
        p->Init((Seg_00473b50*)targetBounds, (Seg_00473b50*)&pos, 1);
        lists->Add(index, p);
    }
}

// FUNCTION: 0x472330
void __stdcall EmitThrustParticles(int posA, int posB, int period, int lifetime,
                            short index)
{
    Lists_00472330* lists = (Lists_00472330 *)g_game->lists_38d77;
    ThrustParticles* p = new ThrustParticles;
    if (p) {
        p->Init((Shape_00472ab0*)posA, (Shape_00472ab0*)posB, period, lifetime);
        lists->Add(index, p);
    }
}

// FUNCTION: 0x472430
void __stdcall EmitWakeParticles(int posA, int posB, int period, short index)
{
    Lists_00472430* l = (Lists_00472430 *)g_game->lists_38d77;
    WakeParticles* p = new WakeParticles;
    if (p) {
        p->Init((Vec3_00474760*)posA, (Vec3_00474760*)posB, period, 1, 1);
        l->Add(index, p);
    }
}

// FUNCTION: 0x472530
void __stdcall EmitBubbles(int posA, int posB, int period, short index)
{
    Lists_00472530* l = (Lists_00472530 *)g_game->lists_38d77;
    WakeParticles* p = new WakeParticles;
    if (p) {
        p->Init((Vec3_00474760*)posA, (Vec3_00474760*)posB, period, 1, 0);
        l->Add(index, p);
    }
}

// FUNCTION: 0x472d50
void TeleportParticles::Update()
{
    std::vector<TeleportParticle>::iterator it = items.begin();

    while (it != items.end()) {
        it->Step();
        if (it->IsExpired(g_game->ticks)) {
            items.erase(it);
        } else {
            ++it;
        }
    }
    if (IsEmitDue()) {
        Emit();
    }
}

// FUNCTION: 0x472e00
int TeleportParticles::IsEmitDue()
{
    if (time <= deadline) {
        unsigned int game_val = g_game->ticks;
        if ((unsigned int)time <= game_val) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: 0x472e30
void TeleportParticles::Render(int p)
{
    for (std::vector<TeleportParticle>::iterator it = items.begin(); it != items.end(); ++it) {
        it->DrawParticle((void*)p, g_game->scrollX, g_game->scrollY);
    }
}

// FUNCTION: 0x472e70
int TeleportParticles::IsFinished()
{
    return items.empty();
}

// FUNCTION: 0x472eb0
void NanoParticles::Update()
{
    std::vector<NanoParticle>::iterator it = items.begin();
    while (it != items.end()) {
        it->Step();
        if (it->IsExpired(g_game->ticks))
            it = items.erase(it);
        else
            ++it;
    }
    if (IsEmitDue())
        Emit();
}

// FUNCTION: 0x472f60
int NanoParticles::IsEmitDue()
{
    if (time <= deadline) {
        unsigned int game_val = g_game->ticks;
        if ((unsigned int)time <= game_val) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: 0x472f90
void NanoParticles::Render(int dest)
{
    for (std::vector<NanoParticle>::iterator it = items.begin(); it != items.end(); ++it)
        it->DrawParticle(dest, g_game->scrollX, g_game->scrollY);
}

// FUNCTION: 0x472fd0
int NanoParticles::IsFinished()
{
    return items.empty();
}

// FUNCTION: 0x473010
void ThrustParticles::Update()
{
    for (std::vector<ThrustParticle>::iterator it = items.begin(); it != items.end(); ) {
        it->Step();
        if (it->IsExpired(g_game->ticks))
            items.erase(it);
        else
            ++it;
    }
    if (IsEmitDue())
        Emit();
}

// FUNCTION: 0x4730c0
int ThrustParticles::IsEmitDue()
{
    if (time <= deadline) {
        unsigned int val = g_game->ticks;
        if ((unsigned int)time <= val) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: 0x4730f0
void ThrustParticles::Render(int dest)
{
    for (std::vector<ThrustParticle>::iterator it = items.begin(); it != items.end(); ++it) {
        it->DrawParticle((void*)dest, g_game->scrollX, g_game->scrollY);
    }
}

// FUNCTION: 0x473130
int ThrustParticles::IsFinished()
{
    return items.empty();
}

// FUNCTION: 0x473170
void WakeParticles::Update()
{
    std::vector<WakeParticle>::iterator it = items.begin();

    while (it != items.end()) {
        it->Step();
        if (it->IsExpired(g_game->ticks)) {
            items.erase(it);
        } else {
            ++it;
        }
    }
    if (IsEmitDue()) {
        Emit();
    }
}

// FUNCTION: 0x473220
int WakeParticles::IsEmitDue()
{
    if (time <= deadline && time <= (unsigned int)g_game->ticks) {
        return 1;
    }
    return 0;
}

// FUNCTION: 0x473250
void WakeParticles::Render(int dest)
{
    for (std::vector<WakeParticle>::iterator it = items.begin(); it != items.end(); ++it) {
        it->DrawParticle((void*)dest, g_game->scrollX, g_game->scrollY);
    }
}

// FUNCTION: 0x473290
int WakeParticles::IsFinished()
{
    return items.empty();
}

#pragma auto_inline(off)
// FUNCTION: 0x473560
void TeleportParticle::Step()
{
    pos1 += dir;
    frame = (frame + 1) % frameCount;
}
#pragma auto_inline(on)

// FUNCTION: 0x473590
void TeleportParticle::DrawParticle(void* dest, short px, short py)
{
    Pos_00473590* q = &posw;
    short sx = q->x - px + 0x80;
    short sy = q->y - (q->h >> 1) - py + 0x20;
    Player* p = &g_game->players[g_game->playerIndex];
    Player* p2 = &g_game->players[g_game->playerIndex];
    int visible;
    if ((g_game->mapFlags & 2) == 2) {
        int col = posw.x >> 5;
        int row = (posw.y - (posw.h >> 1)) >> 5;
        if (InsideExplored(p, col, row) &&
            p->explored[p2->exploredWidth * row + col] != 0)
            visible = 1;
        else
            visible = 0;
    } else {
        int col = posw.x >> 5;
        int row = (posw.y - (posw.h >> 1)) >> 5;
        visible = Identity_00473590(IsSeen_00473590(p, p2, col, row));
    }
    if (visible)
        DrawFrameBlended(dest, GetGafFrame(data, frame), sx, sy);
}

#pragma auto_inline(off)
// FUNCTION: 0x4736c0
int TeleportParticle::IsExpired(int currentTick)
{
    return currentTick > endTime;
}
#pragma auto_inline(on)

// FUNCTION: 0x4736e0
void TeleportParticles::Init(Vec3_004736e0* a, Vec3_004736e0* b, int c)
{
    SetLifetime(c);
    pos1 = *a;
    pos2 = *b;
    dir = pos2 - pos1;
    int dist = dir.Length();
    Fixed16 scale;
    scale.whole = (int)(((__int64)dist << 16) / 327680);
    int step = scale.half[1];
    sparkLifetime = step;
    dir.x /= step;
    dir.y /= step;
    dir.z /= step;
    Emit();
}

// FUNCTION: 0x4737c0
void TeleportParticles::Emit()
{
    int grow = (deadline - g_game->ticks + 10) / 10;

    if (grow > 0)
        items.reserve(grow + items.size());

    for (int i = 0; i < 1; i++) {
        TeleportParticle e;

        e.pos1 = pos1;
        e.pos2 = pos2;
        e.dir = dir;
        e.endTime = g_game->ticks + sparkLifetime;
        e.data = g_game->flamestreamGaf;
        e.frameCount = GetGafFrameCount(g_game->flamestreamGaf) - 1;
        e.frame = (int)(((__int64)HarnessViewRand() * e.frameCount) / 0x8000);
        items.insert(items.end(), 1, e);
    }

    time = g_game->ticks + 10;
}

#pragma auto_inline(off)
// FUNCTION: 0x4739b0
void NanoParticle::Step()
{
    short frame = (flags & 0xf) + 1;
    pos += vel;
    if (frame > 7)
        frame = 1;
    flags = (flags & 0xfff0) + frame;
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x473b30
int NanoParticle::IsExpired(int value)
{
    return value > endTime;
}
#pragma auto_inline(on)

// FUNCTION: 0x473b50
void NanoParticles::Init(Seg_00473b50* a, Seg_00473b50* b, int c)
{
    SetLifetime(c);
    seg_1c = *a;
    seg_34 = *b;
    SPLIT_SEG(seg_34);
    SPLIT_SEG(seg_1c);
    Emit();
}

#pragma auto_inline(off)
// FUNCTION: 0x474130
void ThrustParticle::Step()
{
    pos0 += pos2;
    tick = (tick + 1) % period;
    if (tick == 0) {
        frame = (frame + 1) % frameCount;
    }
}
#pragma auto_inline(on)

// FUNCTION: 0x474170
void ThrustParticle::DrawParticle(void* dest, short px, short py)
{
    Pos_00473590* q = &posw;
    short sx = q->x - px + 0x80;
    short sy = q->y - (q->h >> 1) - py + 0x20;
    Player* p = &g_game->players[g_game->playerIndex];
    Player* p2 = &g_game->players[g_game->playerIndex];
    int visible;
    if ((g_game->mapFlags & 2) == 2) {
        int col = posw.x >> 5;
        int row = (posw.y - (posw.h >> 1)) >> 5;
        if (InsideExplored(p, col, row) &&
            p->explored[p2->exploredWidth * row + col] != 0)
            visible = 1;
        else
            visible = 0;
    } else {
        int col = posw.x >> 5;
        int row = (posw.y - (posw.h >> 1)) >> 5;
        visible = Identity_00474170(IsSeen_00474170(p, p2, col, row));
    }
    if (visible)
        DrawFrameBlended(dest, GetGafFrame(bitmask, frame), sx, sy);
}

#pragma auto_inline(off)
// FUNCTION: 0x4742a0
int ThrustParticle::IsExpired(int value)
{
    return value > endTime;
}
#pragma auto_inline(on)

// FUNCTION: 0x4742c0
void ThrustParticles::Init(Vec3* p, Vec3* q, int a, int b)
{
    this->SetLifetime(b);
    period = a;
    pos0 = *p;
    pos1 = *q;
    pos2 = pos1 - pos0;
    int scale = (int)(((__int64)1 << 32) / (b << 16));
    pos2.Scale(scale);
    Emit();
}

// FUNCTION: 0x4743a0
void ThrustParticles::Emit()
{
    int extra = deadline - g_game->ticks + 1;
    if (extra > 0) {
        items.reserve(extra + items.size());
    }
    Vec3* p = &pos0;
    for (int i = 1; i != 0; i--) {
        ThrustParticle rec;
        rec.period = period;
        rec.endTime = deadline;
        rec.tick = 0;
        rec.pos0 = *p;
        rec.pos1 = pos1;
        rec.pos2 = pos2;
        rec.bitmask = (unsigned short *)g_game->flamestreamGaf;
        rec.frameCount = (int)GetGafFrameCount(g_game->flamestreamGaf) - 1;
        rec.frame = 0;
        items.insert(items.end(), 1, rec);
    }
    time = g_game->ticks + 1;
}

#pragma auto_inline(off)
// FUNCTION: 0x474580
void WakeParticle::Step()
{
    pos += vel;
    tick = (tick + 1) % period;
    if (tick == 0) {
        value += step;
        if (value > max) value = min;
        if (value < min) value = max;
    }
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x4745e0
void WakeParticle::DrawParticle(void* surface, short px, short py)
{
    Rect r;
    short sx = posw.x - px;
    short sy = posw.y - py;
    r.left = sx + 0x80;
    r.top = sy - (posw.height >> 1) + 0x20;
    r.right = r.left + 1;
    r.bottom = r.top + 1;
    if (posw.Visible())
        FillRectangle(surface, &r, value);
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x474720
int WakeParticle::IsExpired(int currentTick)
{
    if (currentTick <= endTime) {
        int r = GetGroundHeight(&pos);
        if (r < g_game->seaLevel)
            return 0;
    }
    return 1;
}
#pragma auto_inline(on)

// FUNCTION: 0x474760
void WakeParticles::Init(Vec3_00474760* a, Vec3_00474760* b, int newPeriod,
                                  int lifetime, int isAscending)
{
    SetLifetime(lifetime);
    period = newPeriod;
    pos_a = *a;
    pos_b = *b;
    dir = pos_b - pos_a;
    int scale = 0x100000000 / (int)(((__int64)dir.Length() * 0x20000) >> 16);
    dir.Scale(scale);
    ascending = isAscending;
    Emit();
}

// FUNCTION: 0x474880
void WakeParticles::Emit()
{
    int grow = deadline - g_game->ticks + 1;

    if (grow > 0)
        items.reserve(grow + items.size());

    int i = 1;
    do {
        WakeParticle e;

        e.tick = 0;
        e.period = period;
        e.endTime = g_game->ticks + period * 6;
        e.pos = pos_a;
        e.pos.xp.hi += (int)((__int64)HarnessViewRand() * 7 / 0x8000) - 3;
        e.pos.yp.hi += (int)((__int64)HarnessViewRand() * 7 / 0x8000) - 3;
        e.pos.zp.hi += (int)((__int64)HarnessViewRand() * 7 / 0x8000) - 3;
        e.pos2 = pos_b;
        e.vel = dir;
        e.data = g_game->smoke1;
        e.min = 0x61;
        e.max = 0x67;
        if (ascending) {
            e.value = 0x61;
            e.step = 1;
        } else {
            e.value = 0x67;
            e.step = -1;
        }
        items.insert(items.end(), 1, e);
    } while (--i);

    time = g_game->ticks + 1;
}

// FUNCTION: 0x474b00
void SmokeParticle::Step()
{
    pos.x += g_game->windX * 8;
    pos.y += g_game->gravity * 4;
    pos.z += g_game->windZ * 8;
    if (--timer == 0) {
        count++;
        int half = period / 2;
        timer = (int)((__int64)HarnessViewRand() * half / 0x8000) + half;
    }
}

// FUNCTION: 0x474cb0
int SmokeParticle::IsExpired(int unused)
{
    return count >= limit;
}

// FUNCTION: 0x474d50
void SmokeParticles::Init(Vec3* p, int limit, int a, int b, int c,
                                  int alt)
{
    SetLifetime(c);
    pos = *p;
    emitPeriod = a;
    altAnimation = alt;
    if (alt)
        maxFrame = GetGafFrameCount(g_game->smoke2) - 1;
    else
        maxFrame = GetGafFrameCount(g_game->smoke1) - 1;
    if (limit != 0)
        maxFrame = limit < maxFrame ? limit : maxFrame;
    if (b != 0)
        holdPeriod = b;
    else
        holdPeriod = 7;
    Emit();
}

// FUNCTION: 0x474df0
void SmokeParticles::Emit()
{
    int periods = (deadline - g_game->ticks + emitPeriod) / emitPeriod;
    if (periods > 0) {
        records.reserve(periods + records.size());
    }
    Vec3* p = &pos;
    for (int i = 1; i != 0; i--) {
        SmokeParticle rec;
        rec.pos = *p;
        rec.period = holdPeriod;
        rec.timer = holdPeriod;
        rec.data = altAnimation ? g_game->smoke2 : g_game->smoke1;
        rec.limit = (int)(((__int64)HarnessViewRand() * (maxFrame - 2)) / 0x8000) + 2;
        rec.count = 0;
        records.insert(records.end(), 1, rec);
    }
    time = g_game->ticks + emitPeriod;
}

// FUNCTION: 0x474f80
int SmokeParticles::IsFinished()
{
    int count;

    count = records.size();

    bool isZero = (count == 0);
    if (isZero) {
        if ((unsigned int)deadline < (unsigned int)g_game->ticks) {
            return 1;
        }
    }

    return 0;
}

// FUNCTION: 0x474fc0
void TimedSubParticle::Step()
{
    pos.x += g_game->windX * 8;
    pos.y += g_game->gravity * 16;
    pos.z += g_game->windZ * 8;
    if (--timer == 0) {
        count++;
        int half = period / 2;
        timer = (int)((__int64)HarnessViewRand() * half / 0x8000) + half;
    }
}

// FUNCTION: 0x475040
void TimedSubParticle::DrawParticle(void* dest, short px, short py)
{
    short sy = posw.z - (posw.y >> 1) - py + 0x20;
    short sx = posw.x - px + 0x80;
    DrawFrameBlended(dest, GetGafFrame(data, count), sx, sy);
}

// FUNCTION: 0x475090
int TimedSubParticle::IsExpired(int unused)
{
    return count >= limit ? 1 : 0;
}

// FUNCTION: 0x475150
void TimedSubParticles::Init(Vec3* p, int a, int b, int c)
{
    SetLifetime(c);
    pos = *p;
    emitPeriod = a;
    maxFrame = GetGafFrameCount(g_game->smoke1) - 1;
    if (b != 0)
        holdPeriod = b;
    else
        holdPeriod = 7;
    Emit();
}

// FUNCTION: 0x4751c0
void TimedSubParticles::Emit()
{
    int missed = (deadline - g_game->ticks + emitPeriod) / emitPeriod;
    if (missed > 0)
        records.reserve(records.size() + missed);
    Vec3* p = &pos;
    int i = 1;
    do {
        TimedSubParticle rec;
        rec.pos = *p;
        rec.period = holdPeriod;
        rec.timer = holdPeriod;
        rec.data = g_game->smoke1;
        rec.limit = (int)((__int64)HarnessViewRand() * (maxFrame - 2) / 0x8000) + 2;
        rec.count = 0;
        records.insert(records.end(), 1, rec);
    } while (--i);
    time = g_game->ticks + emitPeriod;
}

// FUNCTION: 0x475330
int TimedSubParticles::IsFinished()
{
    return 0;
}

// FUNCTION: 0x475340
void SmokeParticles::Update()
{
    std::vector<SmokeParticle>::iterator it = records.begin();
    while (it != records.end()) {
        it->pos.x += g_game->windX * 8;
        it->pos.y += g_game->gravity * 4;
        it->pos.z += g_game->windZ * 8;
        if (--it->timer == 0) {
            it->count++;
            int half = it->period / 2;
            it->timer = (int)((__int64)HarnessViewRand() * half / 0x8000) + half;
        }
        if (it->count >= it->limit) {
            records.erase(it);
        } else {
            ++it;
        }
    }
    if (IsEmitDue())
        Emit();
}

// FUNCTION: 0x475440
int SmokeParticles::IsEmitDue()
{
    if (time <= deadline && (unsigned int)time <= (unsigned int)g_game->ticks) {
        return 1;
    }
    return 0;
}

// FUNCTION: 0x475600
void TimedSubParticles::Update()
{
    std::vector<TimedSubParticle>::iterator it = records.begin();
    while (it != records.end()) {
        it->pos.x += g_game->windX * 8;
        it->pos.y += g_game->gravity * 16;
        it->pos.z += g_game->windZ * 8;
        if (--it->timer == 0) {
            it->count++;
            int half = it->period / 2;
            it->timer = (int)((__int64)HarnessViewRand() * half / 0x8000) + half;
        }
        if (it->count >= it->limit) {
            records.erase(it);
        } else {
            ++it;
        }
    }
    if (IsEmitDue())
        Emit();
}

// FUNCTION: 0x475840
int NanoParticleVector::GetCount()
{
    return _First == 0 ? 0 : (_Last - _First) / 48;
}

struct Shape_00472ab0 {
    Pair_00474880 v[3];
};

// FUNCTION: 0x472ab0
void __stdcall EmitJitteredThrustParticles(Shape_00472ab0* pos, short index)
{
    Shape_00472ab0 s = *pos;
    s.v[0].hi += (int)((__int64)HarnessViewRand() * 3 / 0x8000) - 1;
    s.v[1].hi += (int)((__int64)HarnessViewRand() * 3 / 0x8000) - 1;
    s.v[2].hi += (int)((__int64)HarnessViewRand() * 3 / 0x8000) - 1;
    int r = (int)((__int64)HarnessViewRand() * 3 / 0x8000) + 1;
    Lists_00472330* lists = (Lists_00472330 *)g_game->lists_38d77;
    ThrustParticles* p = new ThrustParticles;
    if (p) {
        p->Init(&s, &s, 1, r);
        lists->Add(index, p);
    }
}

// FUNCTION: 0x470c10
__declspec(noinline) int ObjectPool::Grow(int newCapacity, int newSlotSize)
{
    int result = 0;
    if (newCapacity > capacity) {
        void** table = (void**)GameReallocIgnoreTag((int*)slots, newCapacity * 4);
        if (table != 0) {
            PoolBlock* block = (PoolBlock*)GameAllocShared((newCapacity - capacity) * newSlotSize);
            slots = table;
            if (block != nullptr) {
                int i = capacity;
                if (i < newCapacity) {
                    int offset = 0;
                    do {
                        ((void**)slots)[i] = (char*)block + offset;
                        offset += newSlotSize;
                        i++;
                    } while (i < newCapacity);
                }
                items.push_back(block);
                capacity = newCapacity;
                slotSize = newSlotSize;
                result = 1;
            }
        }
    }
    else {
        result = 1;
    }
    return result;
}

// FUNCTION: 0x471160
void ParticleLists::AddToList(ParticleSystem* x, short index)
{
    if (lists[index].size() > 400) {
        delete lists[index][0];
        lists[index].erase(lists[index].begin());
    }
    lists[index].push_back(x);
}

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

// FUNCTION: 0x471de0
void __stdcall DestroyParticleLists()
{
    if (g_game->lists_38d77) {
        delete (ParticleLists*)g_game->lists_38d77;
        g_game->lists_38d77 = 0;
    }
}

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

typedef void (std::vector<ParticleSystem*>::*ParticleSystemPtrInsertFn)(
    std::vector<ParticleSystem*>::iterator, std::vector<ParticleSystem*>::size_type, ParticleSystem* const&);

// FUNCTION: 0x4732e0 ?insert@?$vector@PAVParticleSystem@@V?$allocator@PAVParticleSystem@@@std@@@std@@QAEXPAPAVParticleSystem@@IABQAV3@@Z
ParticleSystemPtrInsertFn g_particleSystemPtrInsert = &std::vector<ParticleSystem*>::insert;

// FUNCTION: 0x473a00
void NanoParticle::DrawParticle(int dest, short x, short y)
{
    Rect r;
    r.left = (short)(posw.x - x) + 0x80;
    r.top = (short)(posw.y - y) - (posw.height >> 1) + 0x20;
    r.right = r.left + 1;
    r.bottom = r.top + 1;

    Player* p = &g_game->players_1b63[g_game->playerIndex];
    Player* q = &g_game->players_1b63[g_game->playerIndex];
    int visible;
    if ((g_game->mapFlags & 2) == 2) {
        int col = posw.x >> 5;
        int row = (posw.y - (posw.height >> 1)) >> 5;
        visible = (unsigned int)col < (unsigned int)p->exploredWidth && (unsigned int)row < (unsigned int)p->exploredHeight &&
                  p->explored[(unsigned int)p->exploredWidth * row + col] != 0;
    } else {
        int col = posw.x >> 5;
        int row = (posw.y - (posw.height >> 1)) >> 5;
        visible = Identity_00473a00(IsSeen_00473a00(p, q, col, row));
    }

    if (visible)
        FillRectangle((void*)dest, &r, flags);
}

static inline int LengthFloat(const Vec3& v)
{
    float fx = v.x;
    float fy = v.y;
    float fz = v.z;
    return (int)sqrt(fx * fx + fy * fy + fz * fz);
}

// FUNCTION: 0x473d50
void NanoParticles::Emit()
{
    int grow = deadline - g_game->ticks + 1;
    if (grow > 0)
        items.reserve(items.size() + grow * 5);

    int i = 0;
    for (int n = 5; n != 0; n--) {
        NanoParticle e;
        e.pos.x = (int)(((__int64)HarnessViewRand() * seg_1c.end.x) / 0x8000) + seg_1c.start.x;
        e.pos.y = (int)(((__int64)HarnessViewRand() * seg_1c.end.y) / 0x8000) + seg_1c.start.y;
        e.pos.z = (int)(((__int64)HarnessViewRand() * seg_1c.end.z) / 0x8000) + seg_1c.start.z;
        e.tgt.x = (int)(((__int64)HarnessViewRand() * seg_34.end.x) / 0x8000) + seg_34.start.x;
        e.tgt.y = (int)(((__int64)HarnessViewRand() * seg_34.end.y) / 0x8000) + seg_34.start.y;
        e.tgt.z = (int)(((__int64)HarnessViewRand() * seg_34.end.z) / 0x8000) + seg_34.start.z;

        e.vel = e.tgt - e.pos;
        int len = LengthFloat(e.vel);
        Fixed16 step;
        step.whole = (int)(((__int64)len << 16) / 0x40000);
        short s = step.half[1];
        if (s != 0) {
            e.vel.x = e.vel.x / s;
            e.vel.y = e.vel.y / s;
            e.vel.z = e.vel.z / s;
            e.endTime = g_game->ticks + s;
            e.unused = 0x100;
            e.flags = i % 7 + 0xa1;
            items.push_back(e);
        }
        i++;
    }
    time = g_game->ticks + 1;
}

// FUNCTION: 0x474b80
void SmokeParticle::DrawParticle(void* dest, short px, short py)
{
    short sx = posw.x - px + 0x80;
    short sy = posw.z - (posw.y >> 1) - py + 0x20;
    Player* p = &g_game->players[g_game->playerIndex];
    Player* p2 = &g_game->players[g_game->playerIndex];
    int visible;
    if ((g_game->mapFlags & 2) == 2) {
        int col = posw.x >> 5;
        int row = (posw.z - (posw.y >> 1)) >> 5;
        if ((unsigned int)col < (unsigned int)p->exploredWidth && (unsigned int)row < (unsigned int)p->exploredHeight &&
            p->explored[(unsigned int)p->exploredWidth * row + col] != 0)
            visible = 1;
        else
            visible = 0;
    } else {
        int col = posw.x >> 5;
        int row = (posw.z - (posw.y >> 1)) >> 5;
        visible = Identity_00474b80(IsSeen_00474b80(p, p2, col, row));
    }
    if (visible)
        DrawFrameBlended(dest, GetGafFrame(data, count), sx, sy);
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

// FUNCTION: 0x4750f0
int TimedSubParticles::IsEmitDue()
{
    return (unsigned int)time <= (unsigned int)g_game->ticks;
}

static inline int IsExplored(Player* map, Position_00475470* pos)
{
    int tx = pos->x >> 5;
    int ty = (pos->z - (pos->y >> 1)) >> 5;
    if ((unsigned int)tx < (unsigned int)map->exploredWidth && (unsigned int)ty < (unsigned int)map->exploredHeight &&
        map->explored[(unsigned int)map->exploredWidth * ty + tx])
        return 1;
    return 0;
}

static inline int IsSeen(Player* map, Position_00475470* pos)
{
    int tx = pos->x >> 5;
    int ty = (pos->z - (pos->y >> 1)) >> 5;
    if (!((unsigned int)tx < (unsigned int)map->exploredWidth && (unsigned int)ty < (unsigned int)map->exploredHeight)) {
        return 0;
    }
    Player* b = map;
    return (g_game->visibilityMask[(unsigned int)b->exploredWidth * ty + tx] &
            (1 << g_game->playerIndex)) != 0;
}

static inline int IsVisible(Player* map, Position_00475470* pos)
{
    if ((g_game->mapFlags & 2) == 2)
        return IsExplored(map, pos);
    return IsSeen(map, pos);
}

static inline void DrawIfVisible(SmokeParticle* s, void* dest, short px, short py)
{
    short sx = s->posw.x - px + 0x80;
    short sy = s->posw.z - (s->posw.y >> 1) - py + 0x20;
    if (IsVisible(&g_game->players[g_game->playerIndex], &s->posw))
        DrawFrameBlended(dest, GetGafFrame(s->data, s->count), sx, sy);
}

// FUNCTION: 0x475470
void SmokeParticles::Render(int dest)
{
    for (std::vector<SmokeParticle>::iterator it = records.begin(); it != records.end();
         ++it) {
        DrawIfVisible(&*it, (void*)dest, g_game->scrollX, g_game->scrollY);
    }
}

// FUNCTION: 0x475700
void TimedSubParticles::Render(int dest)
{
    for (std::vector<TimedSubParticle>::iterator it = records.begin(); it != records.end(); ++it) {
        it->DrawParticle((void*)dest, g_game->scrollX, g_game->scrollY);
    }
}

typedef void (std::vector<TeleportParticle>::*TeleportParticleInsertFn)(
    std::vector<TeleportParticle>::iterator, std::vector<TeleportParticle>::size_type, const TeleportParticle&);

// FUNCTION: 0x4758c0 ?insert@?$vector@UTeleportParticle@@V?$allocator@UTeleportParticle@@@std@@@std@@QAEXPAUTeleportParticle@@IABU3@@Z
TeleportParticleInsertFn g_teleportParticleInsert = &std::vector<TeleportParticle>::insert;

typedef void (std::vector<ThrustParticle>::*ThrustParticleInsertFn)(
    std::vector<ThrustParticle>::iterator, std::vector<ThrustParticle>::size_type, const ThrustParticle&);

// FUNCTION: 0x475bd0 ?insert@?$vector@UThrustParticle@@V?$allocator@UThrustParticle@@@std@@@std@@QAEXPAUThrustParticle@@IABU3@@Z
ThrustParticleInsertFn g_thrustParticleInsert = &std::vector<ThrustParticle>::insert;

typedef void (std::vector<WakeParticle>::*WakeParticleInsertFn)(
    std::vector<WakeParticle>::iterator, std::vector<WakeParticle>::size_type, const WakeParticle&);

// FUNCTION: 0x475ef0 ?insert@?$vector@UWakeParticle@@V?$allocator@UWakeParticle@@@std@@@std@@QAEXPAUWakeParticle@@IABU3@@Z
WakeParticleInsertFn g_wakeParticleInsert = &std::vector<WakeParticle>::insert;

typedef void (std::vector<SmokeParticle>::*SmokeParticleInsertFn)(
    std::vector<SmokeParticle>::iterator, std::vector<SmokeParticle>::size_type, const SmokeParticle&);

// FUNCTION: 0x476210 ?insert@?$vector@USmokeParticle@@V?$allocator@USmokeParticle@@@std@@@std@@QAEXPAUSmokeParticle@@IABU3@@Z
SmokeParticleInsertFn g_smokeParticleInsert = &std::vector<SmokeParticle>::insert;

typedef void (std::vector<TimedSubParticle>::*TimedSubParticleInsertFn)(
    std::vector<TimedSubParticle>::iterator, std::vector<TimedSubParticle>::size_type, const TimedSubParticle&);

// FUNCTION: 0x476490 ?insert@?$vector@UTimedSubParticle@@V?$allocator@UTimedSubParticle@@@std@@@std@@QAEXPAUTimedSubParticle@@IABU3@@Z
TimedSubParticleInsertFn g_timedSubParticleInsert = &std::vector<TimedSubParticle>::insert;
