#include "win_types.h"
#include <string.h>

#include "../util/vec3.h"
#include "../units/unit_def.h"

#pragma pack(push, 1)
#include "../units/unit.h"

#include "../network/player.h"

#include "../game/game.h"
#include "../state/sim_effects.h"

struct ExplodedPieceInit {
    Ref<Unit> obj;
    int index;
    int r1;
    int r2;
    int r3;
    int x;
    int y;
    int z;
    int f20;
    int f24;
    unsigned int bits;
    Ref<char> rec;
};
#pragma pack(pop)

#include "../../link/globals.h"
#include "../../interfaces/seams.h"
#include "../util/math.h"
#include "explosions.h"
#include "../scripts/unit_script.h"
void __stdcall AddExplosionEffect(void* pos, void* src, int index, int flag);

// FUNCTION: 0x481140
void UnitScript::ExplodePiece(int a, unsigned int b)
{
    if (!(b & 0x20)) {
        ExplodedPieceInit h;
        h.obj = state->unit;
        h.index = a;
        h.r1 = RandomInt(3000);
        h.r2 = RandomInt(3000);
        h.r3 = RandomInt(3000);
        h.x = (0x14 - RandomInt(0x28)) << 14;
        h.y = RandomInt(10) << 16;
        h.z = (0x14 - RandomInt(0x28)) << 14;
        h.f20 = 900;
        if (b & 1) {
            h.bits = (h.bits & ~0x30) | ((b & 2) << 4);
            h.f24 = 1;
            h.bits |= 4;
        } else {
            h.f24 = 0;
            h.bits = (h.bits & ~0x34) | ((b & 2) << 3);
        }
        h.bits = (h.bits & ~2) | ((b >> 2) & 2);
        h.bits = (h.bits & ~1) | ((b >> 4) & 1);
        h.bits = (h.bits & ~8) | ((b & 4) << 1);
        StartExplodePiece((ExplodedPiece *)&h);
    }
    if (b & 0x3f00) {
        Vec3 v;
        v = g_seams.scripts->GetPiecePosition(state->unit, a);
        if (b & 0x100)
            AddExplosionEffect(&v, g_game->sources[0], 2, 0);
        if (b & 0x200)
            AddExplosionEffect(&v, g_game->sources[1], 2, 0);
        if (b & 0x400)
            AddExplosionEffect(&v, g_game->sources[2], 2, 0);
        if (b & 0x800)
            AddExplosionEffect(&v, g_game->sources[3], 2, 0);
        if (b & 0x1000)
            AddExplosionEffect(&v, g_game->sources[4], 2, 0);
        if (b & 0x2000)
            AddExplosionEffect(&v, g_game->sources[5], 2, 0);
    }
}

// FUNCTION: 0x4b1eb0
void CobScript::ExplodePiece(int, unsigned int)
{
}

