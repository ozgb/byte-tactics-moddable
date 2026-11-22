#include "../../interfaces/seams.h"
#include "../../interfaces/orders.h"
#include "../../interfaces/services.h"
#include "../movement/ai_search_goal.h"
#include "../movement/air_maneuver_order.h"
#include "../units/unit_def.h"
#include "../../interfaces/ref.h"
#include <stdio.h>
#include <string.h>
#include "order_queue.h"
#include "order_type_entry.h"

class Order;
class HapiBank;

#pragma pack(push, 1)
class CobScript;

#include "../units/unit.h"

#include "../game/game.h"

struct OrderSaveRecord {
    unsigned short unitType;
    unsigned short ownerType;
    int targetType;
    unsigned char kind;
    unsigned char flag5;
    unsigned int flags6;
    int last_id;
    Vec3 pos;
    int start;
    int cached;
    int field_26;
    int radius;
    int progress;
    unsigned int flags;
    int subFlags;
};
#pragma pack(pop)
#include "../files/files_api.h"

#include "../state/block.h"
#include "../scripts/cob_script.h"

#include "../units/unit_ref.h"

#include "../util/vec3.h"
#include "../state/sim_globals.h"

#include "order_list.h"
OrderTableEntry* __stdcall LowerBoundOrderTypes(OrderTableEntry* first, OrderTableEntry* last,
                                       const char* const& value, int(__stdcall* pred)(int, const char*),
                                       int* unused);
#include "../network/net_game.h"
#include "../sound/sound.h"
#include "../state/sim_speech.h"

#include "../../link/globals.h"

#include "../units/path_order_attach.h"

#include "mission_type.h"

#pragma pack(push, 1)
#include "../movement/path_order.h"

#include "../movement/approach_areas.h"

#pragma pack(pop)

#include "order.h"
#include "../game/weapons_events.h"
#include "../game/movement_events.h"
#include "../game/orders_events.h"
#include "../../interfaces/movement.h"
extern int __cdecl _strcmpi(const char*, const char*);

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

// FUNCTION: 0x4384a0
void __stdcall AddBeCarriedOrder(Unit* p)
{
    if (p->carrier) {
        Order* first = p->list;
        Ref<Order>* pp = &p->list;
        Order* node;
        while ((node = *pp) != 0) {
            if (node->flags_42 & 4) {
                pp = &node->next;
            } else {
                *pp = node->next;
                if (node != first) {
                    node->flags_42 |= 0x10000;
                }
                delete node;
            }
        }
        Order* child =
            new (kSim) Order("BECARRIED", p->carrier, 0, 0, 0, 0);
        InsertBefore(p, child, (child->flags_42 & ORDER_FLAG_SECOND_LIST) ? p->list2 : p->list);
    }
}

// FUNCTION: 0x438590
void __stdcall StartBuildingScript(Unit* obj, Order* target, unsigned short param_3)
{
    int index = g_seams.scripts->FindScript((CobScript*)obj->script.get(), "StartBuilding");
    OrdersPublishStartScript(obj, ORDERS_SCRIPT_BY_INDEX_WITH_ARGS, index, 0, 0, 0, 1, param_3, 0, 0, 0);
    g_seams.network->SendScriptCall(obj, index, 1, param_3, 0, 0, 0);
    target->flags_42 |= 0x400000;
}

// FUNCTION: 0x4385f0
void __stdcall StopBuildingScript(Unit* obj, Order* target)
{
    if (target->flags_42 & 0x400000) {
        int index = g_seams.scripts->FindScript((CobScript*)obj->script.get(), "StopBuilding");
        OrdersPublishStartScript(obj, ORDERS_SCRIPT_BY_INDEX_WITH_ARGS, index, 0, 0, 0, 0, 0, 0, 0, 0);
        g_seams.network->SendScriptCall(obj, index, 0, 0, 0, 0, 0);
        target->flags_42 &= ~0x400000;
    }
}

