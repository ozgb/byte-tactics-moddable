#include "win_types.h"
#include <stdio.h>
struct Unit;
#include "unit_orders.h"
namespace std {
inline void _Construct(Unit** dest, Unit* const& src) { CopyDwordIfNonNull((int *)dest, (int *)&src); }
}
#include <vector>
#include <list>
#include <map>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include <tchar.h>
#include <time.h>
#include <float.h>
struct Unit;
struct Order;

#ifndef __clang__
namespace std {
inline void _Construct(Unit** dest, Unit* const& src) { CopyDwordIfNonNull(dest, &src); }
}
#endif

#include "../util/vec3.h"

static inline Vec3 PosMinus(const Vec3& a, const Vec3& b)
{
    Vec3 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

struct Pair16 { short lo, hi; };

#include "box.h"

union Fixed {
    int value;
    struct { unsigned short frac; short whole; } parts;
};

#include "../state/block.h"
#include "../units/cob_script.h"

#include "../files/files_api.h"
#include "../map/mission.h"
#include "../sound/sound.h"
struct BmpFileHeader;
struct BmpInfo;
struct BmpInfoHeader;
struct CalcedExplosion;
struct FrameTable;
struct Chunk;
class SquadManager;
class AssaultTimer;
class EscortTimer;
class SquadScoutTimer;
class SpatialTimer;

#include "mission_type.h"

#include "unit_motion.h"

#pragma pack(push, 1)
#include "../weapons/weapon_def.h"

struct Mover {
    char unknown_0[0xdc];
    int speed;
    char pad_e0[49];
    union {
        unsigned int f111;
        struct {
            unsigned int b0_7 : 8;
            unsigned int b8 : 1;
            unsigned int b9_31 : 23;
        } f111b;
    };
};

#include "../network/player.h"

#include "../units/unit_def.h"

#include "../map/feature.h"

#include "../game/game.h"

#include "path_order_attach.h"

#include "order.h"

#include "../units/unit.h"
#include "../units/unit_ref.h"
#pragma pack(pop)

#include "path_fx.h"

#include "air_maneuver_order.h"

#include "../ingame/control_panel.h"

class Class_0044e6c0 { public: void SetAltitude(int); };
class Class_0044e730 { public: void SetApproachRadius(int); };
class Class_0044e720 { public: void SetHeading(int); };

class GroundAllyVisitor {
public:
    virtual void CollectGroundAlly(Unit*);
    GroundAllyVisitor(Player* o, std::vector<Unit*>* u, Unit* s) : owner(o), units(u), self(s) {}
    Player* owner;
    std::vector<Unit*>* units;
    Unit* self;
};

class RepairableUnitVisitor {
public:
    virtual void CollectRepairableUnit(Unit*);
    RepairableUnitVisitor(Player* o, std::vector<Unit*>* v, Unit* s) : owner(o), units(v), self(s) {}
    Player* owner;
    std::vector<Unit*>* units;
    Unit* self;
};

class LandingPadList : public std::vector<Unit*> {
public:
    LandingPadList();
};

#include "../../link/globals.h"

#include "../util/math.h"
int __cdecl ScaledSin(short, int);
int __cdecl ScaledCos(short, int);
short __cdecl AngleOfVector(int, int);
int __stdcall GetHeadingBetween(Vec3*, Vec3*);
#include "order_list.h"
MissionType __stdcall GetOrderType(unsigned char, Unit*, Unit*, int);
#include "../../interfaces/seams.h"
void __stdcall AddOrder(MissionType, int, Unit*, Unit*, Vec3*, int, int);
void __stdcall StartBuildingScript(Unit*, Order*, short);
#include "order_queue.h"
void __stdcall EmitNanoParticles(Vec3*, Box*, int);
void __stdcall EmitReverseNanoParticles(Box*, Vec3*, int);
void __stdcall DamageUnit(Unit*, Unit*, int, int, int);
int __stdcall SendScriptCallByName(Unit*, char*, char, int, int, int, int);
void __stdcall DrawUnit(void*, Unit*);

#include "vtol_orders.h"
void WriteScreenshot(char*, char*, int, int, int, int);
void SubtractVec3(int*, int*, int*, int*, int*);
int __stdcall FindWeaponTarget(Unit*, unsigned char, int);
void __stdcall AdjustBuildCount(int kind, Unit* owner, int id, int amount);

static inline Vec3 Direction(short angle, int range)
{
    Fixed distance;
    distance.value = range;
    return DirectionFromAngle(angle, distance);
}

static inline Vec3 Offset(short angle, int distance)
{
    Vec3 v;
    v.x = -ScaledSin(angle, distance);
    v.y = 0;
    v.z = -ScaledCos(angle, distance);
    return v;
}

static inline Point16 WorldToCell(Vec3 v, Point16 origin)
{
    Point16 c;
    c.x = (v.x - (origin.x << 19) + 0x80000) >> 20;
    c.y = (v.z - (origin.y << 19) + 0x80000) >> 20;
    return c;
}

static inline Point16 WorldToCell16(Vec3 v, Point16 origin)
{
    Point16 c;
    c.x = (v.x - (origin.x << 19) + 0x80000) >> 20;
    c.y = (v.z - (origin.y << 19) + 0x80000) >> 20;
    return c;
}

static inline Point16 WorldToCellMul(Vec3 v, Point16 origin)
{
    Point16 c;
    c.x = (v.x - origin.x * 0x80000 + 0x80000) >> 20;
    c.y = (v.z - origin.y * 0x80000 + 0x80000) >> 20;
    return c;
}

static inline void CellToWorld(Point16 origin, Point16 c, Vec3* v)
{
    v->x = (origin.x + c.x * 2) << 19;
    v->z = (origin.y + c.y * 2) << 19;
}

static inline void Snap(Vec3* v, Point16 size)
{
    Point16 c = WorldToCellMul(*v, size);
    CellToWorld(size, c, v);
}

static inline int Contains(unsigned int* bits, unsigned short index)
{
    return bits[index >> 5] & (1 << (index & 31));
}

static inline void Dummy(void) {}

static inline int IsDamaged(Unit* u)
{
    return (unsigned int)u->health < (u->def->maxHealth >> 2) * 3;
}

static inline int GetSpeed(Unit* unit)
{
    return ((WeaponDef*)unit->weapons[0].weapon)->range;
}

static inline Unit* OrderTarget(Order* order)
{
    return order->target;
}

// FUNCTION: 0x413bc0
int __stdcall VtolEvadeOrder(Unit* unit,Order* order,int flags)
{
    int range=((WeaponDef*)unit->weapons[0].weapon)->range;
    if (order->target && !(flags&0x10008)) {
    unsigned int state=0; state=order->state;
    switch(state) {
    case 0:
        if (unit->motion && (unit->def->flags1&0x800)) {
            order->side=RandomInt(2);
            Vec3 pos;
        if (order->side) pos=AddVec3(unit->pos,Offset(unit->heading-0x4000,range<<16));
        else pos=AddVec3(unit->pos,Offset(unit->heading+0x4000,range<<16));
            Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,pos);
            ((Class_0044e730*)move)->SetApproachRadius(128);
            order->SetAttachedFx(move);
            order->flags=0x100e8;
            return 1;
        }
        goto invalid;
    case 1: {
        Vec3 pos;
        if (order->side) pos=AddVec3(unit->pos,Offset(unit->heading-0x4000,range<<17));
        else pos=AddVec3(unit->pos,Offset(unit->heading+0x4000,range<<17));
        Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,pos);
        ((Class_0044e730*)move)->SetApproachRadius(128);
        order->SetAttachedFx(move);
        order->flags=0x100e8;
        return 1;
    }
    case 2: break;
    default: invalid: return 7;
    }
    }
    return 5;
}

// FUNCTION: 0x40f200
void __stdcall PrepVtolClimb(Unit* unit, Order* order, unsigned int flags)
{
    g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
    if (unit->carrier)
        g_seams.units->AttachUnitToPiece(unit, 0, -1, 2);
    g_seams.units->SetStateBits(unit, 1, 1);
    if ((unit->motion->flags & 3) == 1) {
        unit->motion->SetFlightMode(unit, 2);
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, unit->pos);
        ((Class_0044e6c0*)obj)->SetAltitude(unit->def->altitude / 2);
        order->SetAttachedFx(obj);
        order->flags |= flags | 0xe0;
    }
}

