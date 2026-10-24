#include "win_types.h"
#include <stdio.h>
#include <vector>
#include <list>
#include <map>
#include <math.h>
#include <tchar.h>
#include <time.h>
#include <float.h>
#include "../state/block.h"
#include "../util/vec3.h"
#include "vtol_orders_410850.h"
struct Unit;
struct Order;
#include "mission_type.h"

#pragma pack(push, 1)
#include "../weapons/weapon_def.h"
#include "../units/unit_def.h"
struct Owner { char pad0[0x108]; unsigned char allied[0x3e]; unsigned char index; };
#include "unit_motion.h"
#include "../units/unit.h"
#include "order.h"

#include "path_fx.h"
#pragma pack(pop)
#include "../game/game.h"
#include "../../link/globals.h"
short __stdcall GetHeadingBetween(Vec3*, Vec3*);
MissionType __stdcall GetOrderType(unsigned char, Unit*, Unit*, int);
#include "order_list.h"
#include "../util/math_4b6c30.h"
int __cdecl ScaledSin(short, int);
int __cdecl ScaledCos(short, int);
Vec3 __stdcall AddVec3(const Vec3& a, const Vec3& b);
union Fixed { int value; struct { unsigned short frac; short whole; } parts; };
#include "vtol_orders.h"
static inline Vec3 Direction(short angle, int range) { Fixed distance; distance.value=range; return DirectionFromAngle(angle,distance); }
static inline Vec3 Offset(short angle, int distance)
{
    Vec3 v;
    v.x=-ScaledSin(angle,distance);
    v.y=0;
    int z=ScaledCos(angle,distance);
    v.z=-z;
    return v;
}

class LandingPadList : public std::vector<Unit*> { public: LandingPadList(); };
#include "../../interfaces/seams.h"
class UnitList : public std::vector<Unit*> {};

class GroundAllyVisitor {
public:
    virtual void CollectGroundAlly(Unit*);
    GroundAllyVisitor(Owner* o, std::vector<Unit*>* u, Unit* s) : owner(o), units(u), self(s) {}
    Owner* owner; std::vector<Unit*>* units; Unit* self;
};
static inline int IsDamaged(Unit* u) { return (unsigned int)u->health < (u->def->maxHealth>>2)*3; }
static inline void FindPads(Unit* u, std::vector<Unit*>* pads) { g_seams.ai->GetFactoriesInRadius(((Owner*)u->player.get())->index,&u->pos,0xf00,pads); }
static inline int SearchRange(Unit* u) { return u->def->range<<16; }
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
    GroundAllyVisitor visitor((Owner*)unit->player.get(),&units,unit);
    g_seams.map->VisitObjectsInRange(&unit->pos,range,(UnitVisitor*)&visitor);
    if (!units.empty()) {
        order->SetAttachedFx(0);
        MissionType kind=GetOrderType(7,unit,units[0],0);
        AppendOrder(unit,new (kSim) Order(kind,units[0],0,0,0,0));
        order->flags=0;
        return 3;
    }
    if (flags&0xe0) order->angle+=-RandomInt(0x2000)-0x4000;
    Vec3 pos=order->pos+Offset((short)order->angle,(((WeaponDef*)unit->weapons[0].weapon)->range+160)<<16);
    Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,pos);
    move->SetApproachRadius(128);
    order->SetAttachedFx(move);
    order->SetDeadlineTicks(30);
    order->flags|=0xf8;
    return 2;
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
