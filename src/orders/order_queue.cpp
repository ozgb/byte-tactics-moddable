#include "../../interfaces/seams.h"
#include "../../interfaces/ref.h"
#include <stdio.h>
#include <string.h>
#include "order_queue.h"
#include "order_type_entry.h"

class Order;
class HapiBank;

#pragma pack(push, 1)
struct Vec3_0043a1f0 {
    int x, y, z;
};

class CobScript;
class Slot_0043a1f0;

struct Owner_0043a1f0 {
    Ref<Slot_0043a1f0> slot;
};

struct UnitType {
    char unknown_0[0x20];
    char name[0x20];
    char unknown_40[0x18a - 0x40];
    float metalCost;
    char unknown_18e[0x1fa - 0x18e];
    unsigned int maxHealth;
    unsigned short field_1fe;
    char unknown_200[0x241 - 0x200];
    union {
        unsigned char flags;
        unsigned int flags32;
    };
    char unknown_245[0x249 - 0x245];
};

#include "../units/unit.h"

struct Entry_0043a1f0 {
    char unknown_0[4];
    void (__stdcall* notify)(Unit* unit, Order* obj, int code);
    char unknown_8[0x11 - 0x8];
    union {
        unsigned int flags;
        struct {
            char unknown_11[3];
            unsigned char flag14;
        };
    };
    char* name;
};

#include "../game/game.h"

struct SaveDesc_0043a1f0 {
    unsigned short unitType;
    unsigned short ownerType;
    int field_4;
    unsigned char kind;
    unsigned char flag5;
    unsigned int flags6;
    int last_id;
    Vec3_0043a1f0 pos;
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
#include "../units/cob_script.h"

#include "../units/unit_ref.h"

#include "../util/vec3.h"
#include "air_maneuver_order.h"
#include "../state/sim_globals.h"

class Slot_0043a1f0 {
public:
    virtual void Slot0();
    virtual void Attach(void* obj);
    Ref<OrderFx> current;
};

#include "order_list.h"
Entry_0043a1f0* __stdcall LowerBoundOrderTypes(Entry_0043a1f0* first, Entry_0043a1f0* last,
                                       char* const& value, int(__stdcall* pred)(int, char*),
                                       int* unused);
#include "../network/net_game.h"
void __stdcall SendScriptCall(Unit* obj, int index, int param_3, int param_4, int param_5, int param_6, int param_7);
#include "../sound/sound.h"

#include "../../link/globals.h"
typedef Entry_0043a1f0* SimView_g_missionOrderTableBegin;
#undef g_missionOrderTableBegin
#define g_missionOrderTableBegin (*reinterpret_cast<SimView_g_missionOrderTableBegin*>(reinterpret_cast<char*>(&g_missionOrderTableVec) + 0x4))
typedef Entry_0043a1f0* SimView_g_missionOrderTableEnd;
#undef g_missionOrderTableEnd
#define g_missionOrderTableEnd (*reinterpret_cast<SimView_g_missionOrderTableEnd*>(reinterpret_cast<char*>(&g_missionOrderTableVec) + 0x8))

#include "path_order_attach.h"

#include "mission_type.h"

#pragma pack(push, 1)
#include "path_fx.h"

#include "approach_areas.h"

#pragma pack(pop)

#include "order.h"
extern int __cdecl _strcmpi(const char*, const char*);

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
        InsertBefore(p, child, (child->flags_42 & 0x40000) ? p->list2 : p->list);
    }
}

// FUNCTION: 0x438590
void __stdcall StartBuildingScript(Unit* obj, Order* target, unsigned short param_3)
{
    int index = g_seams.units->FindScript((CobScript*)obj->script.get(), "StartBuilding");
    g_seams.units->StartScriptWithArgsByIndex((CobScript*)obj->script.get(), index, 0, 0, 1, param_3, 0, 0, 0);
    SendScriptCall(obj, index, 1, param_3, 0, 0, 0);
    target->flags_42 |= 0x400000;
}