// FUNCTION: 0x40f2a0
int __stdcall VtolLandIfCanOrder(Unit* unit, Order* order, int flags)
{
    if (*(int*)&order->next)
        return 5;
    if (flags & 0x40)
        return 5;
    if ((int)unit->spatialBucket.get() == (int)g_game->overflowBucket.get()) {
        Vec3 centre;
        centre.x = g_game->mapWidth / 2 << 16;
        centre.z = g_game->mapHeight / 2 << 16;
        short angle = GetHeadingBetween(&unit->pos, &centre);
        Vec3 dest = AddVec3(unit->pos, Offset(angle, 0x3200000));
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, dest);
        ((Class_0044e730*)obj)->SetApproachRadius(0x80);
        order->flags |= 0xe0;
        order->SetAttachedFx(obj);
        return 2;
    }
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0:
        if (unit->motion && (unit->def->flags1 & 0x800)) {
            if (order->pos.x == 0 && order->pos.z == 0 && order->pos.y == 0)
                order->pos = unit->pos;
            int a = RandomInt(0x10000);
            order->angle = a;
            order->parity = a & 1;
            PrepVtolClimb(unit, order, 0);
            return 1;
        }
        break;
    case 1: {
        if (g_seams.map->CanPlaceFootprintAt(unit, &unit->pos)) {
            g_seams.units->StartScript((CobScript*)unit->script.get(), "EndTransport", 0, 1);
            Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, unit->pos);
            int h = max(g_seams.units->GetGroundHeight(&unit->pos), g_game->seaLevel);
            ((Class_0044e6c0*)obj)->SetAltitude(h <= g_game->seaLevel ? g_seams.units->GetGroundHeight(&unit->pos) - g_game->seaLevel : 0);
            order->SetAttachedFx(obj);
            order->flags = 0xe0;
            g_seams.units->SetStateBits(unit, 1, 0);
            return 1;
        }
        for (int r = 0x40; r < 0x100; r += 0x10) {
            Vec3 p = unit->pos;
            p.x += (RandomInt(r * 2 + 1) - r) << 16;
            p.z += (RandomInt(r * 2 + 1) - r) << 16;
            Point16 fp = *(Point16*)&unit->footprint;
            Point16 cell = WorldToCellMul(p, fp);
            CellToWorld(fp, cell, &p);
            if (g_seams.map->CanPlaceFootprintAt(unit, &p)) {
                order->SetAttachedFx(new (kSim) Class_0044e2d0(order, p));
                order->flags = 0xe0;
                return 2;
            }
        }
        if ((unsigned char)flags & 0xe0)
            order->angle -= 0x5555;
        Vec3 off = Offset(order->angle, 0xa00000);
        Vec3 sum;
        sum.x = order->pos.x + off.x;
        sum.y = order->pos.y + off.y;
        sum.z = order->pos.z + off.z;
        Vec3 dest = sum;
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, dest);
        ((Class_0044e730*)obj)->SetApproachRadius(0x40);
        order->SetAttachedFx(obj);
        order->flags |= 0xe0;
        return 2;
    }
    case 2:
        if (flags & 0x20) {
            unit->motion->SetFlightMode(unit, 1);
            return 5;
        }
        return 8;
    }
    return 7;
}

#pragma auto_inline(off)
#pragma auto_inline(on)

// FUNCTION: 0x40f7d0
int __stdcall VtolStandbyOrder(Unit* unit, Order* order, int flags)
{
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0:
        if (unit->motion && (unit->def->flags1 & 0x800)) {
            g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
            order->flags |= 0x10000;
            order->SetDeadlineTicks(1);
            order->x = order->source->xWhole;
            order->z = order->source->zWhole;
            return 1;
        }
        break;
    case 1: {
        Unit* other = g_seams.targeting->FindBestTargetIfFireAtWill(unit);
        if (other && IssueAttackOrder(unit, other, 0)) {
            order->flags = 0;
            order->state = 0;
            return 3;
        }
        return 1;
    }
    case 2:
        if ((unit->def->flags1 & 0x800) && (unit->flags & 3) == 2) {
            if (unit->cargo) {
                short angle = RandomInt(0x10000);
                Vec3 p;
                p.x = order->x << 16;
                p.y = 0;
                p.z = order->z << 16;
                int distance = (RandomInt(0x20) + 8) << 16;
                p += Offset(angle, distance);
                Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, p);
                ((Class_0044e6c0*)obj)->SetAltitude(unit->def->altitude);
                order->SetAttachedFx(obj);
                order->SetDeadlineTicks(RandomInt(0xf) + 0x1e);
                order->state = 1;
                return 2;
            }
            AppendOrder(unit, new (kSim) Order(MissionType("VTOL_LANDIFCAN"), 0, &order->pos, 0, 0, 0));
            return 5;
        }
        order->flags |= 0x10000;
        order->SetDeadlineTicks(RandomInt(0x1e) + 0x1e);
        order->state = 1;
        return 2;
    }
    return 7;
}

// FUNCTION: 0x40fa20
int __stdcall VtolMoveOrder(Unit* unit, Order* order, int flags)
{
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0:
        if (unit->motion && (unit->def->flags1 & 0x800)) {
            PrepVtolClimb(unit, order, 0);
            return 1;
        }
        break;
    case 1:
        order->AnnounceStatusIfFlagged(0);
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        Snap(&order->pos, *(Point16*)&unit->footprint);
        order->SetAttachedFx(new (kSim) Class_0044e2d0(order, order->pos));
        order->flags = 0xe0;
        return 1;
    case 2:
        if (!order->next)
            QueueUnitSpeech(unit, 6, 0);
        return 5;
    }
    return 7;
}

#pragma auto_inline(off)
// FUNCTION: 0x4103a0
Vec3 __stdcall DirectionFromAngle(short angle, Fixed scale)
{
    Vec3 v;
    v.x = -ScaledSin(angle, scale.value);
    v.y = 0;
    v.z = -ScaledCos(angle, scale.value);
    return v;
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x410830
LandingPadList::LandingPadList()
{
    unsigned char local;
    ((unsigned char*)this)[0] = local;
    _First = 0;
    _Last = 0;
    _End = 0;
}
#pragma auto_inline(on)

class UnitList : public std::vector<Unit*> {};

static inline void FindPads(Unit* u, std::vector<Unit*>* pads)
{
    g_seams.ai->GetFactoriesInRadius(((Player*)u->player)->index,&u->pos,0xf00,pads);
}

static inline int SearchRange(Unit* u)
{
    return u->def->range<<16;
}

static inline int Patrol(Unit* unit, Order* order, int flags)
{
    if (IsDamaged(unit)) {
        LandingPadList pads;
        FindPads(unit,&pads);
        if (!pads.empty()) {
            order->SetAttachedFx(0);
            Unit* pad=pads[RandomInt(pads.size())];
            AppendOrder(unit,new (kSim) Order("VTOL_LANDING",pad,0,0,0,0));
            order->flags=0;
            return 0;
        }
    }
    UnitList units;
    int range=SearchRange(unit);
    GroundAllyVisitor visitor((Player*)unit->player,&units,unit);
    g_seams.map->VisitObjectsInRange(&unit->pos,range,(UnitVisitor*)&visitor);
    if (!units.empty()) {
        order->SetAttachedFx(0);
        MissionType kind=GetOrderType(7,unit,units[0],0);
        AppendOrder(unit,new (kSim) Order(kind,units[0],0,0,0,0));
        order->flags=0;
        return 3;
    }
    if (flags&0xe0) order->angle+=-RandomInt(0x2000)-0x4000;
    Vec3 pos=AddVec3(order->pos,Offset((short)order->angle,(((WeaponDef*)unit->weapons[0].weapon)->range+160)<<16));
    Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,pos);
    ((Class_0044e730*)move)->SetApproachRadius(128);
    order->SetAttachedFx(move);
    order->SetDeadlineTicks(30);
    order->flags|=0xf8;
    return 2;
}

static inline int BelowSeaLevel(Order* order)
{
    Unit* target = order->target;
    return target->def->modelMaxY + target->pos.y <= g_game->seaLevel << 16;
}

// FUNCTION: 0x411560
int __stdcall VtolUnloadOrder(Unit* unit, Order* order, int flags)
{
    if (!unit->cargo)
        return 5;
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0:
        if (unit->motion && (unit->def->flags1 & 0x800)) {
            order->AnnounceStatusIfFlagged("Unloading");
            order->link.SetUnit(unit->cargo);
            Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, order->pos);
            ((Class_0044e6c0*)obj)->SetAltitude(unit->def->altitude);
            ((Class_0044e730*)obj)->SetApproachRadius(0x140);
            order->SetAttachedFx(obj);
            order->flags = 0xe8;
            return 1;
        }
        break;
    case 1: {
        Unit* cargo = order->target;
        if (g_seams.map->CanPlaceUnitFootprint(cargo->def, 0, WorldToCell16(order->pos, *(Point16*)&cargo->footprint), 1)) {
            Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, order->pos);
            int h = unit->cargo->def->modelMaxYHigh;
            ((Class_0044e6c0*)obj)->SetAltitude(h);
            order->SetAttachedFx(obj);
            order->flags = 0xe8;
            return 1;
        }
        QueueUnitSpeech(unit, 7, "Unable to unload unit");
        return 9;
    }
    case 2: {
        if (flags & 0x40)
            return 9;
        Unit* cargo = order->target;
        if (!g_seams.map->CanPlaceUnitFootprint(cargo->def, 0, WorldToCell16(order->pos, *(Point16*)&cargo->footprint), 1)) {
            QueueUnitSpeech(unit, 7, "Unable to unload unit");
            return 9;
        }
        g_seams.units->StartScript((CobScript*)unit->script.get(), "EndTransport", 0, 0);
        g_seams.units->AttachUnitToPiece(unit->cargo, 0, -1, 1);
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, unit->pos);
        ((Class_0044e6c0*)obj)->SetAltitude(unit->def->altitude);
        order->SetAttachedFx(obj);
        order->flags = 0xe0;
        return 1;
    }
    case 3:
        QueueUnitSpeech(unit, 0xd, 0);
        return 5;
    }
    return 7;
}

// FUNCTION: 0x411840
int __stdcall FindLandingPad(Unit* unit, int pad)
{
    if (pad != -1 && g_seams.map->IsPadSlotFree(unit, pad)) {
        return pad;
    }
    int pads[4];
    pads[0] = -1;
    pads[1] = -1;
    pads[2] = -1;
    pads[3] = -1;
    g_seams.units->QueryScript((CobScript*)unit->script.get(), "QueryLandingPad", &pads[0], &pads[1], &pads[2], &pads[3]);
    for (int i = 0; i < 4; i++) {
        if (pads[i] != -1 && g_seams.map->IsPadSlotFree(unit, pads[i])) {
            return pads[i];
        }
    }
    return -1;
}