// FUNCTION: 0x438650
int __stdcall ComputeReclaimDamagePulse(Unit* a, Unit* b, int n)
{
    UnitDef* bt = b->def.get();
    float v = bt->metalCost > 10.0f ? bt->metalCost : 10.0f;
    union {
        __int64 q;
        struct {
            int lo;
            int hi;
        } w;
    } p;
    p.w.lo = a->def->buildRate * ((a->killCount + 5) / 5) * (int)bt->maxHealth * n;
    p.w.hi = 0;
    int r = (int)((double)p.q / (v * 300.0f));
    if (r <= 1) {
        r = 1;
    }
    return r;
}

// FUNCTION: 0x438700
unsigned int __stdcall WaitIfNotInBuildStance(void* param_1, void* param_2, unsigned int param_3)
{
    if ((*(unsigned char*)((char*)param_1 + 0x10f) & 1) == 0) {
        *(unsigned int*)((char*)param_2 + 6) = param_3 | 4;
        return 2;
    }
    return 1;
}

// FUNCTION: 0x438730
int __stdcall WaitIfCobBusy(char* param_1, char* param_2, unsigned int param_3)
{
    if (*(unsigned char*)(param_1 + 0x10f) & 2) {
        *(unsigned int*)(param_2 + 6) = param_3 | 4;
        return 2;
    }
    return 1;
}


// FUNCTION: 0x438760
MissionType::MissionType(const char* name)
{
    OrderTableEntry* first = g_missionOrderTableVec.Begin();
    int n = g_missionOrderTableVec.End() - g_missionOrderTableVec.Begin();
    for (; 0 < n; ) {
        int n2 = n / 2;
        OrderTableEntry* m = first;
        m += n2;
        int less = _strcmpi(m->name, name) < 0;
        if (less)
            first = ++m, n -= n2 + 1;
        else
            n = n2;
    }
    if (first != g_missionOrderTableVec.End() && _strcmpi(first->name, name) == 0) {
        index = (unsigned char)(first - g_missionOrderTableVec.Begin());
        return;
    }
    index = 0;
}

// FUNCTION: 0x438830
OrderTableEntry* MissionType::GetTableEntry()
{
    unsigned int result = 0;
    result = *(unsigned char*)this;
    return &g_missionOrderTableVec[result];
}

// FUNCTION: 0x438850
int __fastcall GetTableEntryByTypeByte(unsigned char* param_1)
{
    return (int)&g_missionOrderTableVec[*param_1];
}

// FUNCTION: 0x438870
void Order::OrStatusFlags(unsigned int param_1)
{
    subFlags |= param_1;
}

// FUNCTION: 0x438880
void Order::AnnounceStatusIfFlagged(SimSpeechText text)
{
    if (flags_42 & ORDER_FLAG_ANNOUNCE) {
        flags_42 &= ~ORDER_FLAG_ANNOUNCE;
        RecordUnitSpeech(unit, 5, text);
    }
}

// FUNCTION: 0x4388b0
void Order::ReattachFxToUnit()
{
    if (attached != 0) {
        MovementPublishTarget(SCHED_ORDERS, MOVEMENT_SET_TARGET, unit, attached.offset);
    }
}

// FUNCTION: 0x4388d0
void Order::SetAttachedFx(unsigned int obj)
{
    if (unit->motion.get()) {
        if (attached) {
            MovementPublishTarget(SCHED_ORDERS, MOVEMENT_SET_TARGET, unit, 0);
            MovementPublishTarget(SCHED_ORDERS, MOVEMENT_DESTROY_TARGET, unit, attached.offset);
            attached = 0;
        }
        if (obj) {
            subFlags &= ~0x3e0;
            MovementPublishTarget(SCHED_ORDERS, MOVEMENT_SET_TARGET, unit, obj);
            attached.offset = obj;
        }
    }
}

