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

#include "../files/files_api.h"

#pragma pack(push, 1)

struct Vec3FromInts : Vec3 {
    Vec3FromInts(int a, int b, int c) { x = a; y = b; z = c; }
};

struct Point16FromShorts : Point16 {
    Point16FromShorts() {}
    Point16FromShorts(short a, short b) { x = a; y = b; }
};

#include "../util/angles.h"
#include "../../interfaces/seams.h"
#include "../game/map_events.h"
#include "../../interfaces/map.h"
#include "../../interfaces/orders.h"
#include "../../interfaces/services.h"
#include "../game/targeting_events.h"
#include "../game/los_radar_events.h"

#include "../network/player.h"

#include "../movement/unit_def_move_view.h"

#include "../scripts/cob_script.h"

struct Unit;

#include "order.h"

#include "../weapons/weapon_def.h"

#include "../weapons/unit_weapon_slot.h"

#include "../game/game.h"

#include "../units/unit.h"
#include "../units/unit_ref.h"

static inline short PosXWhole(Unit* unit) { return *(short*)((char*)&unit->pos + 2); }
static inline short PosZWhole(Unit* unit) { return *(short*)((char*)&unit->pos + 0xa); }

#include "order_type_entry.h"
#include "../state/sim_globals.h"


#include "../network/net_stats.h"
#include "../movement/order_goals.h"
#include "../movement/ai_search_goal.h"
#include "../movement/unit_motion.h"
#pragma pack(pop)
#include "unit_orders.h"
#include "order_list.h"

#include "../../link/globals.h"

void __stdcall DeleteOrder(Unit* owner, Order* node);

extern int __cdecl _strcmpi(const char*, const char*);


#include "../ingame/markers.h"
void __stdcall DrawUnitRangeRings(void* a, void* b, Order* e, Vec3* p, int c);
extern OrderTableEntry g_readyOrder[];
extern signed char g_slopeSpeedFactor[];
#include "../util/math.h"
void __cdecl RotatePoint2D(short angle, int* xz);
int __cdecl AngleOfVector(int x, int z);
int __stdcall GetHeadingBetween(Vec3* from, Vec3* to);
struct UnitOrderTypeView;
struct FixedPosView;
#include "order_dispatch.h"
void RegisterUnitOrders();
extern int DAT_0051234c;