static inline int IsAhead(Unit* unit, Order* order)
{
    Vec3 toward = Offset(GetHeadingBetween(&unit->pos, &order->target->pos), 0x140000);
    Fixed dist;
    dist.value = 0x140000;
    Vec3 facing = DirectionFromAngle(unit->heading, dist);
    return (short)(((Vec3Words*)&toward)->xWhole * ((Vec3Words*)&facing)->xWhole + ((Vec3Words*)&toward)->zWhole * ((Vec3Words*)&facing)->zWhole) > 0;
}

// FUNCTION: 0x413d80
int __stdcall VtolMobileBuildOrder(Unit* unit,Order* order,int flags)
{
    if (flags&2) { MarkSelectionOrdersDirty(unit); return 5; }
    if (flags&8) { QueueUnitSpeech(unit,7,"Construction terminated"); MarkSelectionOrdersDirty(unit); return 8; }
    unsigned int state=0; state=order->state;
    switch(state) {
    case 0:
        if (unit->motion && (unit->def->flags1&0x800)) {
            order->AnnounceStatusIfFlagged("Building");
            g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
            if (unit->carrier) g_seams.units->AttachUnitToPiece(unit,0,-1,2);
            g_seams.units->SetStateBits(unit, 1,1);
            if ((unit->motion->flags&3)==1) {
                unit->motion->SetFlightMode(unit,2);
                Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,unit->pos);
                ((Class_0044e6c0*)move)->SetAltitude(unit->def->altitude/2);
                order->SetAttachedFx(move);
                order->flags|=0xe0;
            }
            return 1;
        }
        break;
    case 1: {
        UnitDef* def=&g_game->defs[order->type];
        order->retries=0;
        Point16 origin=*(Point16*)&def->footprintx;
        Point16 cell=WorldToCell(order->pos,origin);
        CellToWorldPos(cell,&order->pos,origin);
        Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,order->pos);
        ((Class_0044e730*)move)->SetApproachRadius(unit->def->buildRange);
        order->SetAttachedFx(move);
        order->flags=0xe0;
        return 1;
    }
    case 2: {
        if (flags&0x40) return 8;
        UnitDef* def=&g_game->defs[order->type];
        if (!g_seams.map->CanPlaceUnitFootprint(def,0,WorldToCell16(order->pos,*(Point16*)&g_game->defs[order->type].footprintx),1)) {
            if (!order->retries) QueueUnitSpeech(unit,7,"Waiting for target area to clear");
            else if (order->retries>10) { QueueUnitSpeech(unit,7,"Target area was blocked"); return 8; }
            ++order->retries;
            order->SetDeadlineTicks(30);
            return 2;
        }
        g_seams.map->SnapWorldPosToFootprint(def,&order->pos);
        order->link.SetUnit(g_seams.units->CreateUnit(unit->playerIndex,(short)order->type,order->pos,0,1,0));
        if (!order->target) { QueueUnitSpeech(unit,7,"Unable to create any more units"); return 8; }
        QueueUnitSpeech(unit,9,"Starting construction");
        AddOrder("GETBUILT",1,order->target,unit,0,0,0);
        StartBuildingScript(unit,order,AngleOfVector(unit->pos.x-order->target->pos.x,unit->pos.z-order->target->pos.z)-unit->heading);
        MarkSelectionOrdersDirty(unit);
        return 1;
    }
    case 3:
        WaitIfNotInBuildStance(unit,order,10);
    case 4: {
        if ((unsigned int)g_game->ticks%150==0) {
            int angle=GetHeadingBetween(&unit->pos,&order->target->pos);
            int range=unit->def->buildRange<<16;
            angle+=0xdb6e;
            Vec3 pos=PosMinus(order->target->pos,Offset(angle,range));
            Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,pos);
            ((Class_0044e720*)move)->SetHeading((unsigned short)angle);
            order->SetAttachedFx(move);
        }
        int rate=0; rate=unit->def->buildRate;
        if (g_seams.units->AddBuildProgress(unit,order->target,(float)(rate/30))) {
            Vec3 start;
            g_seams.units->GetNanoPiecePosition(unit,&start);
            Vec3 bounds[2];
            bounds[0]=AddVec3(order->target->pos,*(Vec3*)&order->target->def->modelMinX);
            bounds[1]=AddVec3(order->target->pos,*(Vec3*)&order->target->def->modelMaxX);
            EmitNanoParticles(&start,(Box*)bounds,6);
        }
        if (order->target->buildLeft!=0.0f) {
            order->SetDeadlineTicks(1);
            order->flags|=0xa;
            return 2;
        }
        return 1;
    }
    case 5:
        QueueUnitSpeech(unit,8,"Building complete");
        return 5;
    }
    return 7;
}

#pragma auto_inline(off)
// FUNCTION: 0x414350
void __stdcall CellToWorldPos(Point16 a, Vec3* out, Point16 b)
{
    out->x = (b.x + a.x * 2) << 19;
    out->z = (b.y + a.y * 2) << 19;
}
#pragma auto_inline(on)


// FUNCTION: 0x414e70
int __stdcall VtolRepairUnitOrder(Unit* unit, Order* order, int flags)
{
    if (!order->target) {
        QueueUnitSpeech(unit, 7, "Repairs unsuccessful.");
        return 8;
    }
    if (order->range && (int)_hypot(unit->xWhole - order->x, unit->zWhole - order->z) >= order->range)
        return 5;
    if (order->target->mode != 1) {
        QueueUnitSpeech(unit, 7, "Repairs unsuccessful.");
        return 5;
    }
    Vec3* dst = &order->pos;
    *dst = order->target->pos;
    switch (order->state) {
    case 0:
        if (unit->motion && (unit->def->flags1 & 0x800)) {
            if (!g_seams.units->CanRepair(unit, 0, order->target)) {
                QueueUnitSpeech(unit, 7, "Repair mission failed");
                return 8;
            }
            order->AnnounceStatusIfFlagged("Repairing");
            PrepVtolClimb(unit, order, 0);
            return 1;
        }
        break;
    case 1: {
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, order->pos);
        ((Class_0044e6c0*)obj)->SetAltitude(unit->def->altitude);
        order->SetAttachedFx(obj);
        order->flags = 0xe8;
        return 1;
    }
    case 2:
        if (flags & 0x40)
            return 8;
        if (order->target->mode != 1)
            return 8;
        if (order->target->flags & 0xc) {
            order->SetDeadlineTicks(15);
            return 0;
        }
        if ((unsigned int)order->target->health >= order->target->def->maxHealth)
            return 1;
        {
            g_seams.units->AddRepairProgress(unit, order->target, (float)(unit->def->buildRate / 30));
            Vec3 nano;
            g_seams.units->GetNanoPiecePosition(order->source, &nano);
            Box box;
            box.hi = order->target->pos;
            box.lo = order->target->pos;
            box.lo.x += order->target->def->modelMinX;
            box.lo.z += order->target->def->modelMinZ;
            box.hi.x += order->target->def->modelMaxX;
            box.hi.z += order->target->def->modelMaxZ;
            box.hi.y += order->target->def->modelMaxY;
            EmitNanoParticles(&nano, &box, 6);
        }
        order->SetDeadlineTicks(1);
        order->flags |= 8;
        return 2;
    case 3:
        QueueUnitSpeech(unit, 10, "Unit repaired");
        return 5;
    }
    return 7;
}

// FUNCTION: 0x415250
int __stdcall VtolGetRepairedOrder(Unit* unit, Order* order, int unused)
{
    if (order->target == 0) {
        QueueUnitSpeech(unit, 7, "Repair aborted.");
        return 8;
    }
    switch (order->state) {
    case 0:
        if (unit->health >= unit->def->maxHealth)
            return 1;
        order->SetDeadlineTicks(0x1e);
        order->groups.started = 1;
        return 2;
    case 1:
        QueueUnitSpeech(unit, 10, "Unit repaired");
        return 5;
    default:
        return 7;
    }
}

static inline int Land(Unit* unit, Order* order)
{
    if ((unsigned int)unit->health < (unit->def->maxHealth >> 2) * 3) {
        std::vector<Unit*> pads;
        g_seams.ai->GetFactoriesInRadius(((Player*)unit->player)->index, &unit->pos, 0xf00, &pads);
        if (!pads.empty()) {
            order->SetAttachedFx(0);
            Unit* pad = pads[RandomInt(pads.size())];
            AppendOrder(unit, new (kSim) Order("VTOL_LANDING", pad, 0, 0, 0, 0));
            order->flags = 0;
            return 1;
        }
    }
    do {} while (0);
    return 0;
}

static inline float Total(float base, float amount)
{
    float value = base;
    value += amount;
    return value;
}

// FUNCTION: 0x4158d0
void RepairableUnitVisitor::CollectRepairableUnit(Unit* unit)
{
    if (unit == self) return;
    unsigned int index = 0;
    index = ((Player*)unit->player)->index;
    if (!owner->allied[index]) return;
    unsigned int kind = unit->flags & 3;
    if ((unsigned char)kind != 1) return;
    if (unit->health >= unit->def->maxHealth && unit->buildLeft == 0.0f) return;
    if (unit->lastAttackerSlot == owner->index && unit->lastDamageType == 5) return;
    units->push_back(unit);
}

static inline short Whole(const int& v) { return ((short*)&v)[1]; }

static inline Vec3 Sum(const Vec3& a, const Vec3& b)
{
    Vec3 r = a;
    r.x += b.x; r.y += b.y; r.z += b.z;
    return r;
}

