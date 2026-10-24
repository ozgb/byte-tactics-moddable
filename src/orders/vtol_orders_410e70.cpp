#include "../../interfaces/seams.h"
#include "win_types.h"
#include <vector>

#include "../util/vec3.h"

#include "../state/block.h"
#include "mission_type.h"

struct Unit;
#include "unit_motion.h"

class Class_0044e6c0 { public: void SetAltitude(int); };
class Class_0044e730 { public: void SetApproachRadius(short); };

#pragma pack(push, 1)
#include "../units/unit_def.h"
#include "../network/player.h"
#include "../units/unit.h"
#include "order.h"
#include "path_fx.h"
#pragma pack(pop)

#include "../util/math_4b6c30.h"
int __cdecl ScaledSin(short, int);
int __cdecl ScaledCos(short, int);
int __stdcall GetHeadingBetween(Vec3*, Vec3*);
#include "order_list.h"
int __stdcall IssueAttackOrder(Unit*, Unit*, int);

static inline Vec3 Offset(short angle, int distance)
{
    Vec3 v;
    v.x = -ScaledSin(angle, distance);
    v.y = 0;
    v.z = -ScaledCos(angle, distance);
    return v;
}

Vec3 __stdcall AddVec3(const Vec3& a, const Vec3& b)
{
    Vec3 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
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


static inline void Dummy(void) {}
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
