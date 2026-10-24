#pragma pack(push, 1)
#include "../map/cell.h"

#include "../game/game.h"
#pragma pack(pop)

struct Fixed_00485070 {
    unsigned short frac;
    short whole;
};

struct Pos_00485070 {
    Fixed_00485070 x;
    Fixed_00485070 y;
    Fixed_00485070 z;
};

extern Game* g_game;

// FUNCTION: 0x485070
int __stdcall GetGroundHeight(Pos_00485070* p)
{
    int x = p->x.whole;
    int z = p->z.whole;
    int xc = x >> 4;
    int zc = z >> 4;
    int xf = x & 15;
    int zf = z & 15;
    if (xc < 0 || xc + 1 >= g_game->width || zc < 0 || zc + 1 >= g_game->height)
        return -1;
    Cell* c = &g_game->cells[zc * g_game->width + xc];
    int a = c->height;
    int b = c[1].height;
    c += g_game->width;
    int e = c->height;
    int f = c[1].height;
    int r1 = a + (b - a) * xf / 16;
    return r1 + ((e + (f - e) * xf / 16) - r1) * zf / 16;
}