static inline Vec3 Diff(const Vec3& a, const Vec3& b)
{
    Vec3 r = a;
    r.x -= b.x; r.y -= b.y; r.z -= b.z;
    return r;
}

// FUNCTION: 0x413470
int __stdcall AirToGroundHoverOrder(Unit* unit, Order* order, int flags)
{
    if (flags & 0x10008) {
        if (order->next == 0 && (unit->flags & 0x300000))
            AppendOrderToTail(unit, new (kSim) Order("VTOL_SEEKATTACK", order->target, &order->pos, 0, 0, 0));
        return 5;
    }
    if (order->target == 0 && (order->flags_42 & 0x200)) {
        if (order->next == 0)
            AppendOrderToTail(unit, new (kSim) Order("VTOL_SEEKATTACK", 0, &unit->pos, 0, 0, 0));
        return 5;
    }
    if ((int)unit->spatialBucket.get() == (int)g_game->overflowBucket.get()) {
        Vec3 centre;
        centre.x = g_game->mapWidth / 2 << 16;
        centre.z = g_game->mapHeight / 2 << 16;
        short angle = GetHeadingBetween(&unit->pos, &centre);
        Vec3 dest = AddVec3(unit->pos, Offset(angle, 0x3200000));
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, dest);
        ((Class_0044e730*)obj)->SetApproachRadius(0x80);
        order->flags |= 0xe0;
        order->SetAttachedFx(obj);
        return 2;
    }
    short& ox = order->x;
    short& oz = order->z;
    if (order->range && (int)_hypot(unit->xWhole - ox, unit->zWhole - oz) >= order->range)
        return 5;
    int range = ((WeaponDef*)unit->weapons[0].weapon)->range;
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0:
        if (unit->motion && (unit->def->flags1 & 0x800)) {
            order->AnnounceStatusIfFlagged("Attacking");
            PrepVtolClimb(unit, order, 0);
            return 1;
        }
        break;
    case 1: {
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        int dist = (int)_hypot(order->target->pos.x - unit->pos.x, order->target->pos.z - unit->pos.z);
        int angle = GetHeadingBetween(&unit->pos, &order->target->pos);
        Vec3 off = Offset(RandomInt(0x4000) + angle - 0x2000, dist / 2);
        Vec3 p = Sum(unit->pos, off);
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, p);
        ((Class_0044e730*)obj)->SetApproachRadius(0x80);
        order->SetAttachedFx(obj);
        order->flags = 0x100e8;
        return 1;
    }
    case 2: {
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 0);
        g_seams.targeting->SetWeaponTargetUnit(unit, order->target, 0);
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, order->target->pos);
        ((Class_0044e730*)obj)->SetApproachRadius(range);
        order->SetAttachedFx(obj);
        order->flags = 0x100e8;
        order->side = 0;
        order->misses = 0;
        return 1;
    }
    case 3: {
        if (!g_seams.weapons->WeaponCanReachUnit(unit, order->target, 0))
            order->misses++;
        if (order->misses >= 2) {
            order->misses = 0;
            Vec3 off = Offset(RandomInt(0x10000), range << 16);
            Vec3 p = AddVec3(order->target->pos, off);
            Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, p);
            ((Class_0044e730*)obj)->SetApproachRadius(0x80);
            order->flags |= 0x110e8;
            return 2;
        }
        int a = GetHeadingBetween(&unit->pos, &order->target->pos);
        int angle;
        if (order->side) {
            angle = a + 0x2000;
            order->side = 0;
        } else {
            angle = a - 0x2000;
            order->side = 1;
        }
        Vec3 off = Offset(angle, range * 2 / 3 << 16);
        Vec3 p = Diff(order->target->pos, off);
        Class_0044e330* obj = new (kSim) Class_0044e330(order, order->target, p);
        ((Class_0044e730*)obj)->SetApproachRadius(0x10);
        ((Class_0044e6c0*)obj)->SetAltitude(unit->def->altitude);
        order->SetAttachedFx(obj);
        order->flags = 0x100e8;
        if ((unsigned int)unit->health < (unit->def->maxHealth >> 2) * 3) {
            std::vector<Unit*> v;
            g_seams.ai->GetFactoriesInRadius(((Player*)unit->player)->index, &unit->pos, 0xf00, &v);
            if (!v.empty()) {
                order->SetAttachedFx(0);
                Unit* target = v[RandomInt(v.size())];
                AppendOrder(unit, new (kSim) Order("VTOL_LANDING", target, 0, 0, 0, 0));
                order->flags = 0;
                return 0;
            }
        }
        return 2;
    }
    }
    return 7;
}

// FUNCTION: 0x4111b0
int __stdcall VtolPickupOrder(Unit* unit, Order* order, int flags)
{
    Unit* target = order->target;
    if (target && !(flags & 0x10048)) {
        if (BelowSeaLevel(order)) {
            QueueUnitSpeech(unit, 7, "Transport mission failed");
            return 8;
        }
        if (unit->cargo)
            return 8;
        switch (order->state) {
        case 0:
            if (unit->motion && (unit->def->flags1 & 0x800)) {
                if (target->footprint.x > (short)unit->def->capacity) {
                    QueueUnitSpeech(unit, 7, "Unit is too heavy to transport");
                    return 8;
                }
                order->AnnounceStatusIfFlagged("Loading");
                PrepVtolClimb(unit, order, 0);
                return 1;
            }
            break;
        case 1: {
            Class_0044e190* obj = new (kSim) Class_0044e190(order, order->target);
            ((Class_0044e6c0*)obj)->SetAltitude(unit->def->altitude);
            ((Class_0044e730*)obj)->SetApproachRadius(0x30);
            order->SetAttachedFx(obj);
            order->flags = 0x100e8;
            return 1;
        }
        case 2:
            order->AnnounceStatusIfFlagged("Preparing for transport");
            order->piece = -1;
            g_seams.units->QueryScript((CobScript*)unit->script.get(), "QueryTransport", &order->piece, 0, 0, 0);
            order->flags = 0x100e8;
            return 1;
        case 3: {
            int height = target->def->modelMaxY;
            g_seams.units->StartScriptWithArgs((CobScript*)unit->script.get(), "BeginTransport", 0, 1, 1, height, 0, 0, 0);
            SendScriptCallByName(unit, "BeginTransport", 1, height, 0, 0, 0);
            Vec3 offset = g_seams.units->GetPieceOffset(unit, order->piece);
            Class_0044e250* obj = new (kSim) Class_0044e250(order, order->target, -1);
            ((Class_0044e6c0*)obj)->SetAltitude(-((Vec3Words*)&offset)->yWhole);
            order->SetAttachedFx(obj);
            order->flags = 0x100ea;
            return 1;
        }
        case 4: {
            if (flags & 0x42) {
                g_seams.units->StartScript((CobScript*)unit->script.get(), "EndTransport", 0, 0);
                return 8;
            }
            g_seams.units->AttachUnitToPiece(target, unit, order->piece, 0);
            QueueUnitSpeech(unit, 12, 0);
            Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, unit->pos);
            ((Class_0044e6c0*)obj)->SetAltitude(unit->def->altitude);
            order->flags |= 0xe0;
            return 1;
        }
        case 5:
            return 5;
        }
        return 7;
    }
    QueueUnitSpeech(unit, 7, "Transport mission failed");
    return 8;
}

struct Unit;
struct Order;