// FUNCTION: 0x4385f0
void __stdcall StopBuildingScript(Unit* obj, Order* target)
{
    if (target->flags_42 & 0x400000) {
        int index = g_seams.units->FindScript((CobScript*)obj->script.get(), "StopBuilding");
        g_seams.units->StartScriptWithArgsByIndex((CobScript*)obj->script.get(), index, 0, 0, 0, 0, 0, 0, 0);
        SendScriptCallNoArgs(obj, index);
        target->flags_42 &= ~0x400000;
    }
}

// FUNCTION: 0x438650
int __stdcall ComputeReclaimDamagePulse(Unit* a, Unit* b, int n)
{
    UnitType* bt = ((UnitType *)b->def.get());
    float v = bt->metalCost > 10.0f ? bt->metalCost : 10.0f;
    union {
        __int64 q;
        struct {
            int lo;
            int hi;
        } w;
    } p;
    p.w.lo = ((UnitType *)a->def.get())->field_1fe * ((a->killCount + 5) / 5) * (int)bt->maxHealth * n;
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
    Entry_0043a1f0* first = g_missionOrderTableBegin;
    int n = g_missionOrderTableEnd - g_missionOrderTableBegin;
    for (; 0 < n; ) {
        int n2 = n / 2;
        Entry_0043a1f0* m = first;
        m += n2;
        int less = _strcmpi(m->name, name) < 0;
        if (less)
            first = ++m, n -= n2 + 1;
        else
            n = n2;
    }
    if (first != g_missionOrderTableEnd && _strcmpi(first->name, name) == 0) {
        index = (unsigned char)(first - g_missionOrderTableBegin);
        return;
    }
    index = 0;
}

// FUNCTION: 0x438830
Elem_0043c390* OrderType::GetTableEntry()
{
    unsigned int result = 0;
    result = *(unsigned char*)this;
    int* base = (int*)&g_missionOrderTableBegin;
    int fives = result * 5;
    return (Elem_0043c390*)(*base + fives * 4 + fives);
}

// FUNCTION: 0x438850
int __fastcall GetTableEntryByTypeByte(unsigned char* param_1)
{
    unsigned int eax = 0;
    eax = *param_1;
    eax = eax + eax * 4;
    int edx = (int)g_missionOrderTableBegin + eax * 4;
    eax = eax + edx;
    return eax;
}

// FUNCTION: 0x438870
void Order::OrStatusFlags(unsigned int param_1)
{
    subFlags |= param_1;
}

// FUNCTION: 0x438880
void Order::AnnounceStatusIfFlagged(const char* text)
{
    if (flags_42 & 0x2000) {
        flags_42 &= ~0x2000;
        QueueUnitSpeech(unit, 5, (char*)text);
    }
}

// FUNCTION: 0x4388b0
void Order::ReattachFxToUnit()
{
    if (attached != 0) {
        void* p1 = unit->motion.get();
        void* p2 = ((Owner_0043a1f0*)p1)->slot.get();
        void* p3 = *(void**)p2;
        void* fn_ptr = *(void**)((char*)p3 + 4);
#ifdef __clang__
        ((void (__thiscall*)(void*, int))fn_ptr)(p2, (int)(OrderFx*)attached);
#else
        ((void (__stdcall*)(int))fn_ptr)((int)(OrderFx*)attached);
#endif
    }
}

// FUNCTION: 0x4388d0
void Order::SetAttachedFx(OrderFx* obj)
{
    if (((Owner_0043a1f0 *)unit->motion.get())) {
        if (attached) {
            ((Owner_0043a1f0 *)unit->motion.get())->slot->Attach(0);
            delete (OrderFx*)attached;
            attached = 0;
        }
        if (obj) {
            subFlags &= ~0x3e0;
            ((Owner_0043a1f0 *)unit->motion.get())->slot->Attach(obj);
            attached = obj;
        }
    }
}

