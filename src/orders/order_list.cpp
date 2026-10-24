#include "win_types.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include <vector>
#include <algorithm>
#include <iterator>

#include "../state/block.h"
#include "../util/vec3.h"
#include "../../interfaces/ref.h"

static inline int Vec3Length(const Vec3* v)
{
    double fx = v->x;
    double fy = v->y;
    double fz = v->z;
    return (int)sqrt(fx * fx + fy * fy + fz * fz);
}

static inline void Vec3Scale(Vec3* v, int s)
{
    v->x = (int)(((__int64)v->x * s) >> 16);
    v->y = (int)(((__int64)v->y * s) >> 16);
    v->z = (int)(((__int64)v->z * s) >> 16);
}

static inline void Vec3Add(Vec3* v, const Vec3* o)
{
    v->x += o->x;
    v->y += o->y;
    v->z += o->z;
}
#include "../files/files_api.h"

#pragma pack(push, 1)

struct FP_0043d6d0 {
    unsigned int frac : 16;
    int whole : 16;
};

union Fixed_0043d6d0 {
    int value;
    FP_0043d6d0 parts;
};

static inline Fixed_0043d6d0 MakeFixed_0043d6d0(int i)
{
    Fixed_0043d6d0 f;
    f.parts.frac = 0;
    f.parts.whole = i;
    return f;
}

inline int operator>(const Fixed_0043d6d0& a, const Fixed_0043d6d0& b) { return a.value > b.value; }

#define MAXM_0043d6d0(a, b) ((a) > (b) ? (a) : (b))

static inline void MulFixed(int& v, int f)
{
    v = (int)(((__int64)v * f) >> 16);
}

static inline Vec3 OrderVec3(int a, int b, int c)
{
    Vec3 v;
    v.x = a;
    v.y = b;
    v.z = c;
    return v;
}

static inline Vec3 PosPlus(const Vec3& a, const Vec3& b)
{
    Vec3 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}
