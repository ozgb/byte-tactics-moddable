#include "win_types.h"
#include <string>
#include <math.h>
#include <string.h>

#include "../util/vec3.h"
#include "unit_def.h"

#pragma pack(push, 1)
#include "unit.h"

#include "../network/player.h"

#include "../game/game.h"

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

#include "unit_scripts.h"
int __stdcall GetGroundHeight(Vec3* pos);
int __cdecl AngleOfVector(int a, int b);
#include "../../interfaces/seams.h"
#include "../graphics/particles.h"
void __stdcall EmitWhiteSmoke(int, short);
void __stdcall EmitBlackSmoke(int, short);
#include "../util/math.h"
#include "../weapons/explosions.h"
void __stdcall AddExplosionEffect(void* pos, void* src, int index, int flag);
void __stdcall AttachUnitToPiece(Unit* unit, Unit* transporter, int a, int b);

static inline Unit* GetUnit(unsigned short id)
{
    if (id == 0)
        return 0;
    return &g_game->units[id];
}

#include "unit_script.h"

// FUNCTION: 0x480770
int UnitScript::GetUnitValue(int which, int a, int b, int c, int d)
{
    Unit* unit = state->unit;
    switch (which) {
    case 1:
        return unit->activate_bit0;
    case 2:
        return (unit->flags >> 18) & 3;
    case 3:
        return (unit->flags >> 20) & 3;
    case 4:
        return unit->health * 100 / unit->def->maxHealth;
    case 5:
        return unit->bit0;
    case 6:
        return unit->bit1;
    case 7: {
        Vec3 v;
        v = GetPiecePosition((Object *)unit, a);
        return (v.x & 0xffff0000) + (v.z >> 16);
    }
    case 8: {
        Vec3 v;
        v = GetPiecePosition((Object *)unit, a);
        return v.y;
    }
    case 9: {
        Unit* u = GetUnit((unsigned short)a);
        if (u != 0 && (u->flags & 0x10000000))
            return (u->pos.z >> 16) + (u->pos.x & 0xffff0000);
        break;
    }
    case 10: {
        Unit* u = GetUnit((unsigned short)a);
        if (u != 0 && (u->flags & 0x10000000))
            return u->pos.y;
        break;
    }
    case 11: {
        Unit* u = GetUnit((unsigned short)a);
        if (u != 0 && (u->flags & 0x10000000))
            return u->def->modelMaxY;
        break;
    }
    case 12: {
        int hi = a & 0xffff0000;
        int lo = a << 16;
        if (lo < 0)
            hi += 0x10000;
        return (unsigned short)(AngleOfVector(hi, lo) - unit->heading);
    }
    case 13: {
        int hi = a & 0xffff0000;
        int lo = a << 16;
        if (lo < 0)
            hi += 0x10000;
        return (int)_hypot((double)hi, (double)lo);
    }
    case 14:
        return AngleOfVector(a, b) & 0xffff;
    case 15:
        return (int)_hypot((double)a, (double)b);
    case 16: {
        Vec3 v;
        v.x = a & 0xffff0000;
        v.z = a << 16;
        if (v.z < 0)
            v.x += 0x10000;
        return GetGroundHeight(&v) << 16;
    }
    case 17: {
        int result;
        if (unit->buildLeft == 0.0f)
            result = 0;
        else
            result = 1 - (int)(unit->buildLeft * -99.0f);
        return result;
    }
    case 18:
        return unit->bit2;
    case 19:
        return unit->bit3;
    case 20:
        return unit->activate_bit1;
    }
    return 0;
}

// FUNCTION: 0x480b20
void UnitScript::SetUnitValue(int which, int value)
{
    Unit* unit = state->unit;
    switch (which) {
    case 1:
        unit->SetStateBits(1, value);
        break;
    case 5:
        unit->bit0 = value;
        break;
    case 6:
        unit->bit1 = value;
        break;
    case 18:
        g_seams.map->SetYardOpen(unit, value);
        break;
    case 19:
        unit->bit3 = value;
        break;
    case 20:
        unit->SetStateBits(2, value);
        break;
    }
    unit->dirty = true;
}

// FUNCTION: 0x480c30
int UnitScript::GetPieceTranslation(int index, int slot)
{
    return state->pieces[index].translation[slot];
}

// FUNCTION: 0x480c50
void UnitScript::SetPieceTranslation(int index, int slot, int v)
{
    if (state->pieces[index].translation[slot] != v) {
        state->pieces[index].translation[slot] = v;
        state->pieces[index].value = 0;
        state->dirty = 1;
        if (state->pieces[index].cached) {
            state->unknown_4 = 0;
        }
    }
}

#include "../graphics/model_render.h"

// FUNCTION: 0x480cb0
int UnitScript::GetPieceRotation(int index, int slot)
{
    return state->pieces[index].rotation[slot];
}

// FUNCTION: 0x480ce0
void UnitScript::SetPieceRotation(int index, int slot, int v)
{
    if (state->pieces[index].rotation[slot] != (unsigned short)v) {
        state->pieces[index].rotation[slot] = v;
        state->pieces[index].value = 0;
        state->dirty = 1;
        if (state->pieces[index].cached) {
            state->unknown_4 = 0;
        }
    }
}