// FUNCTION: 0x438930
void Order::AttachApproachRadiusGoal(Vec3* p, int n)
{
    if ((unit->def->flags1 & 0x800) == 0) {
        unsigned int obj = g_seams.movement->NewApproachRadius(Ref<Order>(this).offset, p->x, p->z, n);
        if (unit->motion.get()) {
            if (attached) {
                MovementPublishTarget(SCHED_ORDERS, MOVEMENT_SET_TARGET, unit, 0);
                MovementPublishTarget(SCHED_ORDERS, MOVEMENT_DESTROY_TARGET, unit, attached.offset);
                attached = 0;
            }
            if (obj) {
                subFlags &= ~0x3e0;
                MovementPublishTarget(SCHED_ORDERS, MOVEMENT_SET_TARGET, unit, obj);
                attached.offset = obj;
            }
        }
    } else {
        if (unit->motion.get() && attached) {
            MovementPublishTarget(SCHED_ORDERS, MOVEMENT_SET_TARGET, unit, 0);
            MovementPublishTarget(SCHED_ORDERS, MOVEMENT_DESTROY_TARGET, unit, attached.offset);
            attached = 0;
        }
    }
}

// FUNCTION: 0x438a00
void Order::AttachRingApproachGoal(Vec3* pos, int radius1, int radius2)
{
    if (!(unit->def->flags1 & 0x800)) {
        unsigned int obj = g_seams.movement->NewRingApproach(Ref<Order>(this).offset, pos->x, pos->z, radius1, radius2);
        if (unit->motion.get()) {
            if (attached) {
                MovementPublishTarget(SCHED_ORDERS, MOVEMENT_SET_TARGET, unit, 0);
                MovementPublishTarget(SCHED_ORDERS, MOVEMENT_DESTROY_TARGET, unit, attached.offset);
                attached = 0;
            }
            if (obj) {
                subFlags &= ~0x3e0;
                MovementPublishTarget(SCHED_ORDERS, MOVEMENT_SET_TARGET, unit, obj);
                attached.offset = obj;
            }
        }
    } else {
        if (unit->motion.get()) {
            if (attached) {
                MovementPublishTarget(SCHED_ORDERS, MOVEMENT_SET_TARGET, unit, 0);
                MovementPublishTarget(SCHED_ORDERS, MOVEMENT_DESTROY_TARGET, unit, attached.offset);
                attached = 0;
            }
        }
    }
}

// FUNCTION: 0x438ad0
void Order::AttachBuildFootprintMarker(Point16 cell, Point16 size)
{
    if (!(unit->def->flags1 & 0x800)) {
        unsigned int obj = g_seams.movement->NewPointMarker(Ref<Order>(this).offset, cell, size);
        if (unit->motion.get()) {
            if (attached) {
                MovementPublishTarget(SCHED_ORDERS, MOVEMENT_SET_TARGET, unit, 0);
                MovementPublishTarget(SCHED_ORDERS, MOVEMENT_DESTROY_TARGET, unit, attached.offset);
                attached = 0;
            }
            if (obj) {
                subFlags &= ~0x3e0;
                MovementPublishTarget(SCHED_ORDERS, MOVEMENT_SET_TARGET, unit, obj);
                attached.offset = obj;
            }
        }
    } else {
        if (unit->motion.get()) {
            if (attached) {
                MovementPublishTarget(SCHED_ORDERS, MOVEMENT_SET_TARGET, unit, 0);
                MovementPublishTarget(SCHED_ORDERS, MOVEMENT_DESTROY_TARGET, unit, attached.offset);
                attached = 0;
            }
        }
    }
}

// FUNCTION: 0x438b90
void Order::MergeFlagsFromTable(int k)
{
    kind.index = k;
    flags_42 = ((g_missionOrderTableVec[k & 0xff].flags ^ flags_42) & (ORDER_FLAG_TARGET | ORDER_FLAG_POSITION)) ^ g_missionOrderTableVec[kind.index].flags;
}

// FUNCTION: 0x438be0
int __stdcall GetOrderFlags(void* param)
{
    if (!param) return 0;
    void* ptr1 = ((Unit*)param)->list.get();
    if (!ptr1) return 0;
    return *(int*)((char*)ptr1 + 0x42);
}