// FUNCTION: 0x438930
void Order::AttachApproachRadiusGoal(Vec3* p, int n)
{
    if ((((UnitType *)unit->def.get())->flags32 & 0x800) == 0) {
        ApproachRadius* obj = new (kSim) ApproachRadius(this, p->x, p->z, n);
        if (((Owner_0043a1f0 *)unit->motion.get())) {
            if (attached) {
                ((Owner_0043a1f0 *)unit->motion.get())->slot->Attach(0);
                delete (OrderFx*)attached;
                attached = 0;
            }
            if (obj) {
                subFlags &= ~0x3e0;
                ((Owner_0043a1f0 *)unit->motion.get())->slot->Attach(obj);
                attached = obj;
            }
        }
    } else {
        if (((Owner_0043a1f0 *)unit->motion.get()) && attached) {
            ((Owner_0043a1f0 *)unit->motion.get())->slot->Attach(0);
            delete (OrderFx*)attached;
            attached = 0;
        }
    }
}

// FUNCTION: 0x438a00
void Order::AttachRingApproachGoal(Vec3* pos, int radius1, int radius2)
{
    if (!(((UnitType *)unit->def.get())->flags32 & 0x800)) {
        RingApproach* obj = new (kSim) RingApproach(this, pos->x, pos->z, radius1, radius2);
        if (((Owner_0043a1f0 *)unit->motion.get())) {
            if (attached) {
                ((Owner_0043a1f0 *)unit->motion.get())->slot->Attach(0);
                delete (OrderFx*)attached;
                attached = 0;
            }
            if (obj) {
                subFlags &= ~0x3e0;
                ((Owner_0043a1f0 *)unit->motion.get())->slot->Attach(obj);
                attached = obj;
            }
        }
    } else {
        if (((Owner_0043a1f0 *)unit->motion.get())) {
            if (attached) {
                ((Owner_0043a1f0 *)unit->motion.get())->slot->Attach(0);
                delete (OrderFx*)attached;
                attached = 0;
            }
        }
    }
}

// FUNCTION: 0x438ad0
void Order::AttachBuildFootprintMarker(Point16 cell, Point16 size)
{
    if (!(((UnitType *)unit->def.get())->flags32 & 0x800)) {
        PointMarker* obj = new (kSim) PointMarker(this, cell, size);
        if (((Owner_0043a1f0 *)unit->motion.get())) {
            if (attached) {
                ((Owner_0043a1f0 *)unit->motion.get())->slot->Attach(0);
                delete (OrderFx*)attached;
                attached = 0;
            }
            if (obj) {
                subFlags &= ~0x3e0;
                ((Owner_0043a1f0 *)unit->motion.get())->slot->Attach(obj);
                attached = obj;
            }
        }
    } else {
        if (((Owner_0043a1f0 *)unit->motion.get())) {
            if (attached) {
                ((Owner_0043a1f0 *)unit->motion.get())->slot->Attach(0);
                delete (OrderFx*)attached;
                attached = 0;
            }
        }
    }
}

// FUNCTION: 0x438b90
void Order::MergeFlagsFromTable(int k)
{
    kind.index = k;
    flags_42 = ((g_missionOrderTableBegin[k & 0xff].flags ^ flags_42) & 0x600) ^ g_missionOrderTableBegin[kind.index].flags;
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
        g_missionOrderTableBegin[kind.index].notify(unit, this, 2);
    }
    if (flags_42 & 0x400000) {
        Unit* obj = unit;
        int index = g_seams.units->FindScript((CobScript*)obj->script.get(), "StopBuilding");
        g_seams.units->StartScriptWithArgsByIndex((CobScript*)obj->script.get(), index, 0, 0, 0, 0, 0, 0, 0);
        SendScriptCallNoArgs(obj, index);
        flags_42 &= ~0x400000;
    }
    if (((Owner_0043a1f0 *)unit->motion.get()) != 0) {
        Slot_0043a1f0* slot = ((Owner_0043a1f0 *)unit->motion.get())->slot;
        if (slot->current.get() != 0 && slot->current.get() == attached.get()) {
            if (attached != 0) {
                slot->Attach(0);
                delete (OrderFx*)attached;
                attached = 0;
            }
        } else {
            delete (OrderFx*)attached;
            attached = 0;
        }
    }
    if (!(flags_42 & 0x10000)) {
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
    }
    g_seams.units->UnitRefUnlink((UnitRef*)&link);
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
        if (!(((UnitType *)g_game->defs.get())[(unsigned short)i].flags & 0x20)) {
            if (k == id)
                return n;
            k++;
        }
    }
    return 0;
}