// FUNCTION: 0x439b30
void __stdcall DrawOrderOverlays(Unit* unit, unsigned int mask, void* obj,
                            void* sel, int flag)
{
    Vec3 base = unit->pos;
    Vec3 pos = unit->pos;
    bool done = false;
    for (Order* e = unit->list; e != 0; e = e->next) {
        if (g_missionOrderTableVec[e->kind.index].drawMask & mask & 1) {
            pos = base;
            DrawBuildFootprint(obj, (View *)sel, e, (Pos *)&pos, flag);
        }
        if (g_missionOrderTableVec[e->kind.index].drawMask & mask & 2) {
            pos = base;
            DrawPathAnim(obj, (View *)sel, e, (Pos *)&pos, flag);
        }
        if (g_missionOrderTableVec[e->kind.index].drawMask & mask & 4) {
            pos = base;
            DrawOrderRangeRing(obj, (View *)sel, e, (Pos *)&pos, flag);
        }
        if (g_missionOrderTableVec[e->kind.index].drawMask & mask & 8) {
            pos = base;
            DrawWeaponCoverage(obj, (View *)sel, e, (Pos *)&pos, flag);
        }
        if (g_missionOrderTableVec[e->kind.index].drawMask & mask & 0x10) {
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
const char* __stdcall GetOrderName(Unit* obj)
{
    if (obj && obj->list) {
        return g_missionOrderTableVec[obj->list->kind.index].value;
    }
    return g_missionOrderTableVec[0].value;
}

// FUNCTION: 0x439e30
Order* __stdcall FindOrderByType(Unit* obj, unsigned char kind)
{
    Order* n;
    if (g_missionOrderTableVec[kind].flags & ORDER_FLAG_SECOND_LIST)
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
    int val = g_game->ticks;
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
    Ref<Order>* link = (node->flags_42 & ORDER_FLAG_SECOND_LIST) ? &owner->list2 : &owner->list;
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
    Ref<Order>* link = (child->flags_42 & ORDER_FLAG_SECOND_LIST) ? &p->list2 : &p->list;
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
    : kind(MissionTypeOf(k)), link(), start(Point16FromShorts(0, 0)), cached(Point16FromShorts(0, 0)),
      id(a), count(b), progress(c), created((unsigned int)g_game->ticks)
{
    g_seams.units->UnitRefInit((UnitRef*)&link, o, nullptr);
    link.SetValue(this);
    state = 0;
    flags = 0;
    subFlags = 0;
    wakeFrame = -1;
    pos = p ? *p : Vec3FromInts(0, 0, 0);
    flags_42 = g_missionOrderTableVec[k & 0xff].flags;
    unit = 0;
    next = 0;
    attached = 0;
    if (o == 0)
        flags_42 &= ~ORDER_FLAG_TARGET;
    if (p == 0)
        flags_42 &= ~ORDER_FLAG_POSITION;
    if (!(flags_42 & ORDER_FLAG_TARGET))
        g_seams.units->UnitRefSetUnit((UnitRef*)&link, 0);
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
        file->SetStringItem(key, ((UnitDefMoveView *)g_game->defs.get())[id].name);
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
        if (!(((UnitDefMoveView *)g_game->defs.get())[(unsigned short)i].flags & 0x20)) {
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
    Ref<Order>* link = (node->flags_42 & ORDER_FLAG_SECOND_LIST) ? &owner->list2
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
    unsigned int which = node->flags_42 & ORDER_FLAG_SECOND_LIST;
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
    Ref<Order>* link = (node->flags_42 & ORDER_FLAG_SECOND_LIST) ? &owner->list2 : &owner->list;
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
        RemoveFromList((node->flags_42 & ORDER_FLAG_SECOND_LIST) ? &owner->list2 : base, node, owner->list);
    }
}

static inline void AddFront(Unit* owner, Order* node)
{
    Order* before = (node->flags_42 & ORDER_FLAG_SECOND_LIST) ? owner->list2 : owner->list;
    Ref<Order>* link = (node->flags_42 & ORDER_FLAG_SECOND_LIST) ? &owner->list2 : &owner->list;
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

// FUNCTION: 0x43adc0
void __stdcall AddOrder(MissionType kind, int remove, Unit* owner, Unit* target,
                            Vec3* pos, int param_6, int param_7)
{
    Order* obj = new (kSim) Order(kind.index, target, pos, param_6, param_7, 0);

    if (remove == 0 && !(obj->flags_42 & 0x40))
        PruneLoose(&owner->list, owner->list);

    if (!(obj->flags_42 & ORDER_FLAG_SECOND_LIST))
        PruneUsed(owner);

    obj->flags_42 |= 1;
    if (remove == 0)
        obj->flags_42 |= ORDER_FLAG_ANNOUNCE;

    if (obj->flags_42 & 0x40020)
        AddFront(owner, obj);
    else
        InsertOrderAfterMarked(owner, obj);
}

#pragma pack(push, 1)

typedef std::vector<OrderTableEntry> OrderTableEntryVector;
typedef int(__stdcall* OrderTableEntryCompare)(const OrderTableEntry&, const OrderTableEntry&);



#pragma pack(pop)




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
    if (node->flags_42 & ORDER_FLAG_SECOND_LIST) {
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
    AddOrder(MissionType(kind), remove, owner, (Unit*)id, (Vec3*)pos, param_6, param_7);
}

// FUNCTION: 0x419b00
void __stdcall QueueBuildOrder(char* name, Unit* unit, int count)
{
    if (strstr(name, "MAKENUKE") != 0 || strstr(name, "MAKEANTI") != 0) {
        AdjustBuildCount(MissionType("BUILDWEAPON").index, unit, 0, count);
        return;
    }
    unsigned short id = g_seams.units->FindUnitTypeId(name);
    if (id == 0)
        return;
    int mobile = (int)unit->motion.get();
    const char* kind = mobile ? "MOBILEBUILD" : "BUILDINGBUILD";
    AdjustBuildCount(MissionType(kind).index, unit, id, count);
}

// FUNCTION: 0x43b0b0
void __stdcall AdjustBuildCount(int kind, Unit* owner, int id, int amount)
{
    Order* node;
    if (amount > 0) {
        if (g_missionOrderTableVec[kind & 0xff].flags & ORDER_FLAG_SECOND_LIST)
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
        AddOrder(MissionType((unsigned char)kind), 1, owner, 0, 0, id, amount);
        return;
    }
    for (;;) {
        Order* found = 0;
        if (g_missionOrderTableVec[kind & 0xff].flags & ORDER_FLAG_SECOND_LIST)
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
        if (found->flags_42 & ORDER_FLAG_SECOND_LIST)
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
    unsigned int which = cmd->flags_42 & ORDER_FLAG_SECOND_LIST;
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
        MissionType kind2 = GetOrderType(2, unit, 0, (Vec3*)&unit->pos);
        Order* cmd = new (kSim) Order(kind2, 0, &unit->pos, 0, 0, 0);
        Insert(unit, cmd, (cmd->flags_42 & ORDER_FLAG_SECOND_LIST) ? unit->list2 : unit->list);
        Order* cmd2 = new (kSim) Order(kind, target, 0, 0, 0, ((UnitDefMoveView*)unit->def.get())->maneuverleashlength);
        cmd2->start.x = PosXWhole(unit);
        cmd2->start.y = PosZWhole(unit);
        Insert(unit, cmd2, (cmd2->flags_42 & ORDER_FLAG_SECOND_LIST) ? unit->list2 : unit->list);
    } else {
        Order* cmd3 = new (kSim) Order(kind, target, 0, 0, 0, 0);
        Insert(unit, cmd3, (cmd3->flags_42 & ORDER_FLAG_SECOND_LIST) ? unit->list2 : unit->list);
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
        MissionType kind2 = GetOrderType(2, unit, 0, (Vec3*)&unit->pos);
        Order* cmd = new (kSim) Order(kind2, 0, &unit->pos, 0, 0, 0);
        Insert(unit, cmd, (cmd->flags_42 & ORDER_FLAG_SECOND_LIST) ? unit->list2 : unit->list);
        Order* cmd2 = new (kSim) Order(kind, target, 0, 0, 0, ((UnitDefMoveView*)unit->def.get())->range);
        cmd2->start.x = PosXWhole(unit);
        cmd2->start.y = PosZWhole(unit);
        Insert(unit, cmd2, (cmd2->flags_42 & ORDER_FLAG_SECOND_LIST) ? unit->list2 : unit->list);
        return 1;
    } else if (f == 0x40000 && !param_3) {
        MissionType kind2 = GetOrderType(2, unit, 0, (Vec3*)&unit->pos);
        Order* cmd = new (kSim) Order(kind2, 0, &unit->pos, 0, 0, 0);
        Insert(unit, cmd, (cmd->flags_42 & ORDER_FLAG_SECOND_LIST) ? unit->list2 : unit->list);
        Order* cmd2 = new (kSim) Order(kind, target, 0, 0, 0, ((UnitDefMoveView*)unit->def.get())->maneuverleashlength);
        cmd2->start.x = PosXWhole(unit);
        cmd2->start.y = PosZWhole(unit);
        Insert(unit, cmd2, (cmd2->flags_42 & ORDER_FLAG_SECOND_LIST) ? unit->list2 : unit->list);
        return 1;
    } else if (f == 0x80000 && !param_3) {
        Order* cmd3 = new (kSim) Order(kind, target, 0, 0, 0, 0);
        Insert(unit, cmd3, (cmd3->flags_42 & ORDER_FLAG_SECOND_LIST) ? unit->list2 : unit->list);
        return 1;
    } else {
        return 0;
    }
}

// FUNCTION: 0x43b730
void __stdcall EnqueueOrderType(Unit* p, unsigned char type)
{
    Order* child = new (kSim) Order(type, 0, 0, 0, 0, 0);
    child->flags_42 |= 0x4000;
    InsertBefore(p, child, (child->flags_42 & ORDER_FLAG_SECOND_LIST) ? p->list2 : p->list);
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
    InsertOrderBefore(p, child, (child->flags_42 & ORDER_FLAG_SECOND_LIST) ? p->list2 : p->list);
}

static void DelayOrderWake(Order* node, int n)
{
    unsigned int when = RandomInt(n) + 0x1e;
    node->flags |= 1;
    node->wakeFrame = (unsigned int)g_game->ticks + when;
}

static void ClearAllWeaponTargets(Unit* unit)
{
    for (char i = 0; i < 3; i++)
        TargetingPublishClear(SCHED_ORDERS, unit, i);
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
            ClearAllWeaponTargets(unit);
        switch (g_missionOrderTableVec[node->kind.index].notify(node->unit, node, pending)) {
        case 3:
            DelayOrderWake(node, 0xf);
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
                DelayOrderWake(node, 0x1e);
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
    if (((UnitDefMoveView*)unit->def.get())->defaultmissiontype == 0)
        return;
    InsertCommand(unit, ((UnitDefMoveView*)unit->def.get())->defaultmissiontype);
}

static void RemoveAndDelete(Unit* p, Order* child)
{
    Order* first = p->list;
    Ref<Order>* link = (child->flags_42 & ORDER_FLAG_SECOND_LIST) ? &p->list2 : &p->list;
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
            switch (g_missionOrderTableVec[child->kind.index].notify(child->unit, child, 0)) {
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

// FUNCTION: 0x43c940
inline void __stdcall InsertShiftOrderTypes(OrderTableEntry* _L, OrderTableEntry _V, OrderTableEntryCompare _P)
{
    for (OrderTableEntry* _M = _L; _P(_V, *--_M); _L = _M)
        *_L = *_M;
    *_L = _V;
}

// FUNCTION: 0x43c990
inline void __stdcall InsertionSortOrderTypes(OrderTableEntry* _F, OrderTableEntry* _L, OrderTableEntryCompare _P,
                                   OrderTableEntry*)
{
    if (_F != _L)
        for (OrderTableEntry* _M = _F; ++_M != _L; ) {
            OrderTableEntry _V = *_M;
            if (!_P(_V, *_F))
                InsertShiftOrderTypes(_M, _V, _P);
            else {
                std::copy_backward(_F, _M, _M + 1);
                *_F = _V;
            }
        }
}

inline void _Insertion_sort_0043bc90(OrderTableEntry* _F, OrderTableEntry* _L, OrderTableEntryCompare _P)
{
    InsertionSortOrderTypes(_F, _L, _P, std::_Val_type(_F));
}

// FUNCTION: 0x43ca70
inline OrderTableEntry __stdcall MedianOrderTypes(OrderTableEntry _X, OrderTableEntry _Y, OrderTableEntry _Z,
                                            OrderTableEntryCompare _P)
{
    if (_P(_X, _Y))
        return (_P(_Y, _Z) ? _Y : _P(_X, _Z) ? _Z : _X);
    else
        return (_P(_X, _Z) ? _X : _P(_Y, _Z) ? _Z : _Y);
}

// FUNCTION: 0x43cb20
inline OrderTableEntry* __stdcall PartitionOrderTypes(OrderTableEntry* _F, OrderTableEntry* _L,
                                             OrderTableEntry _Piv, OrderTableEntryCompare _P)
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
inline void __stdcall SortOrderTypes(OrderTableEntry* _F, OrderTableEntry* _L, OrderTableEntryCompare _P,
                                   OrderTableEntry*)
{
    for (; std::_SORT_MAX < _L - _F; ) {
        OrderTableEntry* _M = PartitionOrderTypes(_F, _L, MedianOrderTypes(OrderTableEntry(*_F),
            OrderTableEntry(*(_F + (_L - _F) / 2)), OrderTableEntry(*(_L - 1)), _P), _P);
        if (_L - _M <= _M - _F)
            SortOrderTypes(_M, _L, _P, std::_Val_type(_F)), _L = _M;
        else
            SortOrderTypes(_F, _M, _P, std::_Val_type(_F)), _F = _M;
    }
}

inline void _Sort_0_0043bc90(OrderTableEntry* _F, OrderTableEntry* _L, OrderTableEntryCompare _P,
                             OrderTableEntry*)
{
    if (_L - _F <= std::_SORT_MAX)
        _Insertion_sort_0043bc90(_F, _L, _P);
    else {
        SortOrderTypes(_F, _L, _P, (OrderTableEntry*)0);
        _Insertion_sort_0043bc90(_F, _F + std::_SORT_MAX, _P);
        for (_F += std::_SORT_MAX; _F != _L; ++_F)
            InsertShiftOrderTypes(_F, OrderTableEntry(*_F), _P);
    }
}

inline void SortOrderTableEntries(OrderTableEntry* _F, OrderTableEntry* _L, OrderTableEntryCompare _P)
{
    _Sort_0_0043bc90(_F, _L, _P, std::_Val_type(_F));
}


// FUNCTION: 0x43bc90
void __stdcall RegisterOrderTypes(OrderTableEntry* from, int count)
{
    int sz = g_missionOrderTableVec.Size();
    g_missionOrderTableVec.Reserve(sz + count);
    for (int i = 0; i < count; i++)
        g_missionOrderTableVec.PushBack(from[i]);
    SortOrderTableEntries(g_missionOrderTableVec.Begin(), g_missionOrderTableVec.End(), CompareOrderTypeNames);
}

// FUNCTION: 0x43c020
int __stdcall CompareOrderTypeNames(const OrderTableEntry& a, const OrderTableEntry& b)
{
    return _strcmpi(a.name, b.name) < 0 ? 1 : 0;
}

// FUNCTION: 0x43c050
void RegisterAllOrderTypes()
{
    RegisterOrderTypes(g_readyOrder, 1);
    RegisterGroundOrders();
    RegisterVtolOrders();
    RegisterUnitOrders();
}

// FUNCTION: 0x43c350
void ClearOrderTypeTable()
{
    g_missionOrderTableVec.Clear();
}

template class std::vector<OrderTableEntry>;

// FUNCTION: 0x43c3a0 ?insert@?$vector@UOrderTableEntry@@V?$allocator@UOrderTableEntry@@@std@@@std@@QAEXPAUOrderTableEntry@@IABU3@@Z

typedef int (__stdcall* OrderTableNamePred)(const OrderTableEntry& entry, char* name);

// FUNCTION: 0x43c6b0
OrderTableEntry* __stdcall LowerBoundOrderTypes(OrderTableEntry* first, OrderTableEntry* last,
                                      char* const& value, OrderTableNamePred pred, int*)
{
    int n = 0;
    n += last - first;
    for (; 0 < n; ) {
        int n2 = n / 2;
        OrderTableEntry* m = first;
        m += n2;
        if (pred(*m, value))
            first = ++m, n -= n2 + 1;
        else
            n = n2;
    }
    return first;
}
