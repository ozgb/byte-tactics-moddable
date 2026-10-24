#include "../../interfaces/seams.h"

#include <math.h>
#include <vector>

#include "../util/vec3.h"
static inline Vec3 PosPlus(const Vec3& a, const Vec3& v) { Vec3 r = a; r.x += v.x; r.y += v.y; r.z += v.z; return r; }

#include "../state/block.h"
#include "mission_type.h"

struct Unit;
#include "unit_motion.h"

class Class_0044e6c0 { public: void SetAltitude(int); };
class Class_0044e730 { public: void SetApproachRadius(short); };

#pragma pack(push, 1)
#include "../units/unit_def.h"
#include "../weapons/weapon_def.h"
struct Sound;
struct HapiBank;
struct TdfFile;
struct TdfRecord;
struct Mission;
struct Gadget;
struct Layer;
struct Weapon;
struct Feature;
#include "../network/player.h"
#include "../units/unit.h"
int __stdcall IsUnitCommander(Unit*);
void __stdcall AlignUnitToGround(Unit*);

#include "order.h"
int __stdcall DirectionFromStep(int, int);
int __stdcall FindWeaponTarget(Unit*, unsigned char, int);
void __stdcall ReactToAttack(Unit*, Unit*, int);
int __stdcall GetBuilderCount(int);
int __stdcall GetUnitCount(int, unsigned int);
#include "../game/game.h"
#include "path_fx.h"
#pragma pack(pop)

extern Game* g_game;

int __stdcall RandomInt(int);
int __cdecl ScaledSin(short, int);
int __cdecl ScaledCos(short, int);
int __cdecl AngleOfVector(int x, int z);
int __stdcall GetHeadingBetween(Vec3*, Vec3*);
void __stdcall AppendOrderToTail(Unit*, Order*);
void __stdcall AppendOrder(Unit*, Order*);

class LandingPadList : public std::vector<Unit*> {};

static inline Vec3 Offset(short angle, int distance)
{
    Vec3 v;
    v.x = -ScaledSin(angle, distance);
    v.y = 0;
    v.z = -ScaledCos(angle, distance);
    return v;
}

static inline void Dummy(void) {}
static inline int IsDamaged(Unit* u) { return (unsigned int)u->health < (u->def->maxHealth >> 2) * 3; }
static inline int GetSpeed(Unit* unit)
{
    return ((WeaponDef*)unit->weapon)->range;
}

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

void __stdcall AddOrder(int kind, int remove, Unit* owner, void* id, Vec3* pos, int param_6, int param_7);
void __stdcall AdjustBuildCount(int kind, Unit* owner, int id, int amount);
int __stdcall GetOrderName(Unit* obj);

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
        Vec3 p = PosPlus(unit->pos, off);
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
        Vec3 p = PosPlus(order->pos, off);
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