// FUNCTION: 0x43a1f0
Order::~Order()
{
    if (flags & 2) {
        g_missionOrderTableVec[kind.index].notify(unit, this, 2);
    }
    if (flags_42 & 0x400000) {
        Unit* obj = unit;
        int index = g_seams.scripts->FindScript((CobScript*)obj->script.get(), "StopBuilding");
        OrdersPublishStartScript(obj, ORDERS_SCRIPT_BY_INDEX_WITH_ARGS, index, 0, 0, 0, 0, 0, 0, 0, 0);
        g_seams.network->SendScriptCall(obj, index, 0, 0, 0, 0, 0);
        flags_42 &= ~0x400000;
    }
    if (unit->motion.get() != 0) {
        unsigned int current = g_seams.movement->UnitTarget(unit);
        if (current != 0 && current == attached.offset) {
            if (attached != 0) {
                MovementPublishTarget(SCHED_ORDERS, MOVEMENT_SET_TARGET, unit, 0);
                MovementPublishTarget(SCHED_ORDERS, MOVEMENT_DESTROY_TARGET, unit, attached.offset);
                attached = 0;
            }
        } else {
            if (attached) {
                MovementPublishTarget(SCHED_ORDERS, MOVEMENT_DESTROY_TARGET, unit, attached.offset);
            }
            attached = 0;
        }
    }
    if (!(flags_42 & 0x10000)) {
        WeaponsPublishRelease(SCHED_ORDERS, (Unit*)unit, 3);
    }
    OrdersPublishUnlink(link.owner.get(), &link);
}

short __stdcall ResolveUnitTypeKey(HapiBank* file, unsigned short id)
{
    char key[0x80];
    sprintf(key, "UTYPENAME%4d", id);
    if (file->HasItem(key))
        return g_seams.units->FindUnitTypeId(file->GetStringItem(key, 0));
    int i, n = 0;
    unsigned short k = 0;
    for (i = 1; i < g_game->unitDefCount; i++, n++) {
        if (!(g_game->defs.get()[(unsigned short)i].flags1Byte & 0x20)) {
            if (k == id)
                return n;
            k++;
        }
    }
    return 0;
}

static unsigned char OrderKindFromName(const char* s)
{
    OrderTableEntry* e = LowerBoundOrderTypes(g_missionOrderTableVec.Begin(), g_missionOrderTableVec.End(), s, OrderTypeNameLess, 0);
    if (e == g_missionOrderTableVec.End() || _strcmpi(e->name, s) != 0)
        return 0;
    return (unsigned char)(e - g_missionOrderTableVec.Begin());
}

static unsigned char OrderKindFromSavedIndex(unsigned char want)
{
    int idx = 0;
    int k = 0;
    for (OrderTableEntry* p = g_missionOrderTableVec.Begin(); p <= g_missionOrderTableVec.End(); p++, idx++) {
        if (!((p->flags >> 24) & 1)) {
            if (k == want)
                break;
            k++;
        }
    }
    return (unsigned char)idx;
}