static inline Vec3 PosMinus(const Vec3& a, const Vec3& b)
{
    Vec3 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

struct Vec3Init_0043a1f0 : Vec3 {
    Vec3Init_0043a1f0(int a, int b, int c) { x = a; y = b; z = c; }
};

struct PointInit_0043a1f0 : Point16 {
    PointInit_0043a1f0() {}
    PointInit_0043a1f0(short a, short b) { x = a; y = b; }
};

#include "../util/angles.h"
#include "../../interfaces/seams.h"

#include "../network/player.h"

struct Target_0043d6d0 {
    int active;
    char unknown_4[0x73 - 0x4];
    unsigned char type;
    char unknown_74[0xe4 - 0x74];
    unsigned short field_e4;
};

struct TargetData_0043d6d0 {
    char unknown_0[8];
    Vec3 velocity;
    char unknown_14[0x20 - 0x14];
    int speed;
};

struct Path_0043d6d0 {
    Ref<TargetData_0043d6d0> motion;
};

class MovementClass;
struct UnitType_0043cd20 {
    char unknown_0[0x20];
    char name[0x20];
    char unknown_40[0x192 - 0x40];
    int maxvelocity;
    char unknown_196[0x19a - 0x196];
    int brakerate;
    int acceleration;
    int bankscale;
    int pitchscale;
    char unknown_1aa[0x1ae - 0x1aa];
    int moverate1;
    int moverate2;
    Ref<MovementClass> movementclass;
    unsigned short max_turn;
    char unknown_1bc[0x202 - 0x1bc];
    short range;
    char unknown_204[0x214 - 0x204];
    unsigned short maneuverleashlength;
    char unknown_216[0x22c - 0x216];
    unsigned char draft;
    char unknown_22d[0x230 - 0x22d];
    unsigned char defaultmissiontype;
    char unknown_231[0x241 - 0x231];
    union {
        int flags1;
        unsigned char flags;
        struct {
            unsigned int mode_bits : 11;
            unsigned int flag_800 : 1;
        };
        struct {
            unsigned int low : 19;
            unsigned int b19 : 1;
            unsigned int rest : 12;
        };
    };
    char unknown_245[0x249 - 0x245];
};

#include "../units/cob_script.h"

struct Unit;

#include "order.h"

#include "../weapons/weapon_def.h"

#include "../weapons/unit_weapon_slot.h"

#include "../game/game.h"

#include "../units/unit.h"

static inline short PosXWhole(Unit* unit) { return *(short*)((char*)&unit->pos + 2); }
static inline short PosZWhole(Unit* unit) { return *(short*)((char*)&unit->pos + 0xa); }

#include "order_type_entry.h"
#include "../state/sim_globals.h"


#pragma pack(pop)

extern Game* g_game;
typedef Elem_0043c390* SimView_g_missionOrderTableBegin;
#undef g_missionOrderTableBegin
#define g_missionOrderTableBegin (*reinterpret_cast<SimView_g_missionOrderTableBegin*>(reinterpret_cast<char*>(&g_missionOrderTableVec) + 0x4))

void __stdcall DeleteOrder(Unit* owner, Order* node);

extern int __cdecl _strcmpi(const char*, const char*);


void __stdcall DrawBuildFootprint(void* a, void* b, Order* e, Vec3* p, int c);
void __stdcall DrawUnitRangeRings(void* a, void* b, Order* e, Vec3* p, int c);
void __stdcall DrawPathAnim(void* a, void* b, Order* e, Vec3* p, int c);
void __stdcall DrawWeaponCoverage(void* a, void* b, Order* e, Vec3* p, int c);
void __stdcall DrawOrderRangeRing(void* a, void* b, Order* e, Vec3* p, int c);

// FUNCTION: 0x439b30
void __stdcall DrawOrderOverlays(Unit* unit, unsigned int mask, void* obj,
                            void* sel, int flag)
{
    Vec3 base = unit->pos;
    Vec3 pos = unit->pos;
    bool done = false;
    for (Order* e = unit->list; e != 0; e = e->next) {
        if (g_missionOrderTableBegin[e->kind.index].drawMask & mask & 1) {
            pos = base;
            DrawBuildFootprint(obj, sel, e, &pos, flag);
        }
        if (g_missionOrderTableBegin[e->kind.index].drawMask & mask & 2) {
            pos = base;
            DrawPathAnim(obj, sel, e, &pos, flag);
        }
        if (g_missionOrderTableBegin[e->kind.index].drawMask & mask & 4) {
            pos = base;
            DrawOrderRangeRing(obj, sel, e, &pos, flag);
        }
        if (g_missionOrderTableBegin[e->kind.index].drawMask & mask & 8) {
            pos = base;
            DrawWeaponCoverage(obj, sel, e, &pos, flag);
        }
        if (g_missionOrderTableBegin[e->kind.index].drawMask & mask & 0x10) {
            if (!done) {
                DrawUnitRangeRings(obj, sel, e, &pos, flag);
                done = true;
            }
        }
        base = pos;
    }
}

// FUNCTION: 0x439cf0
int __stdcall GetOrderTargetIfFlagged(void* param_1)
{
    int eax;
    if (param_1 == 0) {
        eax = 0;
    } else {
        int* field_5c = (int*)((Unit*)param_1)->list.get();
        if (field_5c == 0)
            eax = 0;
        else
            eax = *(int*)((char*)field_5c + 0x42);
    }
    if ((eax & 8) != 0)
        return (int)((Unit*)param_1)->list->target.get();
    return 0;
}

// FUNCTION: 0x439d20
int __stdcall GetBuildWeaponPercent(Unit* owner)
{
    Order* p;
    for (p = owner->list2; p; p = p->next) {
        if (p->flags_42 & 0x80000)
            return p->progress * 100 / owner->weapons[p->id].weapon->reloadTime;
    }
    return 0;
}

// FUNCTION: 0x439d80
int __stdcall SumQueuedBuildCount(Unit* owner, int index)
{
    int total = 0;
    Order* p;
    for (p = owner->list; p; p = p->next) {
        if ((p->flags_42 & 0x100) && p->id == index)
            total += p->count;
    }
    for (p = owner->list2; p; p = p->next) {
        if ((p->flags_42 & 0x100) && p->id == index)
            total += p->count;
    }
    return total;
}

// FUNCTION: 0x439dd0
int __stdcall GetOrderTarget(int param_1)
{
    int eax = param_1;
    if (eax != 0) {
        eax = (int)((Unit*)eax)->list.get();
        if (eax != 0) {
            eax = (int)((Order*)eax)->target.get();
            return eax;
        }
    }
    return 0;
}

// FUNCTION: 0x439df0
int __stdcall GetOrderName(Unit* obj)
{
    if (obj && obj->list) {
        return g_missionOrderTableBegin[obj->list->kind.index].value;
    }
    return g_missionOrderTableBegin[0].value;
}

// FUNCTION: 0x439e30
Order* __stdcall FindOrderByType(Unit* obj, unsigned char kind)
{
    Order* n;
    if (g_missionOrderTableBegin[kind].flags & 0x40000)
        n = obj->list2;
    else
        n = obj->list;
    while (n) {
        if (n->kind.index == kind)
            return n;
        n = n->next;
    }
    return 0;
}

// FUNCTION: 0x439e80
void Order::SetDeadlineTicks(int param)
{
    *(unsigned int*)((char*)this + 0x6) |= 1;
    int val = *(int*)((char*)g_game + 0x38a47);
    *(int*)((char*)this + 0xa) = val + param;
}

// FUNCTION: 0x439ea0
int __stdcall ReadyOrder(int arg1, int arg2, int arg3)
{
    return 5;
}

// FUNCTION: 0x439eb0
void __stdcall DeleteOrders(Unit* owner, int all)
{
    Order* first = owner->list;
    Ref<Order>* pp = &owner->list;
    Order* node;
    while ((node = *pp) != 0) {
        if (!all && (node->flags_42 & 4)) {
            pp = &node->next;
        } else {
            *pp = node->next;
            if (node != first) {
                node->flags_42 |= 0x10000;
            }
            delete node;
        }
    }
    if (all) {
        while ((node = owner->list2) != 0)
            DeleteOrder(owner, node);
    }
}

// FUNCTION: 0x439f80
void __stdcall DeleteOrder(Unit* owner, Order* node)
{
    Order* first = owner->list;
    Ref<Order>* link = (node->flags_42 & 0x40000) ? &owner->list2 : &owner->list;
    for (Order* n = *link; n != 0; n = n->next) {
        if (n == node) {
            *link = node->next;
            if (node != first) {
                node->flags_42 |= 0x10000;
            }
            delete node;
            return;
        }
        link = &n->next;
    }
}

// FUNCTION: 0x439fe0 ?MoveOrderToTail@@YGXPAD0@Z
void __stdcall MoveOrderToTail(char* param_1, char* param_2)
{
    Ref<Order>* pp = &((Unit*)param_1)->list;
    Order* node = *pp;
    for (; node != (Order*)param_2; node = node->next) {
        pp = &node->next;
    }
    node = ((Order*)param_2)->next;
    *pp = node;
    for (; node != 0; node = node->next) {
        pp = &node->next;
    }
    *pp = (Order*)param_2;
    ((Order*)param_2)->next = 0;
}

static inline void InsertBefore(Unit* p, Order* child, Order* before)
{
    Ref<Order>* link = (child->flags_42 & 0x40000) ? &p->list2 : &p->list;
    while (*link != before) {
        link = &(*link)->next;
    }
    *link = child;
    child->unit = p;
    child->next = before;
    if (before != 0) {
        child->flags_42 |= before->flags_42 & 0x4000;
    }
}

// FUNCTION: 0x43a020
void __stdcall EnsurePatrolReturnOrder(Unit* p, Order* item)
{
    int add = 1;
    for (Order* c = p->list; c != 0; c = c->next) {
        if (c->flags_42 & 0x8000) {
            add = 0;
            break;
        }
    }
    if (add) {
        Order* child = new (kSim) Order(item->kind.index, 0, &p->pos, 0, 0, 0);
        InsertBefore(p, child, 0);
    }
    item->flags_42 |= 0x8000;
}

static inline MissionType MissionTypeOf(unsigned char k)
{
    MissionType m;
    m.index = k;
    return m;
}

// FUNCTION: 0x43a0c0
Order::Order(unsigned char k, Unit* o, Vec3* p, int a, int b, int c)
    : kind(MissionTypeOf(k)), link(o, 0), start(PointInit_0043a1f0(0, 0)), cached(PointInit_0043a1f0(0, 0)),
      id(a), count(b), progress(c), created((unsigned int)g_game->ticks)
{
    link.SetValue(this);
    state = 0;
    flags = 0;
    subFlags = 0;
    wakeFrame = -1;
    pos = p ? *p : Vec3Init_0043a1f0(0, 0, 0);
    flags_42 = g_missionOrderTableBegin[k & 0xff].flags;
    unit = 0;
    next = 0;
    attached = 0;
    if (o == 0)
        flags_42 &= ~0x200;
    if (p == 0)
        flags_42 &= ~0x400;
    if (!(flags_42 & 0x200))
        link.SetUnit(0);
}

// FUNCTION: 0x43a1e0
void OrderBase::OrStatusFlags(unsigned int)
{
}

// FUNCTION: 0x43a2d0
void __stdcall WriteUnitTypeNameKey(HapiBank* file, unsigned short id)
{
    char key[0x80];
    sprintf(key, "UTYPENAME%4d", id);
    if (!file->HasItem(key) && id >= 1 && id < g_game->unitDefCount)
        file->SetStringItem(key, ((UnitType_0043cd20 *)g_game->defs.get())[id].name);
}

// FUNCTION: 0x43a360
short __stdcall ResolveUnitTypeKey(HapiBank* file, unsigned short id)
{
    char key[0x80];
    sprintf(key, "UTYPENAME%4d", id);
    if (file->HasItem(key))
        return g_seams.units->FindUnitTypeId(file->GetStringItem(key, 0));
    int i, n = 0;
    unsigned short k = 0;
    for (i = 1; i < g_game->unitDefCount; i++, n++) {
        if (!(((UnitType_0043cd20 *)g_game->defs.get())[(unsigned short)i].flags & 0x20)) {
            if (k == id)
                return n;
            k++;
        }
    }
    return 0;
}

// FUNCTION: 0x43a940
int __stdcall OrderTypeNameLess(int param_1, char* param_2)
{
    int result = _strcmpi(*(char**)(param_1 + 0x15), param_2);
    return result < 0;
}

// FUNCTION: 0x43ac60
void __stdcall InsertOrderBefore(Unit* owner, Order* node,
                            Order* before)
{
    Ref<Order>* link = (node->flags_42 & 0x40000) ? &owner->list2
                                                   : &owner->list;
    while (*link != before)
        link = &(*link)->next;
    *link = node;
    node->unit = owner;
    node->next = before;
    if (before != 0)
        node->flags_42 |= before->flags_42 & 0x4000;
}

// FUNCTION: 0x43acb0
void __stdcall AppendOrder(Unit* owner, Order* node)
{
    unsigned int which = node->flags_42 & 0x40000;
    Order* before = which ? owner->list2 : owner->list;
    Ref<Order>* link = which ? &owner->list2 : &owner->list;
    while (*link != before) {
        link = &(*link)->next;
    }
    *link = node;
    node->unit = owner;
    node->next = before;
    if (before != 0) {
        node->flags_42 |= before->flags_42 & 0x4000;
    }
}

// FUNCTION: 0x43ad10
void __stdcall AppendOrderToTail(Unit* owner, Order* node)
{
    Ref<Order>* link = (node->flags_42 & 0x40000) ? &owner->list2 : &owner->list;
    for (Order* n = *link; n != 0; n = n->next)
        link = &n->next;
    *link = node;
    node->unit = owner;
    node->next = 0;
}

// FUNCTION: 0x43ad50
void __stdcall InsertOrderAfterMarked(Unit* owner, Order* node)
{
    Ref<Order>* link = &owner->list;
    node->unit = owner;
    node->flags_42 |= 0x1000;
    for (Order* n = *link; n != 0; n = n->next) {
        if (n->flags_42 & 0x1000) {
            n->flags_42 &= ~0x1000;
            node->unit = owner;
            node->next = n->next;
            n->next = node;
            return;
        }
        link = &n->next;
    }
    node->next = 0;
    *link = node;
}

static inline void UnlinkNode(Ref<Order>* link, Order* node, Order* first)
{
    *link = node->next;
    if (node != first)
        node->flags_42 |= 0x10000;
    delete node;
}

static inline void RemoveFromList(Ref<Order>* list, Order* node, Order* first)
{
    Ref<Order>* link = list;
    for (Order* n = *link; n != 0; n = n->next) {
        if (n == node) {
            UnlinkNode(link, node, first);
            return;
        }
        link = &n->next;
    }
}

static inline void PruneLoose(Ref<Order>* link, Order* first)
{
    Order* n = *link;
    while (n != 0) {
        if (n->flags_42 & 4)
            link = &n->next;
        else
            UnlinkNode(link, n, first);
        n = *link;
    }
}

static inline void PruneUsed(Unit* owner)
{
    Ref<Order>* base = &owner->list;
    for (Order* n = owner->list; n != 0; n = owner->list) {
        if (!(n->flags_42 & 0x4000))
            break;
        Order* node = n;
        RemoveFromList((node->flags_42 & 0x40000) ? &owner->list2 : base, node, owner->list);
    }
}

static inline void AddFront(Unit* owner, Order* node)
{
    Order* before = (node->flags_42 & 0x40000) ? owner->list2 : owner->list;
    Ref<Order>* link = (node->flags_42 & 0x40000) ? &owner->list2 : &owner->list;
    while (*link != before)
        link = &(*link)->next;
    *link = node;
    node->unit = owner;
    node->next = before;
    if (before != 0) {
        node->flags_42 |= before->flags_42 & 0x4000;
        return;
    }
}

static inline void AddMarked(Unit* owner, Order* node)
{
    Ref<Order>* link = &owner->list;
    node->unit = owner;
    node->flags_42 |= 0x1000;
    for (Order* n = *link; n != 0; n = n->next) {
        if (n->flags_42 & 0x1000) {
            n->flags_42 &= ~0x1000;
            node->unit = owner;
            node->next = n->next;
            n->next = node;
            return;
        }
        link = &n->next;
    }
    node->next = 0;
    *link = node;
}

// FUNCTION: 0x43adc0
void __stdcall AddOrder(int kind, int remove, Unit* owner, void* id,
                            Vec3* pos, int param_6, int param_7)
{
    Order* obj = new (kSim) Order(kind, (Unit*)id, pos, param_6, param_7, 0);

    if (remove == 0 && !(obj->flags_42 & 0x40))
        PruneLoose(&owner->list, owner->list);

    if (!(obj->flags_42 & 0x40000))
        PruneUsed(owner);

    obj->flags_42 |= 1;
    if (remove == 0)
        obj->flags_42 |= 0x2000;

    if (obj->flags_42 & 0x40020)
        AddFront(owner, obj);
    else
        AddMarked(owner, obj);
}

#pragma pack(push, 1)

class MissionOrderTable {
public:
    char unknown_0[0x4];
    int first;
    int last;

    int GetCount(void);
};

void ResetAIPlayers();
void StepAllGafSequences();
void ResetNetStats();
void InitCommands();
int UpdatePlacementGhostValidity();
void RefreshSelectionOrders();

typedef std::vector<Elem_0043c390> Vec_0043c390;
typedef int(__stdcall* Pred_0043c390)(const Elem_0043c390&, const Elem_0043c390&);

struct Record_0043dd70 {
    Vec3 velocity;
    Vec3 p2;
    int speed;
    short turn;
    int pathLockStamp;
    unsigned char mode : 2;
    unsigned char flag : 1;
};

class Iface_0043dd20 {
public:
    virtual void v0(int);
    virtual void v1();
    virtual void v2();
    virtual void v3(Vec3* out, int first, int count);
    virtual void v4(Vec3* a, Vec3* b, short* heading);
    virtual int v5();
};

#include "order_goals.h"
#include "../ai/ai_search_goal.h"
class Class_0043db50 { public: void UpdateSfxOccupy(Unit* u); };

#include "unit_motion.h"
#pragma pack(pop)

extern Game* g_game;
typedef Elem_0043c390* SimView_g_missionOrderTableBegin;
#undef g_missionOrderTableBegin
#define g_missionOrderTableBegin (*reinterpret_cast<SimView_g_missionOrderTableBegin*>(reinterpret_cast<char*>(&g_missionOrderTableVec) + 0x4))
extern Elem_0043c390 g_readyOrder[];
extern signed char g_slopeSpeedFactor[];


int __cdecl ScaledSin(short angle, int scale);
int __cdecl ScaledCos(short angle, int scale);
void __cdecl RotatePoint2D(short angle, int* xz);
int __cdecl AngleOfVector(int x, int z);
int __stdcall GetHeadingBetween(Vec3* from, Vec3* to);
MissionType __stdcall GetOrderType(unsigned char relation, Unit* unit, Unit* target, Vec3* pos);
void __stdcall AddOrder(unsigned char kind, int remove, Unit* owner, int id, int* pos, int param_6, int param_7);
int __stdcall FindWeaponTarget(Unit* unit, int a, int b);
int __stdcall RandomInt(int n);
int __stdcall CompareOrderTypeNames(const Elem_0043c390& a, const Elem_0043c390& b);
void RegisterGroundOrders();
void RegisterVtolOrders();
void RegisterUnitOrders();
void __stdcall DeleteOrder(Unit* owner, Order* node);
void __stdcall DeleteOrders(Unit* owner, int all);
void __stdcall MoveOrderToTail(Unit* owner, Order* node);
void __stdcall InsertOrderBefore(Unit* owner, Order* node, Order* before);

static inline int InRange(int d)
{
    return (unsigned int)(d + 0x100000) <= 0x200000;
}

static inline int Matches(Order* node, unsigned char kind, int id, int* pos)
{
    return kind == node->kind.index && (id == 0 || id == (int)node->link.owner.get()) &&
           (!pos || (InRange(pos[0] - node->pos.x) && InRange(pos[2] - node->pos.z)));
}

static inline void RemoveNode(Unit* owner, Order* node, Order* first)
{
    Ref<Order>* link = &owner->list;
    if (node->flags_42 & 0x40000) {
        link = &owner->list2;
    }
    for (Order* n = *link; n != 0; n = n->next) {
        if (n == node) {
            *link = node->next;
            if (node != first) {
                node->flags_42 |= 0x10000;
            }
            delete node;
            return;
        }
        link = &n->next;
    }
}

// FUNCTION: 0x43afc0
void __stdcall IssueOrCancelOrder(unsigned char kind, int remove, Unit* owner, int id, int* pos, int param_6, int param_7)
{
    if (remove) {
        Order* first = owner->list;
        for (Order* node = first; node != 0; node = node->next) {
            if (Matches(node, kind, id, pos)) {
                RemoveNode(owner, node, first);
                return;
            }
        }
    }
    AddOrder(kind, remove, owner, id, pos, param_6, param_7);
}

// FUNCTION: 0x43b0b0
void __stdcall AdjustBuildCount(int kind, Unit* owner, int id, int amount)
{
    Order* node;
    if (amount > 0) {
        if (g_missionOrderTableBegin[kind & 0xff].flags & 0x40000)
            node = owner->list2;
        else
            node = owner->list;
        while (node != 0 && node->next != 0) {
            node = node->next;
        }
        if (node != 0 && node->kind.index == (unsigned char)kind && node->id == id) {
            node->count += amount;
            return;
        }
        AddOrder((unsigned char)kind, 1, owner, 0, 0, id, amount);
        return;
    }
    for (;;) {
        Order* found = 0;
        if (g_missionOrderTableBegin[kind & 0xff].flags & 0x40000)
            node = owner->list2;
        else
            node = owner->list;
        for (; node != 0; node = node->next) {
            if (node->kind.index == (unsigned char)kind && node->id == id)
                found = node;
        }
        if (found == 0)
            break;
        if (found->count > -amount) {
            found->count += amount;
            break;
        }
        Order* first = owner->list;
        amount += found->count;
        Ref<Order>* link = &owner->list;
        if (found->flags_42 & 0x40000)
            link = &owner->list2;
        for (node = *link; node != 0; node = node->next) {
            if (node == found) {
                *link = found->next;
                if (found != first)
                    found->flags_42 |= 0x10000;
                delete found;
                break;
            }
            link = &node->next;
        }
    }
}

static inline void Insert(Unit* unit, Order* cmd, Order* before)
{
    unsigned int which = cmd->flags_42 & 0x40000;
    Order* last = which ? unit->list2 : unit->list;
    Ref<Order>* link = which ? &unit->list2 : &unit->list;
    while (*link != before) {
        link = &(*link)->next;
    }
    *link = cmd;
    cmd->unit = unit;
    cmd->next = before;
    if (before != 0) {
        cmd->flags_42 |= before->flags_42 & 0x4000;
    }
}

// FUNCTION: 0x43b1f0
int __stdcall IssueAttackOrder(Unit* unit, Unit* target, int param_3)
{
    if (unit == target)
        return 0;
    if (!(unit->flags & 0xc0000) && !param_3)
        return 0;
    if (!(unit->flags & 0x300000) && !param_3)
        return 0;
    MissionType kind = GetOrderType(3, unit, target, 0);
    if (!kind.index)
        return 0;
    if ((unit->flags & 0xc0000) == 0x40000 && !param_3) {
        MissionType kind2 = GetOrderType(2, unit, 0, &unit->pos);
        Order* cmd = new (kSim) Order(kind2, 0, &unit->pos, 0, 0, 0);
        Insert(unit, cmd, (cmd->flags_42 & 0x40000) ? unit->list2 : unit->list);
        Order* cmd2 = new (kSim) Order(kind, target, 0, 0, 0, ((UnitType_0043cd20*)unit->def.get())->maneuverleashlength);
        cmd2->start.x = PosXWhole(unit);
        cmd2->start.y = PosZWhole(unit);
        Insert(unit, cmd2, (cmd2->flags_42 & 0x40000) ? unit->list2 : unit->list);
    } else {
        Order* cmd3 = new (kSim) Order(kind, target, 0, 0, 0, 0);
        Insert(unit, cmd3, (cmd3->flags_42 & 0x40000) ? unit->list2 : unit->list);
    }
    return 1;
}

// FUNCTION: 0x43b400
int __stdcall IssueRepairOrder(Unit* unit, Unit* target, int param_3)
{
    MissionType kind = GetOrderType(8, unit, target, 0);
    if (!kind.index)
        return 0;
    unsigned int f = unit->flags & 0xc0000;
    if (f == 0 && !param_3) {
        MissionType kind2 = GetOrderType(2, unit, 0, &unit->pos);
        Order* cmd = new (kSim) Order(kind2, 0, &unit->pos, 0, 0, 0);
        Insert(unit, cmd, (cmd->flags_42 & 0x40000) ? unit->list2 : unit->list);
        Order* cmd2 = new (kSim) Order(kind, target, 0, 0, 0, ((UnitType_0043cd20*)unit->def.get())->range);
        cmd2->start.x = PosXWhole(unit);
        cmd2->start.y = PosZWhole(unit);
        Insert(unit, cmd2, (cmd2->flags_42 & 0x40000) ? unit->list2 : unit->list);
        return 1;
    } else if (f == 0x40000 && !param_3) {
        MissionType kind2 = GetOrderType(2, unit, 0, &unit->pos);
        Order* cmd = new (kSim) Order(kind2, 0, &unit->pos, 0, 0, 0);
        Insert(unit, cmd, (cmd->flags_42 & 0x40000) ? unit->list2 : unit->list);
        Order* cmd2 = new (kSim) Order(kind, target, 0, 0, 0, ((UnitType_0043cd20*)unit->def.get())->maneuverleashlength);
        cmd2->start.x = PosXWhole(unit);
        cmd2->start.y = PosZWhole(unit);
        Insert(unit, cmd2, (cmd2->flags_42 & 0x40000) ? unit->list2 : unit->list);
        return 1;
    } else if (f == 0x80000 && !param_3) {
        Order* cmd3 = new (kSim) Order(kind, target, 0, 0, 0, 0);
        Insert(unit, cmd3, (cmd3->flags_42 & 0x40000) ? unit->list2 : unit->list);
        return 1;
    } else {
        return 0;
    }
}

// FUNCTION: 0x43b700
int __stdcall FindBestTargetIfFireAtWill(Unit* unit)
{
    if (unit->fireOrder == 2)
        return FindWeaponTarget(unit, 0, 0);
    return 0;
}

// FUNCTION: 0x43b730
void __stdcall EnqueueOrderType(Unit* p, unsigned char type)
{
    Order* child = new (kSim) Order(type, 0, 0, 0, 0, 0);
    child->flags_42 |= 0x4000;
    InsertBefore(p, child, (child->flags_42 & 0x40000) ? p->list2 : p->list);
}

void __stdcall MoveOrderToTail(Unit* owner, Order* node)
{
    Ref<Order>* pp = &owner->list;
    Order* n = *pp;
    for (; n != node; n = n->next) {
        pp = &n->next;
    }
    n = node->next;
    *pp = n;
    for (; n != 0; n = n->next) {
        pp = &n->next;
    }
    *pp = node;
    node->next = 0;
}

static void InsertCommand(Unit* p, unsigned char type)
{
    Order* child = new (kSim) Order(type, 0, 0, 0, 0, 0);
    child->flags_42 |= 0x4000;
    InsertOrderBefore(p, child, (child->flags_42 & 0x40000) ? p->list2 : p->list);
}

static void Wait_0043b7c0(Order* node, int n)
{
    unsigned int when = RandomInt(n) + 0x1e;
    node->flags |= 1;
    node->wakeFrame = (unsigned int)g_game->ticks + when;
}

static void ClearTargets_0043b7c0(Unit* unit)
{
    for (char i = 0; i < 3; i++)
        g_seams.targeting->ClearWeaponTarget(unit, i);
}

// FUNCTION: 0x43b7c0
void __stdcall RunOrders(Unit* unit)
{
    Order* node;
    for (;;) {
        node = unit->list;
        if (node == 0)
            break;
        if ((unsigned int)g_game->ticks >= node->wakeFrame) {
            node->wakeFrame = 0xffffffff;
            node->subFlags |= 1;
        }
        unsigned int pending = (node->subFlags | (unsigned short)unit->netDirtyFlags) & node->flags;
        if (node->flags != 0 && pending == 0)
            return;
        unit->netDirtyFlags &= ~pending;
        node->subFlags &= ~pending;
        node->flags = 0;
        if (pending & 0x10000)
            ClearTargets_0043b7c0(unit);
        switch (g_missionOrderTableBegin[node->kind.index].notify(node->unit, node, pending)) {
        case 3:
            Wait_0043b7c0(node, 0xf);
            break;
        case 1:
            node->state++;
            break;
        case 0:
            node->state = 0;
            break;
        case 2:
        case 4:
            break;
        case 5:
        case 8:
            DeleteOrder(unit, node);
            break;
        case 9:
            node->flags_42 |= 0x800000;
            if (node->next != 0) {
                DeleteOrder(unit, node);
            } else {
                node->state = 0;
                Wait_0043b7c0(node, 0x1e);
            }
            break;
        case 6:
            MoveOrderToTail(unit, node);
            break;
        case 7:
            DeleteOrders(unit, 1);
            return;
        default:
            DeleteOrders(unit, 1);
            return;
        }
    }
    if (unit->player->active == 0)
        return;
    char state = unit->player->type;
    if (state != 1 && state != 2)
        return;
    if (((UnitType_0043cd20*)unit->def.get())->defaultmissiontype == 0)
        return;
    InsertCommand(unit, ((UnitType_0043cd20*)unit->def.get())->defaultmissiontype);
}

static void RemoveAndDelete(Unit* p, Order* child)
{
    Order* first = p->list;
    Ref<Order>* link = (child->flags_42 & 0x40000) ? &p->list2 : &p->list;
    Order* node = *link;
    while (node != 0) {
        if (node == child) {
            *link = child->next;
            if (child != first)
                child->flags_42 |= 0x10000;
            delete child;
            break;
        }
        link = &node->next;
        node = *link;
    }
}

// FUNCTION: 0x43bad0
void __stdcall RunSecondaryOrders(Unit* p)
{
    Order* child = p->list2;
    while (child != 0) {
        if (child->flags == 0 || (unsigned int)g_game->ticks >= child->wakeFrame) {
            child->flags = 0;
            switch (g_missionOrderTableBegin[child->kind.index].notify(child->unit, child, 0)) {
            case 3: {
                unsigned int when = RandomInt(0xf) + 0x1e;
                child->flags |= 1;
                child->wakeFrame = (unsigned int)g_game->ticks + when;
                break;
            }
            case 1:
                child->state++;
                break;
            case 0:
                child->state = 0;
                break;
            case 2:
            case 4:
                break;
            case 5:
            case 8:
            case 9:
                RemoveAndDelete(p, child);
                break;
            case 6:
            case 7:
                RemoveAndDelete(p, child);
                return;
            default:
                RemoveAndDelete(p, child);
                break;
            }
            child = p->list2;
        } else {
            child = child->next;
            continue;
        }
    }
}

typedef Vec_0043c390 SimView_g_missionOrderTableVec;
#undef g_missionOrderTableVec
#define g_missionOrderTableVec (*reinterpret_cast<SimView_g_missionOrderTableVec*>(&Sim().g_missionOrderTableVec))

// FUNCTION: 0x43c940
inline void __stdcall InsertShiftOrderTypes(Elem_0043c390* _L, Elem_0043c390 _V, Pred_0043c390 _P)
{
    for (Elem_0043c390* _M = _L; _P(_V, *--_M); _L = _M)
        *_L = *_M;
    *_L = _V;
}

// FUNCTION: 0x43c990
inline void __stdcall InsertionSortOrderTypes(Elem_0043c390* _F, Elem_0043c390* _L, Pred_0043c390 _P,
                                   Elem_0043c390*)
{
    if (_F != _L)
        for (Elem_0043c390* _M = _F; ++_M != _L; ) {
            Elem_0043c390 _V = *_M;
            if (!_P(_V, *_F))
                InsertShiftOrderTypes(_M, _V, _P);
            else {
                std::copy_backward(_F, _M, _M + 1);
                *_F = _V;
            }
        }
}

inline void _Insertion_sort_0043bc90(Elem_0043c390* _F, Elem_0043c390* _L, Pred_0043c390 _P)
{
    InsertionSortOrderTypes(_F, _L, _P, std::_Val_type(_F));
}

// FUNCTION: 0x43ca70
inline Elem_0043c390 __stdcall MedianOrderTypes(Elem_0043c390 _X, Elem_0043c390 _Y, Elem_0043c390 _Z,
                                            Pred_0043c390 _P)
{
    if (_P(_X, _Y))
        return (_P(_Y, _Z) ? _Y : _P(_X, _Z) ? _Z : _X);
    else
        return (_P(_X, _Z) ? _X : _P(_Y, _Z) ? _Z : _Y);
}

// FUNCTION: 0x43cb20
inline Elem_0043c390* __stdcall PartitionOrderTypes(Elem_0043c390* _F, Elem_0043c390* _L,
                                             Elem_0043c390 _Piv, Pred_0043c390 _P)
{
    for (; ; ++_F) {
        for (; _P(*_F, _Piv); ++_F)
            ;
        for (; _P(_Piv, *--_L); )
            ;
        if (_L <= _F)
            return (_F);
        std::iter_swap(_F, _L);
    }
}

// FUNCTION: 0x43c720
inline void __stdcall SortOrderTypes(Elem_0043c390* _F, Elem_0043c390* _L, Pred_0043c390 _P,
                                   Elem_0043c390*)
{
    for (; std::_SORT_MAX < _L - _F; ) {
        Elem_0043c390* _M = PartitionOrderTypes(_F, _L, MedianOrderTypes(Elem_0043c390(*_F),
            Elem_0043c390(*(_F + (_L - _F) / 2)), Elem_0043c390(*(_L - 1)), _P), _P);
        if (_L - _M <= _M - _F)
            SortOrderTypes(_M, _L, _P, std::_Val_type(_F)), _L = _M;
        else
            SortOrderTypes(_F, _M, _P, std::_Val_type(_F)), _F = _M;
    }
}

inline void _Sort_0_0043bc90(Elem_0043c390* _F, Elem_0043c390* _L, Pred_0043c390 _P,
                             Elem_0043c390*)
{
    if (_L - _F <= std::_SORT_MAX)
        _Insertion_sort_0043bc90(_F, _L, _P);
    else {
        SortOrderTypes(_F, _L, _P, (Elem_0043c390*)0);
        _Insertion_sort_0043bc90(_F, _F + std::_SORT_MAX, _P);
        for (_F += std::_SORT_MAX; _F != _L; ++_F)
            InsertShiftOrderTypes(_F, Elem_0043c390(*_F), _P);
    }
}

inline void sort_0043bc90(Elem_0043c390* _F, Elem_0043c390* _L, Pred_0043c390 _P)
{
    _Sort_0_0043bc90(_F, _L, _P, std::_Val_type(_F));
}

extern int DAT_0051234c;

// FUNCTION: 0x43bc90
void __stdcall RegisterOrderTypes(Elem_0043c390* from, int count)
{
    int sz = g_missionOrderTableVec.size();
    g_missionOrderTableVec.reserve(sz + count);
    std::copy(from, from + count, std::back_inserter(g_missionOrderTableVec));
    sort_0043bc90(g_missionOrderTableVec.begin(), g_missionOrderTableVec.end(), CompareOrderTypeNames);
}

// FUNCTION: 0x43c020
int __stdcall CompareOrderTypeNames(const Elem_0043c390& a, const Elem_0043c390& b)
{
    return _strcmpi(a.name, b.name) < 0 ? 1 : 0;
}

// FUNCTION: 0x43c050
void RegisterAllOrderTypes()
{
    RegisterOrderTypes(g_readyOrder, 1);
    RegisterGroundOrders();
    RegisterVtolOrders();
    g_seams.ai->RegisterAICommands();
    RegisterUnitOrders();
}

// FUNCTION: 0x43c350
void ClearOrderTypeTable()
{
    g_missionOrderTableEnd = (int)g_missionOrderTableBegin;
}

// FUNCTION: 0x43c360
int MissionOrderTable::GetCount(void)
{
    return first == 0 ? 0 : (last - first) / 0x19;
}

typedef void (Vec_0043c390::*DestroyFn_0043c390)(Vec_0043c390::iterator, Vec_0043c390::iterator);

struct Access_0043c390 : Vec_0043c390 {
    static DestroyFn_0043c390 fn;
};

// FUNCTION: 0x43c390 ?_Destroy@?$vector@UElem_0043c390@@V?$allocator@UElem_0043c390@@@std@@@std@@IAEXPAUElem_0043c390@@0@Z
DestroyFn_0043c390 Access_0043c390::fn = &Access_0043c390::_Destroy;

template class std::vector<Elem_0043c390>;

// FUNCTION: 0x43c3a0 ?insert@?$vector@UElem_0043c390@@V?$allocator@UElem_0043c390@@@std@@@std@@QAEXPAUElem_0043c390@@IABU3@@Z

typedef int (__stdcall* Pred_0043c6b0)(const Elem_0043c390& entry, char* name);

// FUNCTION: 0x43c6b0
Elem_0043c390* __stdcall LowerBoundOrderTypes(Elem_0043c390* first, Elem_0043c390* last,
                                      char* const& value, Pred_0043c6b0 pred, int*)
{
    int n = 0;
    n += last - first;
    for (; 0 < n; ) {
        int n2 = n / 2;
        Elem_0043c390* m = first;
        m += n2;
        if (pred(*m, value))
            first = ++m, n -= n2 + 1;
        else
            n = n2;
    }
    return first;
}

// FUNCTION: 0x43cbb0
void UnitMotion::ApplyClampedTurnDelta(Unit* unit, short amount)
{
    if (amount != 0) {
        unsigned short max = ((UnitType_0043cd20*)unit->def.get())->max_turn;
        if (amount >= max)
            turn = max;
        else if (amount <= -max)
            turn = -max;
        else
            turn = amount;
        unit->angles.heading += turn;
        unit->moved = 1;
    } else {
        turn = 0;
    }
}

static inline void ClampToZero(int& value)
{
    if (value < 0)
        value = 0;
}

static inline void AddFixed(int& v, float f)
{
    v += (int)((double)f * 65536.0);
}

static inline Vec3 Offset(short angle, int distance)
{
    Vec3 v;
    v.x = -ScaledSin(angle, distance);
    v.y = 0;
    v.z = -ScaledCos(angle, distance);
    return v;
}

static inline void ClampToCell(Vec3& pos, PointInit_0043a1f0 cell, PointInit_0043a1f0 draft)
{
    int cx = (draft.x + cell.x * 2) << 19;
    int cz = (draft.y + cell.y * 2) << 19;
    if (pos.x > cx + 0x7ffff)
        pos.x = cx + 0x7ffff;
    else if (pos.x < cx - 0x7ffff)
        pos.x = cx - 0x7ffff;
    if (pos.z > cz + 0x7ffff)
        pos.z = cz + 0x7ffff;
    else if (pos.z < cz - 0x7ffff)
        pos.z = cz - 0x7ffff;
}

// FUNCTION: 0x43cc20
void UnitMotion::UpdateVelocityFromHeading(Unit* unit, int amount)
{
    speed = speed + amount;
    ClampToZero(speed);

    int idx = unit->angles.pitch >> 11;
    if (idx < -5)
        idx = -5;
    if (idx > 5)
        idx = 5;

    int range = (int)(((__int64)(g_slopeSpeedFactor[idx] << 16) * ((UnitType_0043cd20*)unit->def.get())->maxvelocity) >> 16);
    range = (int)(((__int64)range << 16) / 0x640000);
    if (unit->yWhole < g_game->seaLevel && !(((UnitType_0043cd20*)unit->def.get())->flags1 & 0x81000))
        range = (int)(((__int64)range * 0x8000) >> 16);
    if (speed > range)
        speed = range;

    int dist = speed;
    unsigned short angle = unit->angles.heading;
    Vec3 v;
    v.x = -ScaledSin(angle, dist);
    v.y = 0;
    v.z = -ScaledCos(angle, dist);
    velocity = v;
}

// FUNCTION: 0x43cd20
void UnitMotion::SteerGroundUnit(Unit* unit)
{
    if (obj->v5() == 0) {
        turn = 0;
        const int& amount = -((UnitType_0043cd20*)unit->def.get())->brakerate;
        UpdateVelocityFromHeading(unit, amount);
        return;
    }

    Vec3 p[3];
    obj->v3(p, 0, 3);

    Vec3* ppos = &unit->pos;
    Vec3 d = PosMinus(p[1], *ppos);
    int gap1 = (int)_hypot(d.x, d.z);

    int dz;
    if (gap1 > 0x500000) {
        int dx = p[1].x - p[0].x;
        dz = p[1].z - p[0].z;
        int len = (int)_hypot(dx, dz);
        if (len >= 0x10000) {
            int ndx = (int)(((__int64)dx << 16) / len);
            int ndz = (int)(((__int64)dz << 16) / len);
            int back = gap1 - 0x500000;
            if (back > len)
                back = len;
            p[1].x -= (int)(((__int64)ndx * back) >> 16);
            p[1].z -= (int)(((__int64)ndz * back) >> 16);
        }
    }

    int az = p[1].z - unit->pos.z;
    int ax = p[1].x - unit->pos.x;
    int d1 = (int)(((__int64)ax * ax) >> 32) + (int)(((__int64)az * az) >> 32);

    short ang = (short)GetHeadingBetween(ppos, &p[1]);
    short diff = ang - unit->angles.heading;
    int sdiff = diff;
    int adiff = abs(sdiff);

    int bz = p[2].z - unit->pos.z;
    int bx = p[2].x - unit->pos.x;
    int d2 = (int)(((__int64)bx * bx) >> 32) + (int)(((__int64)bz * bz) >> 32);

    if (diff != 0) {
        unsigned short max = ((UnitType_0043cd20*)unit->def.get())->max_turn;
        if (sdiff >= max)
            turn = max;
        else if (sdiff <= -max)
            turn = -max;
        else
            turn = diff;
        unit->angles.heading += turn;
        unit->moved = 1;
    } else {
        turn = 0;
    }

    int turned = (int)((((__int64)(adiff & 0xffff) * (__int64)speed)
                        / ((UnitType_0043cd20*)unit->def.get())->max_turn));
    int rate = ((UnitType_0043cd20*)unit->def.get())->brakerate;
    int spd = speed;
    int t = (int)(((__int64)spd * spd) >> 16);
    int q = (int)(((__int64)t << 16) / (2 * rate));
    int r = (int)(((__int64)q * q) >> 32);
    int lim = (int)(((__int64)turned * turned) >> 32) * 4;

    if (d1 > lim && d2 > r)
        UpdateVelocityFromHeading(unit, ((UnitType_0043cd20*)unit->def.get())->acceleration);
    else
        UpdateVelocityFromHeading(unit, -rate);
}

#pragma auto_inline(off)
// FUNCTION: 0x43d0d0
void UnitMotion::ApplyBankAndPitch(Unit* owner, Vec3* v)
{
    Vec3Scale(&p2, 0xf333);
    Vec3Add(&p2, v);
    int xz[2];
    xz[0] = p2.x;
    xz[1] = p2.z;
    RotatePoint2D(owner->angles.heading, xz);
    int n = (int)(((__int64)g_game->gravity << 16) / 0xccd);
    owner->angles.bank = (short)AngleOfVector((int)(((__int64)((UnitType_0043cd20*)owner->def.get())->bankscale * -xz[0]) >> 16), n);
    owner->angles.pitch = (short)AngleOfVector((int)(((__int64)((UnitType_0043cd20*)owner->def.get())->pitchscale * -xz[0]) >> 16), n);
}
#pragma auto_inline(on)

// FUNCTION: 0x43d210
void UnitMotion::SetFlightMode(Unit* owner, int newState)
{
    if (mode != newState) {
        if (newState == 1) {
            speed = 0;
            Vec3 zero = OrderVec3(0, 0, 0);
            velocity = zero;
            ApplyBankAndPitch(owner, &zero);
            g_seams.units->SetStateBits(owner, 1, 0);
        } else {
            g_seams.units->SetStateBits(owner, 1, 1);
        }
        mode = newState;
    }
}

// FUNCTION: 0x43d290
void UnitMotion::SteerAircraft(Unit* unit) {
    if (mode != 2) {
        velocity = OrderVec3(0, 0, 0);
        speed = 0;
        turn = 0;
        return;
    }

    Vec3 old = velocity;
    Vec3 a;
    Vec3 b;
    short heading;
    obj->v4(&a, &b, &heading);

    UnitType_0043cd20* type = (UnitType_0043cd20*)unit->def.get();
    const float eps = 1.52587890625e-05f;

    float f18 = (float)type->acceleration * eps;
    int q = (int)(((__int64)type->acceleration << 16) / type->maxvelocity);
    int scale = 0x10000 - q;
    Vec3Scale(&velocity, scale);

    float dist = (float)_hypot(velocity.x, velocity.z) * eps;
    float maxd = (float)((UnitType_0043cd20*)unit->def.get())->brakerate * eps;
    if (dist > maxd) {
        int f = (int)((double)maxd / dist * 65536.0);
        MulFixed(velocity.x, f);
        MulFixed(velocity.z, f);
        int g = (int)(((double)dist - maxd) * 65536.0);
        velocity += Offset(unit->angles.heading, g);
    }

    Vec3 da = PosMinus(unit->pos, a);
    Vec3 db = PosMinus(velocity, b);

    if ((int)unit->spatialBucket.get() != (int)g_game->overflowBucket.get()) {
        int lim;
        if ((speed & -4) < 0x40000)
            lim = 0x10000;
        else
            lim = speed >> 2;
        if (da.y <= -lim)
            velocity.y = lim;
        else if (da.y >= lim)
            velocity.y = -lim;
        else
            velocity.y = -da.y;
    }

#ifdef __clang__
    double hd = _hypot(da.x, da.z) * eps;
#else
    float hd = (float)_hypot(da.x, da.z) * eps;
#endif
    short d = (short)(heading - unit->angles.heading);
    if (d != 0) {
        unsigned short max = ((UnitType_0043cd20*)unit->def.get())->max_turn;
        if (d >= (int)max)
            turn = max;
        else if (d <= -(int)max)
            turn = (short)-max;
        else
            turn = d;
        unit->angles.heading = (short)(unit->angles.heading + turn);
        unit->moved = 1;
    } else {
        turn = 0;
    }

    if (hd < 8.0f)
        hd = 8.0f;

#ifdef __clang__
    double k = -sqrt((2.0 * f18) / hd);
    double vxd = (double)da.x * k * eps - (double)db.x * eps;
    double vzd = (double)da.z * k * eps - (double)db.z * eps;
    float vx = vxd;
    float vz = vzd;
    double vxs = vx;
    double mag = _hypot(vxd, vzd);
    if (mag > f18) {
        vxs = vx * (f18 / mag);
        vz = vz * (f18 / mag);
    }
#else
    float k = -sqrt((2.0f * f18) / hd);
    float vx = ((float)da.x) * k * eps - (float)db.x * eps;
    float vz = (float)da.z * k * eps - (float)db.z * eps;
    float mag = (float)_hypot(vx, vz);
    if (mag > f18) {
        vx = vx * (f18 / mag);
        vz = vz * (f18 / mag);
    }

#endif

#ifdef __clang__
    velocity.x += (int)(vxs * 65536.0);
#else
    AddFixed(velocity.x, vx);
#endif
    AddFixed(velocity.z, vz);
    speed = Vec3Length(&velocity);

    {
        Vec3 delta = PosMinus(velocity, old);
        ApplyBankAndPitch(unit, &delta);
    }
}

// FUNCTION: 0x43d6d0
void UnitMotion::UpdatePosition(Unit* u)
{
    if (u->carrier != 0) {
        Vec3 v;
        v = g_seams.units->GetPiecePosition(u->carrier, (signed char)u->transportPiece);
        if (((UnitType_0043cd20*)u->def.get())->b19) {
            *(Fixed_0043d6d0*)&v.y = MAXM_0043d6d0(*(Fixed_0043d6d0*)&v.y, MakeFixed_0043d6d0(((UnitType_0043cd20*)u->def.get())->draft * 0xffff + g_game->seaLevel));
        }
        g_seams.units->SetUnitPosition(u, v, mode);
        Angles16 o = g_seams.units->GetPieceAngles(u->carrier, (signed char)u->transportPiece);
        u->angles = o;
        if (((Path_0043d6d0*)u->carrier.get())->motion != 0) {
            speed = ((Path_0043d6d0*)u->carrier.get())->motion->speed;
            velocity = ((Path_0043d6d0*)u->carrier.get())->motion->velocity;
        } else {
            speed = 0;
            Vec3 zero = OrderVec3(0, 0, 0);
            velocity = zero;
        }
        u->moved = 0;
        return;
    }

    Vec3 pos;
    pos = PosPlus(u->pos, velocity);
    int m = mode;
    if (pos.x == u->pos.x && pos.z == u->pos.z && pos.y == u->pos.y && m == u->mode)
        return;

    lastMoveTick = g_game->ticks;
    PointInit_0043a1f0 draft = *(PointInit_0043a1f0*)&u->footprint;
    PointInit_0043a1f0 cell;
    cell.x = (pos.x - draft.x * 0x80000 + 0x80000) >> 20;
    cell.y = (pos.z - draft.y * 0x80000 + 0x80000) >> 20;
    if (cell.x == u->cell.x && cell.y == u->cell.y && m == u->mode) {
        u->pos = pos;
        u->moved = 1;
        return;
    }

    if (((Target_0043d6d0*)u->player.get())->active != 0) {
        if (((Target_0043d6d0*)u->player.get())->type == 1 || ((Target_0043d6d0*)u->player.get())->type == 2)
            flag = g_seams.map->CanPlaceUnitFootprint((UnitDef*)u->def, (unsigned short)u->id, *(Point16*)&cell, m) == 0;
    }

    if (flag) {
        ClampToCell(pos, *(PointInit_0043a1f0*)&u->cell, *(PointInit_0043a1f0*)&u->footprint);

        if (speed > (((UnitType_0043cd20*)u->def.get())->maxvelocity / 2)) {
            int half = ((UnitType_0043cd20*)u->def.get())->maxvelocity / 2;
            speed = half;
            unsigned short angle = u->angles.heading;
            Vec3 vec;
            vec.x = -ScaledSin(angle, half);
            vec.y = 0;
            int z = -ScaledCos(angle, half);
            vec.z = z;
            velocity = vec;
        }
        u->pos = pos;
        u->moved = 1;
        return;
    }

    g_seams.map->RemoveUnitFromMap(u);
    u->pos = pos;
    *(Point16*)&u->cell = cell;
    u->mode = (short)m;
    g_seams.map->AddUnitToMap(u);
    u->moved = 1;
    g_seams.los_radar->UpdateUnitLineOfSight(u);
}

// FUNCTION: 0x43da70
void UnitMotion::UpdateMoveRate(Unit* unit)
{
    int rate;
    if ((flags & 4) == 0 && unit->carrier == 0
        && (speed != 0 || turn != 0)) {
        if (speed <= ((UnitType_0043cd20*)unit->def.get())->moverate1) {
            rate = 1;
        } else {
            rate = 2 + (speed > ((UnitType_0043cd20*)unit->def.get())->moverate2);
        }
    } else {
        rate = 0;
    }
    if (rate == (int)((unit->flags >> 2) & 3))
        return;
    if (rate == 0) {
        g_seams.units->StartScript((CobScript*)unit->script.get(), "StopMoving", 0, 1);
    } else if ((unit->flags & 0xc) == 0) {
        g_seams.units->StartScript((CobScript*)unit->script.get(), "StartMoving", 0, 1);
    }
    switch (rate) {
    case 1:
        g_seams.units->StartScript((CobScript*)unit->script.get(), "MoveRate1", 0, 1);
        break;
    case 2:
        g_seams.units->StartScript((CobScript*)unit->script.get(), "MoveRate2", 0, 1);
        break;
    case 3:
        g_seams.units->StartScript((CobScript*)unit->script.get(), "MoveRate3", 0, 1);
        break;
    }
    unit->flags = (unit->flags & 0xfffffff3) | ((rate & 3) << 2);
}

// FUNCTION: 0x43dc00
UnitMotion::UnitMotion(Unit* unit)
{
    velocity = OrderVec3(0, 0, 0);
    speed = 0;
    turn = 0;
    pathLockStamp = 0;
    p2 = OrderVec3(0, 0, 0);
    mode = 1;
    flag = 0;
    movementClass = (int)((UnitType_0043cd20*)unit->def.get())->movementclass.get();
    if (((Target_0043d6d0*)unit->player.get())->active != 0 && ((Target_0043d6d0*)unit->player.get())->type == 3) {
        if (((UnitType_0043cd20*)unit->def.get())->flag_800)
            obj = (Iface_0043dd20*)new (kSim) PackedPosGoal(unit);
        else
            obj = (Iface_0043dd20*)new (kSim) PatrolGoal(unit);
    } else {
        if (((UnitType_0043cd20*)unit->def.get())->flag_800)
            obj = (Iface_0043dd20*)new (kSim) LiteGoal(unit);
        else
            obj = (Iface_0043dd20*)new (kSim) AiSearchGoal(unit);
    }
}

// FUNCTION: 0x43dd10
void UnitMotion::DestroyObject()
{
    if (obj != 0) {
        obj->v0(1);
    }
}

// FUNCTION: 0x43dd20
void UnitMotion::UpdateMotion(Unit* u)
{
    obj->v2();
    if (((UnitType_0043cd20*)u->def.get())->flag_800)
        SteerAircraft(u);
    else
        SteerGroundUnit(u);
    UpdatePosition(u);
    UpdateMoveRate(u);
    g_seams.units->UpdateSfxOccupy((UnitMotion*)this, 0, u);
}

// FUNCTION: 0x43dd70
void UnitMotion::SaveMotion(Unit* info, HapiBank* file)
{
    char name[32];
    Record_0043dd70 hdr;
    hdr.velocity = velocity;
    hdr.p2 = p2;
    hdr.speed = speed;
    hdr.turn = turn;
    hdr.pathLockStamp = pathLockStamp;
    hdr.mode = mode;
    hdr.flag = flag;
    sprintf(name, "u%04xmob", info->id);
    file->OpenNamedBox(name);
    file->SeekBox(0);
    file->WriteBox(&hdr, 0x23);
}

// FUNCTION: 0x43de30
void UnitMotion::LoadMotion(Unit* unit, HapiBank* file)
{
    char name[32];
    Record_0043dd70 rec;
    sprintf(name, "u%04xmob", unit->id);
    file->OpenNamedBox(name);
    file->SeekBox(0);
    file->ReadBox(&rec, 0x23);
    velocity = rec.velocity;
    p2 = rec.p2;
    speed = rec.speed;
    turn = rec.turn;
    pathLockStamp = rec.pathLockStamp;
    mode = rec.mode;
    flag = rec.flag;
}
