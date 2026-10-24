#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

void* __stdcall GetGafFrame(void* a, int b);
void __stdcall DrawFrameBlended(void* dest, void* src, int x, int y);

#pragma pack(push, 1)
struct Pos_00474b80 {
    short x;
    char unknown_2[0x4 - 0x2];
    short h;
    char unknown_6[0x8 - 0x6];
    short y;
};

#include "../network/player.h"

#include "../game/game.h"

struct SmokeParticle {
    void* data;
    union {
        struct {
            int x;
            int y;
            int z;
        };
        struct {
            char unknown_4[0x6 - 0x4];
            Pos_00474b80 posw;
        };
    };
    int limit;
    int rounds;
    int period;
    int countdown;

    void Step();
    void DrawParticle(void* dest, short px, short py);
    int IsExpired(int unused);
};
#pragma pack(pop)

#include "../../link/globals.h"

static inline int Identity_00474b80(int v) { return v; }

static inline int IsSeen_00474b80(Player* p, Player* q, int col, int row)
{
    if (!((unsigned int)col < (unsigned int)p->exploredWidth && (unsigned int)row < (unsigned int)p->exploredHeight))
        return 0;
    return (g_game->visibilityMask[(unsigned int)q->exploredWidth * row + col] &
            (1 << g_game->playerIndex)) != 0;
}

// FUNCTION: 0x474b80
void SmokeParticle::DrawParticle(void* dest, short px, short py)
{
    Pos_00474b80* q = &posw;
    short sx = q->x - px + 0x80;
    short sy = q->y - (q->h >> 1) - py + 0x20;
    Player* p = &g_game->players[g_game->playerIndex];
    Player* p2 = &g_game->players[g_game->playerIndex];
    int visible;
    if ((g_game->mapFlags & 2) == 2) {
        int col = posw.x >> 5;
        int row = (posw.y - (posw.h >> 1)) >> 5;
        if ((unsigned int)col < (unsigned int)p->exploredWidth && (unsigned int)row < (unsigned int)p->exploredHeight &&
            p->explored[(unsigned int)p->exploredWidth * row + col] != 0)
            visible = 1;
        else
            visible = 0;
    } else {
        int col = posw.x >> 5;
        int row = (posw.y - (posw.h >> 1)) >> 5;
        visible = Identity_00474b80(IsSeen_00474b80(p, p2, col, row));
    }
    if (visible)
        DrawFrameBlended(dest, GetGafFrame(data, rounds), sx, sy);
}

