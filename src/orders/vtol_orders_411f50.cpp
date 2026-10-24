#include "../../interfaces/seams.h"
#include <list>
#include "win_types.h"
#include <math.h>
#include <vector>

#include "../util/vec3.h"
static inline Vec3 PosPlus(const Vec3& a, const Vec3& other)
{
    Vec3 r;
    r.x = a.x + other.x;
    r.y = a.y + other.y;
    r.z = a.z + other.z;
    return r;
}

struct Unit;
#include "unit_motion.h"
#include "../state/block.h"
#include "path_order_attach.h"
#include "mission_type.h"

void UpdateMouseScroll();
void UpdateEdgeScroll();
void CenterCameraOnRadarClick();
void CenterCameraOnStartPosition();
#pragma pack(push, 1)
#include "../units/unit_def.h"
struct Struct_Unit96 {
    char pad0[0x146]; unsigned char slotIndex;
};
#include "../units/unit.h"
#include "order.h"
struct Struct_Game391e9 {
    char pad0[0xd3c]; int gravity;
};
#include "../game/game.h"
#include "path_fx.h"
#pragma pack(pop)

extern Game* g_game;

int __stdcall RandomInt(int);
int __cdecl ScaledSin(short, int);
int __cdecl ScaledCos(short, int);
int __stdcall GetHeadingBetween(Vec3*, Vec3*);
void __stdcall AppendOrder(Unit*, Order*);
void __stdcall AppendOrderToTail(Unit*, Order*);

static inline Vec3 Offset(short angle, int distance)
{
    Vec3 v;
    v.x = -ScaledSin(angle, distance);
    v.y = 0;
    v.z = -ScaledCos(angle, distance);
    return v;
}

static inline void AttachMove(Order* order, const Vec3& dest, short radius);

void __stdcall PrepVtolClimb(Unit* unit, Order* order, unsigned int flags)
{
    g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
    if (unit->carrier)
        g_seams.units->AttachUnitToPiece(unit, 0, -1, 2);
    g_seams.units->SetStateBits(unit, 1, 1);
    if ((unit->motion->flags & 3) == 1) {
        unit->motion->SetFlightMode(unit, 2);
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, unit->pos);
        obj->SetAltitude(unit->def->altitude / 2);
        order->SetAttachedFx(obj);
        order->flags |= flags | 0xe0;
    }
}

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
            Vec3 dest = PosPlus(unit->pos, Offset(angle, 0x8c00000));
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
        Vec3 dest = PosPlus(unit->pos, Offset(RandomInt(0x4000) + angle - 0x2000, radius));
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
        Vec3 dest = PosPlus(unit->pos, Offset(angle, (unit->def->attackrunlength + 0x3c0) << 16));
        AttachMove(order, dest, 0x3c0);
        order->flags = 0xe2;
        return 1;
    }
    case 6: {
        g_seams.targeting->ClearWeaponTarget(unit, 0);
        Vec3 dest = PosPlus(unit->pos, Offset(unit->heading, 0x5a00000));
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