// FUNCTION: 0x40fbe0
int __stdcall VtolFollowOrder(Unit* unit, Order* order, int flags)
{
    if (order->target && !(flags&0x48)) {
        if ((int)unit->spatialBucket.get() == (int)g_game->overflowBucket.get()) {
            Vec3 center;
            center.x=(g_game->mapWidth/2)<<16;
            center.z=(g_game->mapHeight/2)<<16;
            short angle=GetHeadingBetween(&unit->pos,&center);
            Vec3 pos=AddVec3(unit->pos,Direction(angle,0x3200000));
            Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,pos);
            move->SetApproachRadius(128);
            order->flags|=0xe0;
            order->SetAttachedFx(move);
            return 2;
        }
        Vec3* dst=&order->pos;
        *dst=order->target->pos;
        switch(order->state) {
        case 0:
            if (unit->motion && (unit->def->flags1&0x800)) {
                order->AnnounceStatusIfFlagged("Guarding");
                g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
                if (unit->carrier) g_seams.units->AttachUnitToPiece(unit,0,-1,2);
                g_seams.units->SetStateBits(unit, 1,1);
                if ((unit->motion->flags&3)==1) {
                    unit->motion->SetFlightMode(unit,2);
                    Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,unit->pos);
                    move->SetAltitude(unit->def->altitude/2);
                    order->SetAttachedFx(move);
                    order->flags|=0xe0;
                }
                order->angle=RandomInt(0x10000);
                order->parity=order->angle&1;
                return 1;
            }
            return 7;
        case 1:
            g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
            return 1;
        case 2: {
            Unit* attacker=order->target->attacker;
            if (attacker && !attacker->player.get()->allied[unit->player.get()->index]) {
                if (flags & 0x10) {
                    if (!Contains(unit->def->categories,attacker->unitDefIndex)) {
                        if (IssueAttackOrder(unit,attacker,1)) { order->flags=0; return 3; }
                        if (unit->flags & 0x300000) {
                            unsigned char i=0;
                            UnitWeaponSlot* weapon=unit->weapons;
                            do {
                                if (!((weapon->flags&2) && weapon->flags&0x10 && ((unsigned char)(((WeaponDef*)weapon->weapon)->flags.value >> 26)&1) == 0)) {
                                } else {
                                    Unit* target=g_seams.targeting->GetWeaponTargetUnit(unit,i);
                                    if (!target || !g_seams.weapons->WeaponCanReachUnit(unit,target,i) || Contains(unit->def->weaponCategories[i],target->unitDefIndex))
                                        g_seams.targeting->SetWeaponTargetUnit(unit,attacker,i);
                                }
                                ++i;
                                weapon++;
                            } while (i<3);
                        }
                    }
                }
            }
            if (g_seams.units->CanRepair(unit, 0, order->target)) {
                MissionType kind=GetOrderType(8,unit,order->target,0);
                if (kind.index) {
                    order->SetAttachedFx(0);
                    AppendOrder(unit,new (kSim) Order(kind,order->target,0,0,0,0));
                    order->flags=0; return 3;
                }
            }
            if (order->target->list && order->target->list->kind.index && (unit->def->flags1&0x40) &&
                g_seams.units->CanRepair(unit, 0, order->target->list->target) &&
                (order->target->def->flags1&0x40) && order->target->list &&
                (order->target->list->flags_42&0x100000) && unit!=order->target->list->target) {
                int building = order->target->list->kind=="MobileBuild" || order->target->list->kind=="BuildingBuild" || order->target->list->kind=="VTOL_MobileBuild";
                int actionable = ((order->target->list->flags_42&0x200) && order->target->list->target) || (order->target->list->flags_42&0x400);
                MissionType kind;
                if (!building && actionable) {
                    kind=order->target->list->kind;
                    if (kind=="REPAIRUNIT") kind=MissionType("VTOL_REPAIRUNIT");
                    if (kind=="RECLAIM") kind=MissionType("VTOL_RECLAIM");
                    if (kind=="RECLAIMUNIT") kind=MissionType("VTOL_RECLAIMUNIT");
                    if (kind=="HELPBUILD") kind=MissionType("VTOL_HELPBUILD");
                    order->SetAttachedFx(0);
                    AppendOrder(unit,new (kSim) Order(kind,OrderTarget(order)->list->target,&order->target->list->pos,0,0,0));
                    order->flags=0; return 3;
                }
                if (building && order->target->list->target) {
                    order->SetAttachedFx(0);
                    kind=MissionType("VTOL_HelpBuild");
                    AppendOrder(unit,new (kSim) Order(kind,OrderTarget(order)->list->target,&order->target->list->pos,0,0,0));
                    order->flags=0; return 3;
                }
            }
            if (flags&0xe0) order->angle += -RandomInt(0x2000)-0x4000;
            short angle=order->angle;
            Vec3 pos;
            if ((unsigned char)(unit->flags>>31)&1)
                pos=AddVec3(order->target->pos,Offset(angle,(((WeaponDef*)unit->weapons[0].weapon)->range+160)<<16));
            else pos=AddVec3(order->target->pos,Offset(angle,0x1400000));
            Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,pos);
            move->SetApproachRadius(128);
            order->SetAttachedFx(move);
            order->SetDeadlineTicks(30);
            order->flags|=0xf8;
            return 2;
        }
        }
        return 7;
    }
    if (!order->next) AppendOrderToTail(unit,new (kSim) Order("VTOL_SEEKGUARD",order->target,&order->pos,0,0,0));
    return 5;
}

// FUNCTION: 0x410c70
void GroundAllyVisitor::CollectGroundAlly(Unit* unit)
{
    if (unit->player.get()->allied[owner->index] && !(unit->def->flags1 & 0x800) && unit != self)
        units->push_back(unit);
}




struct Unit;




// FUNCTION: 0x40f790
Vec3 __stdcall AddVec3(const Vec3& a, const Vec3& b)
{
    Vec3 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

// FUNCTION: 0x410e70
int __stdcall VtolPatrolOrder(Unit* unit, Order* order, int flags)
{
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 2: {
        if (flags & 0xe0)
            return 6;
        short angle = GetHeadingBetween(&unit->pos, &order->pos);
        Vec3 dest = AddVec3(order->pos, Offset(angle, 0x1400000));
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, dest);
        ((Class_0044e730*)obj)->SetApproachRadius(0x150);
        order->SetAttachedFx(obj);
        order->flags |= 0xe0;
        if ((unsigned int)unit->health < (unit->def->maxHealth >> 2) * 3) {
            std::vector<Unit*> v;
            g_seams.ai->GetFactoriesInRadius(unit->player->index, &unit->pos, 0xf00, &v);
            if (!v.empty()) {
                order->SetAttachedFx(0);
                Unit* target = v[RandomInt(v.size())];
                AppendOrder(unit, new (kSim) Order("VTOL_LANDING", target, 0, 0, 0, 0));
                order->flags = 0;
                return 0;
            }
        }
        Dummy();
        Dummy();
        Dummy();
        Dummy();
        Dummy();
        Dummy();
        Dummy();
        Dummy();
        Dummy();
        Dummy();
        Dummy();
        Dummy();
        Dummy();
        Dummy();
        Dummy();
        Dummy();
        Unit* next = g_seams.targeting->FindBestTargetIfFireAtWill(unit);
        if (next && IssueAttackOrder(unit, next, 0)) {
            order->flags = 0;
            return 3;
        }
        order->SetDeadlineTicks(0x1e);
        return 2;
    }
    case 1:
        order->subFlags &= ~0xe0;
        return 1;
    case 0:
        if (unit->motion && (unit->def->flags1 & 0x800)) {
            EnsurePatrolReturnOrder(unit, order);
            order->AnnounceStatusIfFlagged("Patrolling");
            PrepVtolClimb(unit, order, 0);
            g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
            return 1;
        }
        break;
    }
    return 7;
}



struct Unit;


struct BmpFileHeader;
struct BmpInfo;
struct BmpInfoHeader;
struct CalcedExplosion;



// FUNCTION: 0x4118e0
int __stdcall VtolLandingOrder(Unit* unit, Order* order, int flags)
{
    int radius = ((WeaponDef*)unit->weapon)->range;
    Unit* host = order->target;
    if (!host) {
        QueueUnitSpeech(unit, 7, "Landing aborted");
        return 8;
    }
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0:
        if (unit->motion && (unit->def->flags1 & 0x800)) {
            order->AnnounceStatusIfFlagged("Landing");
            PrepVtolClimb(unit, order, 0);
            order->angle = RandomInt(0x10000);
            return 1;
        }
        break;
    case 1: {
        if (FindLandingPad(order->target, FindLandingPad(host, -1)) != -1) {
            order->state = 2;
            return 2;
        }
        Vec3 dest = AddVec3(order->target->pos, Offset(order->angle, radius << 16));
        order->angle += 0x4000;
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, dest);
        obj->SetApproachRadius(0x80);
        order->SetAttachedFx(obj);
        order->flags = 0xe8;
        order->state = 1;
        return 2;
    }
    case 2: {
        Class_0044e250* obj = new (kSim) Class_0044e250(order, order->target, -1);
        ((Class_0044e2d0*)obj)->SetApproachRadius(0xa0);
        order->SetAttachedFx(obj);
        order->flags = 0xe8;
        return 1;
    }
    case 3: {
        int pad = FindLandingPad(host, -1);
        order->angle = pad;
        if (pad == -1) {
            QueueUnitSpeech(unit, 7, "Landing failed");
            return 0;
        }
        Class_0044e250* obj = new (kSim) Class_0044e250(order, order->target, order->angle);
        ((Class_0044e2d0*)obj)->SetApproachRadius(0x30);
        order->SetAttachedFx(obj);
        order->flags = 0xe8;
        return 1;
    }
    case 4:
        return 1;
    case 5: {
        if (flags & 0x20)
            return 1;
        int pad = FindLandingPad(host, order->angle);
        order->angle = pad;
        if (pad == -1) {
            QueueUnitSpeech(unit, 7, "Landing aborted: all pads are occupied");
            return 0;
        }
        Class_0044e250* obj = new (kSim) Class_0044e250(order, order->target, order->angle);
        if (unit->cargo)
            ((Class_0044e2d0*)obj)->SetAltitude(unit->cargo->def->modelMaxYHigh);
        else
            ((Class_0044e2d0*)obj)->SetAltitude(0);
        g_seams.units->StartScript((CobScript*)unit->script.get(), "EndTransport", 0, 1);
        order->SetAttachedFx(obj);
        order->SetDeadlineTicks(0xf);
        order->state = 5;
        order->flags |= 0xe8;
        return 2;
    }
    case 6: {
        int dropped = 0;
        if (flags & 0x40)
            return 8;
        if (!g_seams.map->IsPadSlotFree(host, order->angle)) {
            QueueUnitSpeech(unit, 7, "Landing aborted: no pads available");
            return 0;
        }
        if (unit->cargo) {
            g_seams.units->StartScript((CobScript*)unit->script.get(), "EndTransport", 0, 0);
            g_seams.units->AttachUnitToPiece(unit->cargo, order->target, order->angle, 0);
            dropped = 1;
        } else {
            g_seams.units->AttachUnitToPiece(unit, order->target, order->angle, 0);
        }
        if (!dropped && unit->health < unit->def->maxHealth
            && (order->target->def->flags1 & 0x200)
            && (order->target->def->flags1 & 0x40)
            && order->target->buildLeft == 0.0f) {
            order->SetAttachedFx(0);
            AppendOrder(unit, new (kSim) Order("SELFREPAIR", order->target, 0, 0, 0, 0));
        }
        return 5;
    }
    }
    return 7;
}


struct Unit;

#pragma pack(push, 1)
struct Struct_Unit96 {
    char pad0[0x146]; unsigned char slotIndex;
};
struct Struct_Game391e9 {
    char pad0[0xd3c]; int gravity;
};
#pragma pack(pop)



