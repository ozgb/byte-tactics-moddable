#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

extern Game* g_game;

struct SquadManager;
#include "squad_manager.h"

#include "squad_timer.h"

static inline Vec3 Vec3At(int ax, int ay, int az)
{
    Vec3 v;
    v.x = ax;
    v.y = ay;
    v.z = az;
    return v;
}

// FUNCTION: 0x407d40
// FUNCTION: 0x407e70 ??_GSpatialTimer@@UAEPAXI@Z
SpatialTimer::SpatialTimer(SquadManager* p, void* q)
    : SquadTimer(p, (Group*)q)
{
    int x = (int)(g_game->baseX / 2 * 65536.0);
    best = Vec3At(x, 0, (int)(g_game->baseY / 2 * 65536.0));
    x = (int)(g_game->baseX / 2 * 65536.0);
    probe = Vec3At(x, 0, (int)(g_game->baseY / 2 * 65536.0));
    x = (int)(g_game->baseX / 2 * 65536.0);
    step = Vec3At(x, 0, (int)(g_game->baseY / 2 * 65536.0));
    bestRating = 0;
}
