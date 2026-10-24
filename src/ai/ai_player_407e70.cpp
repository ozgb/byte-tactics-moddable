#include <vector>
#include "../state/sim_allocator.h"

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

#include "../../link/globals.h"

struct Position_00408090 {
    short xFrac;
    short x;
    short yFrac;
    short y;
    short zFrac;
    short z;
};

#include "../network/player.h"

int __stdcall IsPointVisible(Player* player, Position_00408090* pos);

static inline int IsExplored(Player* player, Position_00408090* pos)
{
    int tx = pos->x >> 5;
    int ty = (pos->z - (pos->y >> 1)) >> 5;
    if ((unsigned int)tx < (unsigned int)player->exploredWidth && (unsigned int)ty < (unsigned int)player->exploredHeight &&
        player->explored[(unsigned int)player->exploredWidth * ty + tx])
        return 1;
    return 0;
}

static inline int IsVisible(Player* player, Position_00408090* pos)
{
    if ((g_game->mapFlags & 2) == 2)
        return IsExplored(player, pos);
    return IsPointVisible(player, pos);
}

#include "../units/unit_def.h"

#pragma pack(push, 1)
struct UnitDefFlags_00407e90 {
    char unknown_0[0x245];
    unsigned int unknown_bits : 4;
    unsigned int flag4 : 1;
};

#pragma pack(pop)

#include "../units/unit.h"

#include "../orders/mission_type.h"

#include "squad_manager.h"

#include "group.h"
#include "squad_timer.h"

static inline Vec3 MakeVec3(int ax, int ay, int az)
{
    Vec3 v;
    v.x = ax;
    v.y = ay;
    v.z = az;
    return v;
}

#include "../util/math_4b6c30.h"
int __cdecl ScaledSin(int angle, int distance);
int __cdecl ScaledCos(int angle, int distance);

static inline Vec3 Offset(int angle, int distance)
{
    Vec3 v;
    v.x = -ScaledSin(angle, distance);
    v.y = 0;
    v.z = -ScaledCos(angle, distance);
    return v;
}

int __stdcall SumUnitRatingsInRange(int index, Vec3* pos, int range);
#include "../../interfaces/seams.h"

// FUNCTION: 0x407e90
void SpatialTimer::OnTimer()
{
    int delay = RandomInt(150) + 30;
    next = g_game->ticks + delay;
    if (group->units.empty())
        return;
    std::vector<Unit*> unused;
    if (RandomInt(10) == 0) {
        probe = best;
        int angle = RandomInt(0x10000);
        step = Offset(angle, 0x1400000);
    }
    probe += step;
    if (IsVisible((Player*)group->player, (Position_00408090*)&probe)) {
        int r = SumUnitRatingsInRange(player, &probe, 0xa0);
        if (RandomInt(r) > RandomInt(bestRating)) {
            bestRating = r;
            best = probe;
        }
    }
    for (SimVector<Unit*>::iterator it = group->units.begin();
         it != group->units.end(); ++it) {
        Unit* u = *it;
        if (((UnitDefFlags_00407e90*)u->def.get())->flag4) {
            if (u->motion || g_seams.weapons->WeaponCanReachPos(u, (Vec3*)&u->pos, (Vec3*)&best, 0)) {
                MissionType kind = g_seams.orders->GetOrderType(3, u, 0, (Vec3*)&best);
                if (kind.index)
                    g_seams.orders->AddOrder(kind, 0, u, 0, (Vec3*)&best, 0, 0);
            }
        }
    }
}
