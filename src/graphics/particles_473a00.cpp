#include "../util/vec3.h"

struct Pos_004739b0 {
    char unknown_0[0x2];
    short x;
    char unknown_4[0x2];
    short height;
    char unknown_8[0x2];
    short y;
};

#pragma pack(push, 1)

#include "rect.h"

#include "../network/player.h"

#include "../game/game.h"

extern Game* g_game;

static inline int Identity_00473a00(int v) { return v; }

static inline int IsSeen_00473a00(Player* p, Player* q, int col, int row)
{
    if (!((unsigned int)col < (unsigned int)p->exploredWidth && (unsigned int)row < (unsigned int)p->exploredHeight))
        return 0;
    return (g_game->visibilityMask[(unsigned int)q->exploredWidth * row + col] &
            (1 << g_game->playerIndex)) != 0;
}

class NanoParticle {
public:
    union {
        Vec3 pos;
        Pos_004739b0 posw;
    };
    char unknown_c[0xc];
    Vec3 vel;
    char unknown_24[4];
    int flags;
    int endTime;

    void Step();
    void DrawParticle(int dest, short x, short y);
    int IsExpired(int value);
};
#pragma pack(pop)

void __stdcall FillRectangle(void* surface, Rect* rect, int color);

// FUNCTION: 0x473a00
void NanoParticle::DrawParticle(int dest, short x, short y)
{
    Rect r;
    r.left = (short)(posw.x - x) + 0x80;
    r.top = (short)(posw.y - y) - (posw.height >> 1) + 0x20;
    r.right = r.left + 1;
    r.bottom = r.top + 1;

    Player* p = &g_game->players_1b63[g_game->playerIndex];
    Player* q = &g_game->players_1b63[g_game->playerIndex];
    int visible;
    if ((g_game->mapFlags & 2) == 2) {
        int col = posw.x >> 5;
        int row = (posw.y - (posw.height >> 1)) >> 5;
        visible = (unsigned int)col < (unsigned int)p->exploredWidth && (unsigned int)row < (unsigned int)p->exploredHeight &&
                  p->explored[(unsigned int)p->exploredWidth * row + col] != 0;
    } else {
        int col = posw.x >> 5;
        int row = (posw.y - (posw.height >> 1)) >> 5;
        visible = Identity_00473a00(IsSeen_00473a00(p, q, col, row));
    }

    if (visible)
        FillRectangle((void*)dest, &r, flags);
}