static inline void AttachMove(Order* order, const Vec3& dest, short radius);

// FUNCTION: 0x411f50
int __stdcall AirStrikeOrder(Unit* unit, Order* order, unsigned int flags)
{
    if (flags & 0x1000a) {
        if (!order->next && (unit->flags & 0x300000))
            AppendOrderToTail(unit, new (kSim) Order("VTOL_SEEKATTACK", order->target, &order->pos, 0, 0, 0));
        return 5;
    }
    Unit* target = order->target;
    if (!target && (order->flags_42 & 0x200)) {
        if (!order->next)
            AppendOrderToTail(unit, new (kSim) Order("VTOL_SEEKATTACK", 0, &unit->pos, 0, 0, 0));
        return 5;
    }
    if (target)
        order->pos = target->pos;
    if (order->range && (int)_hypot(unit->xWhole - order->start.x, unit->zWhole - order->start.y) >= order->range)
        return 5;
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0:
        if (unit->motion && (unit->def->flags1 & 0x800)) {
            order->AnnounceStatusIfFlagged("Attacking");
            PrepVtolClimb(unit, order, 0);
            return 1;
        }
        break;
    case 1: {
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 0);
        Vec3* op;
        Vec3* up;
        op = &order->pos;
        up = &unit->pos;
        if ((int)_hypot(order->pos.x - unit->pos.x, order->pos.z - unit->pos.z) < 0x1e00000) {
            int angle = GetHeadingBetween(up, op);
            Vec3 dest = AddVec3(unit->pos, Offset(angle, 0x8c00000));
            AttachMove(order, dest, 0x3c0);
            order->flags |= 0xe2;
            return 1;
        }
        return 1;
    }
    case 2: {
        Vec3* op2;
        Vec3* up2;
        op2 = &order->pos;
        up2 = &unit->pos;
        int dist = (int)_hypot(order->pos.x - unit->pos.x, order->pos.z - unit->pos.z);
        int angle = GetHeadingBetween(up2, op2);
        int radius = dist / 2;
        Vec3 dest = AddVec3(unit->pos, Offset(RandomInt(0x4000) + angle - 0x2000, radius));
        AttachMove(order, dest, 0x1e0);
        order->flags = 0x100e8;
        return 1;
    }
    case 3:
        return 1;
    case 4: {
        if (flags & 0xe0)
            return 1;
        UnitDef* def = unit->def;
        int size = def->altitude;
        int rate = ((Struct_Game391e9 *)g_game->net.get())->gravity;
        if (!rate)
            break;
        float x = (float)sqrt(size * 2.0 / rate) * 30.0f;
        int time = (int)(x * unit->motion->field_22) + 1;
        time += def->attackrunlength;
        Class_0044e2d0* obj;
        if (order->target)
            obj = (Class_0044e2d0*)new (kSim) Class_0044e190(order, order->target);
        else
            obj = new (kSim) Class_0044e2d0(order, order->pos);
        obj->SetApproachRadius(time);
        order->SetAttachedFx(obj);
        order->SetDeadlineTicks(1);
        order->flags |= 0x100e8;
        return 2;
    }
    case 5: {
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 0);
        g_seams.targeting->SetWeaponTargetPos(unit, &order->pos, 0);
        int angle = GetHeadingBetween(&unit->pos, &order->pos);
        Vec3 dest = AddVec3(unit->pos, Offset(angle, (unit->def->attackrunlength + 0x3c0) << 16));
        AttachMove(order, dest, 0x3c0);
        order->flags = 0xe2;
        return 1;
    }
    case 6: {
        g_seams.targeting->ClearWeaponTarget(unit, 0);
        Vec3 dest = AddVec3(unit->pos, Offset(unit->heading, 0x5a00000));
        AttachMove(order, dest, 0x80);
        order->flags = 0xe2;
        if (unit->health < unit->def->maxHealth / 4 * 3) {
            std::vector<Unit*> pads;
            g_seams.ai->GetFactoriesInRadius(((Struct_Unit96*)unit->player.get())->slotIndex, &unit->pos, 0xf00, &pads);
            if (!pads.empty()) {
                order->SetAttachedFx(0);
                Unit* pad = pads[RandomInt(pads.size())];
                AppendOrder(unit, new (kSim) Order("VTOL_LANDING", pad, 0, 0, 0, 0));
                order->flags = 0;
                return 0;
            }
            return 0;
        }
        order->state = 3;
        return 2;
    }
    }
    return 7;
}

static inline void AttachMove(Order* order, const Vec3& dest, short radius)
{
    Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, dest);
    obj->SetApproachRadius(radius);
    order->SetAttachedFx(obj);
}





struct Unit;


#pragma pack(push, 1)
struct Sound;
struct HapiBank;
struct TdfFile;
struct TdfRecord;
struct Mission;
struct Gadget;
struct Layer;
struct Feature;

#pragma pack(pop)



void __stdcall AddOrder(int kind, int remove, Unit* owner, void* id, Vec3* pos, int param_6, int param_7);

// FUNCTION: 0x412710
int __stdcall AirToGroundOrder(Unit* unit, Order* order, int flags)
{
    int speed = GetSpeed(unit);
    if (flags & 0x1000a) {
        if (*(int*)&order->next == 0 && (unit->flags & 0x300000))
            AppendOrderToTail(unit, new (kSim) Order("VTOL_SEEKATTACK", order->target, &order->pos, 0, 0, 0));
        return 5;
    }
    if (order->target == 0 && (order->flags_42 & 0x200)) {
        if (*(int*)&order->next == 0)
            AppendOrderToTail(unit, new (kSim) Order("VTOL_SEEKATTACK", 0, &unit->pos, 0, 0, 0));
        return 5;
    }
    if (order->target)
        order->pos = order->target->pos;
    if ((int)unit->spatialBucket.get() == (int)g_game->overflowBucket.get()) {
        order->SetDeadlineTicks(0x1e);
        order->state = 2;
    }
    if (order->range && (int)_hypot(unit->xWhole - order->x, unit->zWhole - order->z) >= order->range)
        return 5;
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0:
        if (unit->motion && (unit->def->flags1 & 0x800)) {
            order->AnnounceStatusIfFlagged("Attacking");
            PrepVtolClimb(unit, order, 0);
            return 1;
        }
        break;
    case 1: {
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        int dist = (int)_hypot(order->pos.x - unit->pos.x, order->pos.z - unit->pos.z);
        int angle = GetHeadingBetween(&unit->pos, &order->pos);
        Vec3 off = Offset(RandomInt(0x4000) + angle - 0x2000, dist / 2);
        Vec3 p = AddVec3(unit->pos, off);
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, p);
        ((Class_0044e730*)obj)->SetApproachRadius(0x80);
        order->SetAttachedFx(obj);
        order->flags = 0x100e8;
        return 1;
    }
    case 2: {
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 0);
        if (order->target)
            g_seams.targeting->SetWeaponTargetUnit(unit, order->target, 0);
        else
            g_seams.targeting->SetWeaponTargetPos(unit, &order->pos, 0);
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, order->pos);
        ((Class_0044e730*)obj)->SetApproachRadius(speed);
        order->SetAttachedFx(obj);
        order->flags = 0x100e8;
        return 1;
    }
    case 3: {
        short angle = AngleOfVector(unit->pos.x - order->pos.x, unit->pos.z - order->pos.z);
        Vec3 off = Offset(angle, speed * 0x30000);
        Vec3 p = AddVec3(order->pos, off);
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, p);
        ((Class_0044e730*)obj)->SetApproachRadius(RandomInt(0x80) + 0x80);
        order->SetAttachedFx(obj);
        order->flags = 0x100ea;
        return 1;
    }
    case 4: {
        if (IsDamaged(unit)) {
            LandingPadList v;
            g_seams.ai->GetFactoriesInRadius(unit->player->index, &unit->pos, 0xf00, &v);
            if (!v.empty()) {
                order->SetAttachedFx(0);
                Unit* target = v[RandomInt(v.size())];
                AppendOrder(unit, new (kSim) Order("VTOL_LANDING", target, 0, 0, 0, 0));
                order->flags = 0;
                return 0;
            }
        }
        short angle = RandomInt(2) ? unit->heading + 0x4000 : unit->heading - 0x4000;
        Vec3 off = Offset(angle, speed << 16);
        Vec3 sum;
        sum.x = unit->pos.x + off.x;
        sum.y = unit->pos.y + off.y;
        sum.z = unit->pos.z + off.z;
        Vec3 p = sum;
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, p);
        ((Class_0044e730*)obj)->SetApproachRadius(0x80);
        order->SetAttachedFx(obj);
        order->flags = 0x100ea;
        Dummy();
        Dummy();
        Dummy();
        Dummy();
        Dummy();
        Dummy();
        Dummy();
        Dummy();
        Dummy();
        Dummy();
        Dummy();
        return 1;
    }
    case 5:
        order->state = 2;
        return 2;
    }
    return 7;
}

struct Unit;


