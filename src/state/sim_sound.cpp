#include "sim_effects.h"
#include "../game/game.h"
#include "../network/player.h"
#include "../util/vec3.h"
#include "../../interfaces/seams.h"
#include "../../link/globals.h"

unsigned int SoundHeardMask(const Vec3& at)
{
    if (g_seams.los_radar->GetMapCell(at.x / (1 << 20), at.z / (1 << 20)) == nullptr)
        return 0;
    int tx = (short)(at.x >> 16) >> 5;
    int ty = ((short)(at.z >> 16) - ((short)(at.y >> 16) >> 1)) >> 5;
    unsigned int mask = 0;
    for (int p = 0; p < 10; p++) {
        Player* player = &g_game->players[p];
        if (tx >= (unsigned int)player->exploredWidth || ty >= (unsigned int)player->exploredHeight)
            continue;
        int cell = player->exploredWidth * ty + tx;
        bool heard = (g_game->mapFlags & 2) == 2 ? player->explored[cell] != 0 : (g_game->visibilityMask[cell] & (1 << p)) != 0;
        if (heard)
            mask |= 1u << p;
    }
    return mask;
}