static unsigned char KindByName_0043a420(char* s)
{
    Entry_0043a1f0* e = LowerBoundOrderTypes(g_missionOrderTableBegin, g_missionOrderTableEnd, s, OrderTypeNameLess, 0);
    if (e == g_missionOrderTableEnd || _strcmpi(e->name, s) != 0)
        return 0;
    return (unsigned char)(e - g_missionOrderTableBegin);
}

static unsigned char KindByIndex_0043a420(unsigned char want)
{
    int idx = 0;
    int k = 0;
    for (Entry_0043a1f0* p = g_missionOrderTableBegin; p <= g_missionOrderTableEnd; p++, idx++) {
        if (!(p->flag14 & 1)) {
            if (k == want)
                break;
            k++;
        }
    }
    return (unsigned char)idx;
}

// FUNCTION: 0x43a420
Order::Order(Unit* punit, HapiBank* file, char* name)
    : link(0, 0)
{
    link.SetValue(this);
    link.SetUnit(0);
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
    SaveDesc_0043a1f0 desc;
    if (file->ReadBox(&desc, 0x3a) != 0x3a)
        return;

    char buf1[0x80];
    sprintf(buf1, "%s%s", name, "_name");
    {
        char* s = file->GetStringItem(buf1, 0);
        if (s != 0)
            desc.kind = KindByName_0043a420(s);
        else
            desc.kind = KindByIndex_0043a420(desc.kind);
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
    link.SetUnit(g_seams.units->LoadUnit(desc.ownerType, file));

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

    char* sname = g_missionOrderTableBegin[desc.kind].name;
    if (strcmp(sname, "MobileBuild") == 0 || strcmp(sname, "VTOL_MobileBuild") == 0 ||
        strcmp(sname, "BuildingBuild") == 0) {
        unitType = (unsigned short)ResolveUnitTypeKey(file, (unsigned short)unitType);
    }

    char buf3[0x20];
    sprintf(buf3, "%s%s", name, "g");
    switch (desc.field_4) {
    case 2:
        attached = new (kSim) Class_0044de80((int)this, file, buf3);
        return;
    case 3:
        attached = new (kSim) AirManeuverOrder((int)this, file, buf3);
        return;
    case 4:
        attached = new (kSim) ApproachRadius((int)this, file, buf3);
        return;
    case 5:
        attached = new (kSim) RingApproach((int)this, file, buf3);
        return;
    case 6:
        attached = new (kSim) PointMarker((int)this, file, buf3);
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

    SaveDesc_0043a1f0 desc;
    Unit* u = unit;
    if (u == 0)
        desc.unitType = 0;
    else
        desc.unitType = u->id;
    Unit* o = link.owner;
    desc.ownerType = (o != 0 && (link.owner->flags & 0x10000000) != 0 && link.owner != 0) ? o->id : 0;
    desc.field_4 = attached ? ((OrderFx*)attached)->GetType() : 0;
    desc.kind = kind.index;
    desc.flag5 = state;
    desc.flags6 = flags;
    desc.last_id = wakeFrame;
    desc.pos = *(Vec3_0043a1f0*)&pos;
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
    file->SetStringItem(buf1, g_missionOrderTableBegin[kind.index].name);

    char* s = g_missionOrderTableBegin[kind.index].name;
    if (strcmp(s, "MobileBuild") == 0 || strcmp(s, "VTOL_MobileBuild") == 0 ||
        strcmp(s, "BuildingBuild") == 0) {
        unsigned short id = unitType;
        char buf2[0x80];
        sprintf(buf2, "UTYPENAME%4d", id);
        if (!file->HasItem(buf2) && id >= 1 && id < g_game->unitDefCount)
            file->SetStringItem(buf2, ((UnitType *)g_game->defs.get())[id].name);
    }

    if (desc.field_4 != 0) {
        char buf3[0x20];
        sprintf(buf3, "%s%s", name, "g");
        ((OrderFx*)attached)->SerializeToSave(this, file, buf3);
    }
    return 1;
}