static inline UnitDef* Definitions() { return g_game->defs; }
void __stdcall EmitNanoParticles(Vec3*, Vec3*, int);
static inline short Angle(Vec3* a,Vec3* b) { return AngleOfVector(a->x-b->x,a->z-b->z); }
struct Feature;
// FUNCTION: 0x414380
int __stdcall VtolHelpBuildOrder(Unit* unit,Order* order,int flags)
{
    if (order->target && !(flags&8)) {
        if (flags&2) { MarkSelectionOrdersDirty(unit); return 5; }
        unsigned int state=0; state=order->state;
        switch(state) {
        case 0:
        if (unit->motion) {
            if (unit->def->flags1&0x800) {
                if (!unit->def->ids) return 7;
                order->AnnounceStatusIfFlagged("Building");
                g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
                if (unit->carrier) g_seams.units->AttachUnitToPiece(unit,0,-1,2);
                g_seams.units->SetStateBits(unit, 1,1);
                if ((unit->motion->flags&3)==1) {
                    unit->motion->SetFlightMode(unit,2);
                    Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,unit->pos);
                    ((Class_0044e6c0*)move)->SetAltitude(unit->def->altitude/2);
                    order->SetAttachedFx(move);
                    order->flags|=0xe0;
                }
                return 1;
            }
        }
        break;
    case 1: {
        order->retries=0;
        Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,order->target->pos);
        ((Class_0044e730*)move)->SetApproachRadius(unit->def->buildRange);
        order->SetAttachedFx(move);
        order->flags=0xe0;
        return 1;
    }
    case 2:
        if (flags&0x40) return 8;
        if (order->target->buildLeft==0.0f) return 5;
        StartBuildingScript(unit,order,(short)GetHeadingBetween(&unit->pos,&order->target->pos));
        MarkSelectionOrdersDirty(unit);
        return 1;
    case 3: {
        if ((unsigned int)g_game->ticks%150==0) {
            int angle=GetHeadingBetween(&unit->pos,&order->target->pos);
            int range=unit->def->buildRange<<16;
            angle+=0xdb6e;
            Vec3* base=&order->target->pos;
            Vec3 pos=PosMinus(*base,Offset(angle,range));
            Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,pos);
            ((Class_0044e720*)move)->SetHeading((unsigned short)angle);
            order->SetAttachedFx(move);
        }
        int rate=0; rate=unit->def->buildRate;
        int ok=g_seams.units->AddBuildProgress(unit,order->target,(float)(rate/30));
        if (ok) {
            Vec3 start;
            g_seams.units->GetNanoPiecePosition(unit,&start);
            Vec3 bounds[2];
            bounds[0]=AddVec3(order->target->pos,*(Vec3*)&order->target->def->modelMinX);
            bounds[1]=AddVec3(order->target->pos,*(Vec3*)&order->target->def->modelMaxX);
            EmitNanoParticles(&start,bounds,6);
        }
        if (order->target->buildLeft!=0.0f) {
            order->SetDeadlineTicks(1);
            order->flags|=0xa;
            return 2;
        }
        return 5;
    }
    }
        return 7;
    }
    QueueUnitSpeech(unit,7,"Construction terminated by hostile action");
    MarkSelectionOrdersDirty(unit);
    return 8;
}



struct Unit;





void __stdcall EmitReverseNanoParticles(Box* from, Vec3* to, int count);

// FUNCTION: 0x414770
int __stdcall VtolReclaimOrder(Unit* unit, Order* order, int flags)
{
    Point16 cell;
    Point16 size;
    unsigned short index = g_seams.weapons->FindFeatureAtPos(&order->pos, &cell, &size);
    if (index == 0xffff) {
        QueueUnitSpeech(unit, 7, "Reclamation failed");
        return 8;
    }
    Feature* f = &g_game->features[index];
    if (!(f->flags & 0x80))
        return 8;
    switch (order->state) {
    case 0:
        if (unit->motion && (unit->def->flags1 & 0x800) && (unit->def->flags2 & 0x400)) {
            order->AnnounceStatusIfFlagged("Reclaiming");
            PrepVtolClimb(unit, order, 0);
            return 1;
        }
        break;
    case 1: {
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, order->pos);
        order->SetAttachedFx(obj);
        order->flags = 0xe0;
        return 1;
    }
    case 2:
        if (flags & 0x40)
            return 8;
        order->time = (int)(30.0f - (f->metal + f->energy) * -0.5f);
        QueueUnitSpeech(unit, 11, 0);
        return 1;
    case 3:
        order->SetDeadlineTicks(2);
        order->time -= 2;
        if (order->time <= 0)
            return 1;
        unit->workTime = g_game->ticks + 300;
        if (order->time > 30) {
            Vec3 nano;
            g_seams.units->GetNanoPiecePosition(unit, &nano);
            Box box;
            box.lo.x = cell.x << 20;
            box.lo.z = cell.y << 20;
            box.lo.y = g_seams.units->GetGroundHeight(&box.lo) << 16;
            box.hi = box.lo;
            box.hi.x += f->footprintX << 20;
            box.hi.z += f->footprintZ << 20;
            box.hi.y += f->height << 16;
            EmitReverseNanoParticles(&box, &nano, 6);
            EmitReverseNanoParticles(&box, &nano, 6);
        }
        return 2;
    case 4:
        g_seams.features->ReclaimFeature(unit, &order->pos);
        return 5;
    }
    return 7;
}


static inline Vec3 Sub(const Vec3& a, const Vec3& b)
{
    Vec3 r; r.z = a.z - b.z; r.y = a.y - b.y; r.x = a.x - b.x; return r;
}

static inline int Square(const Vec3& v)
{
    __int64 a = v.x, b = v.z;
    return (int)((a*a) >> 32) + (int)((b*b) >> 32);
}

struct Unit;




void __stdcall EmitReverseNanoParticles(Vec3* box, Vec3* from, int count);

// FUNCTION: 0x414a80
int __stdcall VtolReclaimUnitOrder(Unit* unit, Order* order, unsigned int flags)
{
    Unit* target = order->target;
    if (!target || (flags & 0x10008))
        return 5;
    switch (order->state) {
    case 0:
        if (unit->motion && (unit->def->flags1 & 0x800)) {
            if (!(unit->def->flags2 & 0x400)) {
                QueueUnitSpeech(unit, 7, "Reclamation failed");
                return 7;
            }
            if (!g_seams.units->CanReclaim(unit, 0, target)) {
                QueueUnitSpeech(unit, 7, "That unit cannot be reclaimed");
                return 8;
            }
            order->AnnounceStatusIfFlagged("Reclaiming");
            PrepVtolClimb(unit, order, 0);
            return 1;
        }
        return 7;
    case 1: {
        order->elapsed = ComputeReclaimDamagePulse(unit, target, 15);
        order->duration = 0;
        Point16 origin = *(Point16*)&unit->footprint;
        Point16 cell = WorldToCell(order->pos, origin);
        CellToWorldPos(cell, &order->pos, origin);
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, order->target->pos);
        order->SetAttachedFx(obj);
        order->flags |= 0x100e8;
        QueueUnitSpeech(unit, 11, 0);
        return 1;
    }
    case 2: {
        if (flags & 0x40)
            return 9;
        order->flags |= 0x10008;
        Vec3 delta = Sub(unit->pos, order->target->pos);
        int range = 0;
        range = unit->def->buildRange;
        int square = Square(delta);
        if (square <= range * range && g_seams.units->CanReclaim(unit, 0, order->target)) {
            if (order->duration >= 15) {
                g_seams.weapons->DamageUnit(unit, order->target, order->elapsed, 5, 0);
                order->duration = 0;
            }
            Vec3 start;
            g_seams.units->GetNanoPiecePosition(order->source, &start);
            Vec3 bounds[2];
            bounds[1] = order->target->pos;
            bounds[0] = order->target->pos;
            bounds[0].x += order->target->def->modelMinX;
            bounds[0].z += order->target->def->modelMinZ;
            bounds[1].x += order->target->def->modelMaxX;
            bounds[1].z += order->target->def->modelMaxZ;
            bounds[1].y += order->target->def->modelMaxY;
            EmitReverseNanoParticles(bounds, &start, 6);
            order->SetDeadlineTicks(2);
            order->duration += 2;
            return 2;
        }
        order->SetDeadlineTicks(30);
        return 0;
    }
    }
    return 7;
}

// FUNCTION: 0x4103e0
int __stdcall VtolSeekAttackOrder(Unit* unit, Order* order, int flags)
{
    if (flags & 0x40) return 5;
    if ((int)unit->spatialBucket.get() == (int)g_game->overflowBucket.get()) {
        Vec3 center;
        center.x=(g_game->mapWidth/2)<<16;
        center.z=(g_game->mapHeight/2)<<16;
        short angle=GetHeadingBetween(&unit->pos,&center);
        Vec3 pos=AddVec3(unit->pos,Offset(angle,0x3200000));
        Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,pos);
        ((Class_0044e730*)move)->SetApproachRadius(128);
        order->flags|=0xe0;
        order->SetAttachedFx(move);
        return 2;
    }
    unsigned int state=0; state=order->state;
    switch(state) {
    case 0:
        if (unit->motion && (unit->def->flags1&0x800)) {
            if (order->target) {
                if (IssueAttackOrder(unit,order->target,0)) { order->flags=0; return 0; }
            } else {
                if (!order->pos.x && !order->pos.z && !order->pos.y) order->pos=unit->pos;
                order->angle=RandomInt(0x10000);
                order->parity=order->angle&1;
                g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
                if (unit->carrier) g_seams.units->AttachUnitToPiece(unit,0,-1,2);
                g_seams.units->SetStateBits(unit, 1,1);
                if ((unit->motion->flags&3)==1) {
                    unit->motion->SetFlightMode(unit,2);
                    Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,unit->pos);
                    ((Class_0044e6c0*)move)->SetAltitude(unit->def->altitude/2);
                    order->SetAttachedFx(move);
                    order->flags|=0xe0;
                }
            }
            return 1;
        }
        break;
    case 1: {
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        if ((unsigned int)unit->health < (unit->def->maxHealth>>2)*3) {
            LandingPadList pads;
            g_seams.ai->GetFactoriesInRadius(unit->player.get()->index,&unit->pos,0xf00,&pads);
            if (!pads.empty()) {
                order->SetAttachedFx(0);
                Unit* pad=pads[RandomInt(pads.size())];
                AppendOrder(unit,new (kSim) Order("VTOL_LANDING",pad,0,0,0,0));
                order->flags=0;
                return 0;
            }
        }
        Unit* target=g_seams.targeting->FindBestTargetIfFireAtWill(unit);
        if (target && IssueAttackOrder(unit,target,0)) return 5;
        if (flags&0xe0) order->angle+=-RandomInt(0x2000)-0x5555;
        Vec3 pos=AddVec3(order->pos,Offset((short)order->angle,(((WeaponDef*)unit->weapons[0].weapon)->range+160)<<16));
        Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,pos);
        ((Class_0044e730*)move)->SetApproachRadius(128);
        order->SetAttachedFx(move);
        order->SetDeadlineTicks(RandomInt(30)+30);
        order->flags|=0xe0;
        return 2;
    }
    }
    return 7;
}