// FUNCTION: 0x43a420
Order::Order(Unit* punit, HapiBank* file, char* name)
{
    g_seams.units->UnitRefInit((UnitRef*)&link, nullptr, nullptr);
    g_seams.units->UnitRefSetValue((UnitRef*)&link, this);
    g_seams.units->UnitRefSetUnit((UnitRef*)&link, 0);
    next = 0;
    attached = 0;
    created = (unsigned int)g_game->ticks;
    if (file == 0)
        return;
    if (name == 0)
        return;
    if (strlen(name) > 0x1f)
        return;

    file->OpenNamedBox(name);
    file->SeekBox(0);
    OrderSaveRecord desc;
    if (file->ReadBox(&desc, 0x3a) != 0x3a)
        return;

    char buf1[0x80];
    sprintf(buf1, "%s%s", name, "_name");
    {
        const char* s = file->GetStringItem(buf1, 0);
        if (s != 0)
            desc.kind = OrderKindFromName(s);
        else
            desc.kind = OrderKindFromSavedIndex(desc.kind);
    }

    Unit* u;
    if (desc.unitType == 0)
        u = 0;
    else
        u = g_game->units + desc.unitType;
    if (punit != u)
        return;
    if (desc.kind == 0)
        return;

    unit = punit;
    g_seams.units->UnitRefSetUnit((UnitRef*)&link, g_seams.units->LoadUnit(desc.ownerType, file));

    kind.index = desc.kind;
    state = desc.flag5;
    flags = desc.flags6;
    wakeFrame = desc.last_id;
    pos = *(Vec3*)&desc.pos;
    start = *(Point16*)&desc.start;
    cached = *(Point16*)&desc.cached;
    unitType = desc.field_26;
    radius = desc.radius;
    progress = desc.progress;
    flags_42 = desc.flags;
    subFlags = desc.subFlags;

    char* sname = g_missionOrderTableVec[desc.kind].name;
    if (strcmp(sname, "MobileBuild") == 0 || strcmp(sname, "VTOL_MobileBuild") == 0 ||
        strcmp(sname, "BuildingBuild") == 0) {
        unitType = (unsigned short)ResolveUnitTypeKey(file, (unsigned short)unitType);
    }

    char buf3[0x20];
    sprintf(buf3, "%s%s", name, "g");
    switch (desc.targetType) {
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        attached.offset = g_seams.movement->LoadTarget(desc.targetType, Ref<Order>(this).offset, file, buf3);
        return;
    case 0:
        attached = 0;
        return;
    case 1:
        attached = 0;
        return;
    default:
        attached = 0;
        return;
    }
}

// FUNCTION: 0x43a970
int Order::SerializeToSave(Unit* punit, HapiBank* file, char* name)
{
    if (unit->id != punit->id || !file || !name)
        return 0;
    if (strlen(name) > 0x1f)
        return 0;

    OrderSaveRecord desc;
    Unit* u = unit;
    if (u == 0)
        desc.unitType = 0;
    else
        desc.unitType = u->id;
    Unit* o = link.owner;
    desc.ownerType = (o != 0 && (link.owner->flags & 0x10000000) != 0 && link.owner != 0) ? o->id : 0;
    desc.targetType = attached ? g_seams.movement->TargetType(attached.offset) : 0;
    desc.kind = kind.index;
    desc.flag5 = state;
    desc.flags6 = flags;
    desc.last_id = wakeFrame;
    desc.pos = *(Vec3*)&pos;
    desc.start = *(int*)&start;
    desc.cached = *(int*)&cached;
    desc.field_26 = unitType;
    desc.radius = radius;
    desc.progress = progress;
    desc.flags = flags_42;
    desc.subFlags = subFlags;

    file->OpenNamedBox(name);
    file->SeekBox(0);
    file->WriteBox(&desc, 0x3a);

    char buf1[0x80];
    sprintf(buf1, "%s%s", name, "_name");
    file->SetStringItem(buf1, g_missionOrderTableVec[kind.index].name);

    char* s = g_missionOrderTableVec[kind.index].name;
    if (strcmp(s, "MobileBuild") == 0 || strcmp(s, "VTOL_MobileBuild") == 0 ||
        strcmp(s, "BuildingBuild") == 0) {
        unsigned short id = unitType;
        char buf2[0x80];
        sprintf(buf2, "UTYPENAME%4d", id);
        if (!file->HasItem(buf2) && id >= 1 && id < g_game->unitDefCount)
            file->SetStringItem(buf2, g_game->defs.get()[id].unitname);
    }

    if (desc.targetType != 0) {
        char buf3[0x20];
        sprintf(buf3, "%s%s", name, "g");
        g_seams.movement->SaveTarget(attached.offset, Ref<Order>(this).offset, file, buf3);
    }
    return 1;
}
