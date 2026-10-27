#ifndef LOS_HELPERS_H
#define LOS_HELPERS_H

#include "../util/vec3.h"
#include "../network/player.h"
#include "game.h"

extern Game* g_game;

static inline int IsExplored(Player* player, Vec3Words* pos)
{
    int tx = pos->xWhole >> 5;
    int ty = (pos->zWhole - (pos->yWhole >> 1)) >> 5;
    if ((unsigned int)tx < (unsigned int)player->exploredWidth && (unsigned int)ty < (unsigned int)player->exploredHeight &&
        player->explored[(unsigned int)player->exploredWidth * ty + tx] != 0)
        return 1;
    return 0;
}

static inline int IsSeen(Player* player, Vec3Words* pos)
{
    int tx = pos->xWhole >> 5;
    int ty = (pos->zWhole - (pos->yWhole >> 1)) >> 5;
    if (!((unsigned int)tx < (unsigned int)player->exploredWidth && (unsigned int)ty < (unsigned int)player->exploredHeight))
        return 0;
    return (g_game->visibilityMask[(unsigned int)player->exploredWidth * ty + tx] &
            (1 << g_game->playerIndex)) != 0;
}

#endif