// FUNCTION: 0x410850
int __stdcall VtolSeekGuardOrder(Unit* unit, Order* order, int flags)
{
    if (flags&0x40) return 5;
    if ((int)unit->spatialBucket.get() == (int)g_game->overflowBucket.get()) {
        Vec3 center;
        center.x=(g_game->mapWidth/2)<<16;
        center.z=(g_game->mapHeight/2)<<16;
        short angle=GetHeadingBetween(&unit->pos,&center);
        Vec3 pos=AddVec3(unit->pos,Direction(angle,0x3200000));
        Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,pos);
        move->SetApproachRadius(128);
        order->flags|=0xe0;
        order->SetAttachedFx(move);
        return 2;
    }
    switch(order->state) {
    case 0:
        if (unit->motion && (unit->def->flags1&0x800)) {
            Vec3* pos=&order->pos;
            if (!pos->x && !pos->z && !pos->y) *pos=unit->pos;
            order->angle=RandomInt(0x10000);
            order->parity=order->angle&1;
            return 1;
        }
        break;
    case 1:
        int result=Patrol(unit,order,flags);
        return result;
    }
    return 7;
}

// FUNCTION: 0x412d40
int __stdcall AirToAirOrder(Unit* unit, Order* order, int flags)
{
    if (flags & 0x10008) {
        if (*(int*)&order->next == 0 && (unit->flags & 0x300000))
            AppendOrderToTail(unit, new (kSim) Order("VTOL_SEEKATTACK", order->target, &order->pos, 0, 0, 0));
        return 5;
    }
    if (order->target == 0 && (order->flags_42 & 0x200)) {
        if (*(int*)&order->next == 0)
            AppendOrderToTail(unit, new (kSim) Order("VTOL_SEEKATTACK", 0, &unit->pos, 0, 0, 0));
        return 5;
    }
    if ((int)unit->spatialBucket.get() == (int)g_game->overflowBucket.get()) {
        Vec3 centre;
        centre.x = g_game->mapWidth / 2 << 16;
        centre.z = g_game->mapHeight / 2 << 16;
        short angle = GetHeadingBetween(&unit->pos, &centre);
        Vec3 dest = AddVec3(unit->pos, Offset(angle, 0x3200000));
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, dest);
        obj->SetApproachRadius(0x80);
        order->flags |= 0xe0;
        order->SetAttachedFx(obj);
        return 2;
    }
    if (order->range && (int)_hypot(unit->xWhole - order->x, unit->zWhole - order->z) >= order->range)
        return 5;
    switch (order->state) {
    case 0:
        if (unit->motion && (unit->def->flags1 & 0x800)) {
            order->AnnounceStatusIfFlagged("Attacking");
            PrepVtolClimb(unit, order, 0);
            order->SetDeadlineTicks(1);
            order->ticks = 0;
            return 1;
        }
        break;
    case 1:
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 0);
        g_seams.targeting->SetWeaponTargetUnit(unit, order->target, 0);
        if (flags & 0xe0) {
            if (IsAhead(unit, order)) {
                Vec3 from = AddVec3(unit->pos, Offset(unit->heading, unit->def->maxvelocity * 30));
                Vec3 to = Offset(unit->heading, unit->def->maxvelocity);
                AirManeuverOrder* obj = new (kSim) AirManeuverOrder(order, from, to);
                obj->SetAltitude(unit->def->altitude);
                order->SetAttachedFx(obj);
                order->SetDeadlineTicks(RandomInt(0x1e) + 0x3c);
                order->ticks = 0;
                return 2;
            }
        }
        if (!(flags & 0xe0) && order->ticks < 0x5a) {
            if (IsAhead(unit, order))
                order->ticks = 0;
            else
                order->ticks += 0x2d;
            Fixed d;
            d.value = (int)_hypot(unit->pos.x - order->target->pos.x, unit->pos.z - order->target->pos.z);
            if (d.parts.whole > 0xa0) {
                Vec3 p = order->target->pos;
                p.x += ((UnitMotion*)order->target->motion)->velocity.x * 45;
                p.z += ((UnitMotion*)order->target->motion)->velocity.z * 45;
                order->SetAttachedFx(new (kSim) AirManeuverOrder(order, p,
                    Sum(((UnitMotion*)order->target->motion)->velocity, Offset(order->target->heading, order->target->def->maxvelocity / 2))));
            }
            order->SetDeadlineTicks(0x2d);
            order->flags |= 0x100e8;
            return 2;
        }
        order->SetAttachedFx(0);
        AppendOrder(unit, new (kSim) Order("VTOL_EVADE", order->target, 0, 0, 0, 0));
        order->ticks = 0;
        order->flags = 0;
        return 0;
    }
    return 7;
}

// FUNCTION: 0x4152f0
int __stdcall VtolRepairPatrolOrder(Unit* unit, Order* order, unsigned int flags)
{
    if ((flags & 0x48) != 0) {
        order->SetDeadlineTicks(0x1e);
        return 0;
    }
    for (;;) {
        switch (order->state) {
        case 0:
            if (unit->motion && (unit->def->flags1 & 0x800) && (unit->def->flags2 & 0x200)) {
                if (order->target != 0)
                    order->pos = order->target->pos;
                EnsurePatrolReturnOrder(unit, order);
                order->AnnounceStatusIfFlagged("Patrolling");
                PrepVtolClimb(unit, order, 0);
                return 1;
            }
            break;
        case 1: {
            if (flags & 0xe0)
                return 6;
            Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, order->pos);
            obj->SetAltitude(unit->def->altitude);
            order->SetAttachedFx(obj);
            order->SetDeadlineTicks(0x2d);
            order->flags |= 0xe0;
            if (Land(unit, order))
                return 0;
            if (((Player*)unit->player)->energy >= ((Player*)unit->player)->energyCapacity * 0.2) {
                std::vector<Unit*> units;
                int range = unit->def->range << 16;
                RepairableUnitVisitor collector((Player*)unit->player, &units, unit);
                g_seams.map->VisitObjectsInRange(&unit->pos, range, (UnitVisitor*)&collector);
                if (!units.empty()) {
                    Unit* target = units[RandomInt(units.size())];
                    if (g_seams.units->CanRepair(unit, 0, target) && target->buildLeft == 0.0f) {
                        if (IssueRepairOrder(unit, target, 0))
                            return 6;
                        return 3;
                    }
                    if (g_seams.units->CanRepair(unit, 0, target) && target->buildLeft != 0.0f) {
                        order->SetAttachedFx(0);
                        AppendOrder(unit, new (kSim) Order("VTOL_HELPBUILD", target, 0, 0, 0, 0));
                        order->flags = 0;
                        return 3;
                    }
                }
            }
            Vec3 energyPos, metalPos;
            Vec3* energy = &energyPos;
            Vec3* metal = &metalPos;
            float energyAmount, metalAmount;
            Fixed range;
            range.value = 0xf00000;
            if (g_seams.map->PickRandomReclaimableResourcesInRadius(&unit->pos, range.value, &energy, &energyAmount, &metal, &metalAmount)) {
                if (((Player*)unit->player)->GetMetal() < ((Player*)unit->player)->metalCapacity * 0.2 && metal) {
                    order->SetAttachedFx(0);
                    AppendOrder(unit, new (kSim) Order("VTOL_RECLAIM", 0, metal, 0, 0, 0));
                    order->flags = 0;
                    return 3;
                }
                if (((Player*)unit->player)->GetEnergy() < ((Player*)unit->player)->energyCapacity * 0.2 && energy) {
                    order->SetAttachedFx(0);
                    AppendOrder(unit, new (kSim) Order("VTOL_RECLAIM", 0, energy, 0, 0, 0));
                    order->flags = 0;
                    return 3;
                }
                if (metal && Total(((Player*)unit->player)->GetMetal(), metalAmount) <= ((Player*)unit->player)->metalCapacity) {
                    order->SetAttachedFx(0);
                    AppendOrder(unit, new (kSim) Order("VTOL_RECLAIM", 0, metal, 0, 0, 0));
                    order->flags = 0;
                    return 3;
                }
                if (energy && Total(((Player*)unit->player)->GetEnergy(), energyAmount) <= ((Player*)unit->player)->energyCapacity) {
                    order->SetAttachedFx(0);
                    AppendOrder(unit, new (kSim) Order("VTOL_RECLAIM", 0, energy, 0, 0, 0));
                    order->flags = 0;
                    return 3;
                }
            }
            return 2;
        }
        }
        return 7;
    }
}