// FUNCTION: 0x480d40
void UnitScript::SetObjectState(int newState)
{
    state = (ObjectState*)newState;
}

// FUNCTION: 0x480d50
void UnitScript::SetPieceVisible(int index, int flag)
{
    if (state->pieces[index].visible != flag) {
        state->pieces[index].visible = flag;
        state->pieces[index].value = 0;
        if (state->pieces[index].cached) {
            state->unknown_4 = 0;
        }
    }
}

// FUNCTION: 0x480db0
void UnitScript::SetPieceCached(int index, int flag)
{
    state->pieces[index].cached = flag;
    state->field_10 = 0;
}

// FUNCTION: 0x480df0
void UnitScript::SetPieceShaded(int index, int flag)
{
    state->pieces[index].shaded = flag;
    state->field_10 = 0;
}

// FUNCTION: 0x480e30
int UnitScript::IsPieceVisible(int index)
{
    return state->pieces[index].visible;
}

// FUNCTION: 0x480e50
int UnitScript::IsPieceCached(int index)
{
    return state->pieces[index].cached;
}

// FUNCTION: 0x480e70
int UnitScript::IsPieceShaded(int index)
{
    return state->pieces[index].shaded;
}

// FUNCTION: 0x480e90
void UnitScript::ExplodeLegacy(int, int, int)
{
}

// FUNCTION: 0x480ea0
void UnitScript::PlaySoundNoop(int)
{
}

// FUNCTION: 0x480eb0
void UnitScript::EmitSfx(int a, int b)
{
    if (!g_seams.los_radar->IsUnitVisibleToPlayer(&g_game->players[g_game->playerIndex], state->unit))
        return;
    UpdateObjectState((Unit_459200 *)state->unit);

    Vec3 v1;
    Vec3 v2;

    if (b & 0x100) {
        v1 = state->unit->pos;
        v1.x += state->pieces[a].pos.x;
        v1.y += state->pieces[a].pos.y;
        v1.z -= state->pieces[a].pos.z;
    } else {
        Unit* u = state->unit;
        v2 = u->pos;
        v1 = u->pos;
        v1.x += state->pieces[a].sfxOffset[0];
        v1.y += state->pieces[a].sfxOffset[1];
        v1.z -= state->pieces[a].sfxOffset[2];
        v2.x += state->pieces[a].sfxOffset[3];
        v2.y += state->pieces[a].sfxOffset[4];
        v2.z -= state->pieces[a].sfxOffset[5];
    }

    switch (b) {
    case 0:
        EmitThrustParticles((int)&v1, (int)&v2, 1, 6, 7);
        break;
    case 1:
        EmitThrustParticles((int)&v1, (int)&v2, 1, 7, 7);
        break;
    case 2:
        EmitWakeParticles((int)&v1, (int)&v2, 0x10, 2);
        break;
    case 3:
        EmitWakeParticles((int)&v1, (int)&v2, 8, 2);
        break;
    case 4:
        EmitWakeParticles((int)&v2, (int)&v1, 0x10, 2);
        break;
    case 5:
        EmitWakeParticles((int)&v2, (int)&v1, 8, 2);
        break;
    case 0x101:
        EmitWhiteSmoke((int)&v1, 9);
        break;
    case 0x102:
        EmitBlackSmoke((int)&v1, 9);
        break;
    case 0x103:
        v2.x = v1.x;
        v2.y = v1.y;
        v2.z = v1.z;
        v2.y = g_game->seaLevel << 16;
        EmitBubbles((int)&v1, (int)&v2, 8, 7);
        break;
    }
}

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
        v = g_seams.units->GetPiecePosition(state->unit, a);
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

// FUNCTION: 0x481340
void UnitScript::AttachUnit(unsigned short id, int a, int b)
{
    Unit* u = GetUnit(id);
    if (u != 0 && (u->flags & 0x10000000)
        && (u->carrier == 0 || u->carrier == state->unit)) {
        AttachUnitToPiece(u, state->unit, a, b);
    }
}

// FUNCTION: 0x4813b0
void UnitScript::DropUnit(unsigned short id)
{
    Unit* u = GetUnit(id);
    if (u != 0 && (u->flags & 0x10000000) && u->carrier == state->unit) {
        if (g_seams.map->CanPlaceUnitFootprint(u->def, (unsigned short)u->id, *(Point16*)&u->cell, 1)) {
            AttachUnitToPiece(u, 0, -1, 1);
        }
    }
}

// FUNCTION: 0x481430
int UnitScript::IsCarryingUnit(int id)
{
    Unit* p = state->unit->cargo;
    while (p) {
        if (p->id == id)
            return 1;
        p = p->cargoNext;
    }
    return 0;
}

// FUNCTION: 0x481470
int UnitScript::GetTransporterId()
{
    Unit* t = (Unit*)state->unit->carrier;
    return t ? t->id : 0;
}
