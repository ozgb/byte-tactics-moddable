#include <stddef.h>
#include <stdio.h>
#include <string.h>

#pragma pack(push, 1)
#include "../weapons/weapon_def.h"
#include "../weapons/unit_weapon_slot.h"
#include "../orders/order.h"
#include "../units/unit_def.h"
#include "../network/player.h"
#include "../units/unit.h"
#include "../game/game.h"
#pragma pack(pop)

#include "../util/vec3.h"
#include "../state/sim_globals.h"
#include "../../interfaces/weapons.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"
#include "../../interfaces/state_writer.h"
#include "damage.h"
#include "projectile.h"
#include "debris.h"
#include "explosions.h"
#include "piece_explode.h"
#include "weapon_slot_flags.h"
#include "weapons.h"

#include "../game/weapons_events.h"
#include "../../link/globals.h"
#include "weapons_apply.h"

extern void* g_weaponAimCobVtable[];

static Unit* UnitOfOffset(unsigned int offset)
{
    Ref<Unit> ref;
    ref.offset = offset;
    return ref.get();
}

static int KnownKind(int kind)
{
    return kind == WEAPONS_DAMAGE || kind == WEAPONS_REMOTE_DAMAGE || (kind >= WEAPONS_DETONATE && kind <= WEAPONS_SET_STOCKPILES) ||
           kind == WEAPONS_ADD_STOCKPILE;
}

static int Valid(const WeaponsEventHeader* e, unsigned int size, Unit* unit)
{
    if (!KnownKind(e->kind))
        return 0;
    if (size < WeaponsEventSize(e->kind))
        return 0;
    if (e->kind != WEAPONS_SPAWN && e->kind != WEAPONS_AREA_DAMAGE && e->kind != WEAPONS_AIM_DONE && unit == nullptr)
        return 0;
    switch (e->kind) {
    case WEAPONS_RELEASE: {
        const WeaponsRelease* p = reinterpret_cast<const WeaponsRelease*>(e);
        return p->weapon >= 0 && p->weapon <= 3;
    }
    case WEAPONS_CLAIM: {
        const WeaponsClaim* p = reinterpret_cast<const WeaponsClaim*>(e);
        return p->weapon >= 0 && p->weapon <= 3;
    }
    case WEAPONS_ADD_STOCKPILE: {
        const WeaponsAddStockpile* p = reinterpret_cast<const WeaponsAddStockpile*>(e);
        return p->weapon >= 0 && p->weapon <= 2;
    }
    case WEAPONS_AIM_DONE:
        return reinterpret_cast<const WeaponsAimDone*>(e)->callback != 0;
    case WEAPONS_SPAWN: {
        const WeaponsSpawn* p = reinterpret_cast<const WeaponsSpawn*>(e);
        return p->weapon >= -1 && p->weapon < 256;
    }
    case WEAPONS_AREA_DAMAGE: {
        const WeaponsAreaDamage* p = reinterpret_cast<const WeaponsAreaDamage*>(e);
        return p->weapon >= 0 && p->weapon < 256;
    }
    }
    return 1;
}

void __stdcall WeaponsLoadState(Unit* unit, int slot, const WeaponsSlotState* state)
{
    if (unit == nullptr || slot < 0 || slot > 2)
        return;
    UnitWeaponSlot* s = &unit->weapons[slot];
    s->aimCob.nAimCobDone = state->aimDone;
    ((unsigned char*)s->weapon.get())[0x10a] = (unsigned char)state->typeByte;
    s->muzzleAimFromDeltaZ = state->aimOffset;
    s->reloadTimer = (unsigned short)state->reload;
    s->fireHeading = (short)state->heading;
    s->firePitch = (short)state->pitch;
    s->stockpile = (unsigned char)state->stockpile;
    WeaponSlotFlagBits* flags = (WeaponSlotFlagBits*)&s->flags;
    flags->bits.bit0 = state->flags & 1;
    flags->bits.bit1 = state->flags >> 1 & 1;
    flags->bits.bit2 = state->flags >> 2 & 1;
    flags->bits.bit3 = state->flags >> 3 & 1;
    flags->bits.bit4 = state->flags >> 4 & 1;
}

int __stdcall WeaponsApply(const WeaponsEventHeader* e, unsigned int size)
{
    if (size < sizeof(WeaponsEventHeader))
        return 0;
    Unit* unit = UnitOfOffset(e->unit);
    if (!Valid(e, size, unit))
        return 0;
    switch (e->kind) {
    case WEAPONS_DAMAGE: {
        const WeaponsDamage* p = reinterpret_cast<const WeaponsDamage*>(e);
        DamageUnit(UnitOfOffset(p->source), unit, p->amount, p->damageKind, (unsigned short)p->direction);
        break;
    }
    case WEAPONS_REMOTE_DAMAGE: {
        const WeaponsDamage* p = reinterpret_cast<const WeaponsDamage*>(e);
        Unit* other = UnitOfOffset(p->source);
        UnitDamagePacket packet;
        packet.type = 11;
        packet.victimId = unit->id;
        packet.attackerId = other == nullptr ? 0 : other->id;
        packet.amount = (unsigned short)p->amount;
        packet.headingByte = (unsigned char)(p->direction >> 8);
        packet.damageKind = (unsigned char)p->damageKind;
        ApplyUnitDamage(&packet);
        break;
    }
    case WEAPONS_DETONATE:
        DetonateUnitWeapon(unit, reinterpret_cast<const WeaponsDetonate*>(e)->selfDestruct);
        break;
    case WEAPONS_RELEASE:
        unit->ReleaseWeapons((unsigned char)reinterpret_cast<const WeaponsRelease*>(e)->weapon);
        break;
    case WEAPONS_CLAIM:
        unit->ClaimWeapons((unsigned char)reinterpret_cast<const WeaponsClaim*>(e)->weapon);
        break;
    case WEAPONS_INIT_SLOTS:
        for (int i = 0; i < 3; i++)
            unit->weapons[i].aimCob.ppVtbl = g_weaponAimCobVtable;
        InitUnitWeaponSlots(unit);
        break;
    case WEAPONS_REMOVE_PROJECTILES:
        RemoveUnitProjectiles(unit);
        break;
    case WEAPONS_AIM_DONE: {
        const WeaponsAimDone* p = reinterpret_cast<const WeaponsAimDone*>(e);
        Ref<WeaponAimCobCb> callback;
        callback.offset = p->callback;
        callback.get()->OnAimCobReturn(p->result);
        break;
    }
    case WEAPONS_EXPLODE_PIECE: {
        const WeaponsExplodePiece* p = reinterpret_cast<const WeaponsExplodePiece*>(e);
        ExplodeUnitPiece(unit, p->piece, p->flags);
        break;
    }
    case WEAPONS_AREA_DAMAGE: {
        const WeaponsAreaDamage* p = reinterpret_cast<const WeaponsAreaDamage*>(e);
        Vec3 pos;
        pos.x = p->pos[0];
        pos.y = p->pos[1];
        pos.z = p->pos[2];
        ApplyAreaDamageAt(&g_game->weapons[p->weapon], &pos);
        break;
    }
    case WEAPONS_SPAWN: {
        const WeaponsSpawn* p = reinterpret_cast<const WeaponsSpawn*>(e);
        Vec3 pos;
        pos.x = p->pos[0];
        pos.y = p->pos[1];
        pos.z = p->pos[2];
        Vec3 vel;
        vel.x = p->vel[0];
        vel.y = p->vel[1];
        vel.z = p->vel[2];
        WeaponDef* weapon = p->weapon < 0 ? nullptr : &g_game->weapons[p->weapon];
        return SpawnProjectile(weapon, &pos, &vel, p->announce);
    }
    case WEAPONS_SET_STOCKPILES: {
        const WeaponsSetStockpiles* p = reinterpret_cast<const WeaponsSetStockpiles*>(e);
        for (int i = 0; i < 3; i++)
            if (unit->weapons[i].flags & 2)
                unit->weapons[i].stockpile = (unsigned char)p->stockpiles[i];
        break;
    }
    case WEAPONS_ADD_STOCKPILE:
        unit->weapons[reinterpret_cast<const WeaponsAddStockpile*>(e)->weapon].stockpile++;
        break;
    }
    return 1;
}

enum FieldKind { F_INT, F_BYTES, F_HEAP, F_UNIT };

struct Field {
    const char* name;
    unsigned short offset;
    unsigned char size;
    unsigned char kind;
    unsigned int mask;
};

static const Field kExplodedPiece[] = {
    {"unit", 0, 4, F_UNIT, 0x0},
    {"index", 4, 4, F_INT, 0x0},
    {"angVelZ", 8, 4, F_INT, 0x0},
    {"angVelX", 12, 4, F_INT, 0x0},
    {"angVelY", 16, 4, F_INT, 0x0},
    {"vel_x", 20, 4, F_INT, 0x0},
    {"vel_y", 24, 4, F_INT, 0x0},
    {"vel_z", 28, 4, F_INT, 0x0},
    {"ticks", 32, 4, F_INT, 0x0},
    {"scale", 36, 4, F_INT, 0x0},
    {"flags", 40, 4, F_INT, 0x3f},
    {"rec", 44, 4, F_HEAP, 0x0},
};

static const Field kPieceRec[] = {
    {"rec.desc", 0, 4, F_HEAP, 0x0},
    {"rec.+0x4", 4, 12, F_BYTES, 0x0},
    {"rec.angle_10", 16, 2, F_INT, 0x0},
    {"rec.angle_12", 18, 2, F_INT, 0x0},
    {"rec.angle_14", 20, 2, F_INT, 0x0},
    {"rec.pos.x", 22, 4, F_INT, 0x0},
    {"rec.pos.y", 26, 4, F_INT, 0x0},
    {"rec.pos.z", 30, 4, F_INT, 0x0},
    {"rec.verts", 34, 4, F_HEAP, 0x0},
    {"rec.+0x26", 38, 2, F_BYTES, 0x0},
    {"rec.flags", 40, 2, F_INT, 0x0},
    {"rec.link0", 42, 4, F_HEAP, 0x0},
    {"rec.link1", 46, 4, F_HEAP, 0x0},
    {"rec.link2", 50, 4, F_HEAP, 0x0},
};

static const Field kDebris[] = {
    {"obj", 0, 4, F_HEAP, 0x0},
    {"ref1.index", 4, 2, F_INT, 0x0},
    {"ref1.value", 6, 2, F_INT, 0x0},
    {"ref1.kind", 8, 1, F_INT, 0x0},
    {"+0x9", 9, 3, F_BYTES, 0x0},
    {"ref1.src", 12, 4, F_HEAP, 0x0},
    {"ref2.index", 16, 2, F_INT, 0x0},
    {"ref2.value", 18, 2, F_INT, 0x0},
    {"ref2.kind", 20, 1, F_INT, 0x0},
    {"+0x15", 21, 3, F_BYTES, 0x0},
    {"ref2.src", 24, 4, F_HEAP, 0x0},
    {"pos.x", 28, 4, F_INT, 0x0},
    {"pos.y", 32, 4, F_INT, 0x0},
    {"pos.z", 36, 4, F_INT, 0x0},
    {"size.x", 40, 4, F_INT, 0x0},
    {"size.y", 44, 4, F_INT, 0x0},
    {"size.z", 48, 4, F_INT, 0x0},
    {"vel.x", 52, 4, F_INT, 0x0},
    {"vel.y", 56, 4, F_INT, 0x0},
    {"vel.z", 60, 4, F_INT, 0x0},
    {"spin.x", 64, 4, F_INT, 0x0},
    {"spin.y", 68, 4, F_INT, 0x0},
    {"spin.z", 72, 4, F_INT, 0x0},
    {"angle_x", 76, 2, F_INT, 0x0},
    {"angle_y", 78, 2, F_INT, 0x0},
    {"angle_z", 80, 2, F_INT, 0x0},
    {"flag", 82, 2, F_INT, 0x0},
};

static const Field kPiece[] = {
    {"state", 0, 1, F_INT, 0x0},
    {"+0x1", 1, 3, F_BYTES, 0x0},
    {"vertexCount", 4, 4, F_INT, 0x0},
    {"faceCount", 8, 4, F_INT, 0x0},
    {"firstFace", 12, 4, F_INT, 0x0},
    {"f10", 16, 4, F_INT, 0x0},
    {"f14", 20, 4, F_INT, 0x0},
    {"color", 24, 4, F_HEAP, 0x0},
    {"name", 28, 4, F_HEAP, 0x0},
    {"active", 32, 4, F_INT, 0x0},
    {"vertices", 36, 4, F_HEAP, 0x0},
    {"faces", 40, 4, F_HEAP, 0x0},
    {"sibling", 44, 4, F_HEAP, 0x0},
    {"child", 48, 4, F_HEAP, 0x0},
};

static const Field kFace[] = {
    {"faces[%u].color", 0, 4, F_INT, 0x0},
    {"faces[%u].nverts", 4, 4, F_INT, 0x0},
    {"faces[%u].+0x8", 8, 4, F_INT, 0x0},
    {"faces[%u].vindex", 12, 4, F_HEAP, 0x0},
    {"faces[%u].texFrame", 16, 4, F_HEAP, 0x0},
    {"faces[%u].texKind", 20, 1, F_INT, 0x0},
    {"faces[%u].+0x15", 21, 3, F_BYTES, 0x0},
    {"faces[%u].texSrc", 24, 4, F_HEAP, 0x0},
    {"faces[%u].flags", 28, 4, F_INT, 0x0},
};

static const Field kProjectile[] = {
    {"start.x", 0x10, 4, F_INT, 0},
    {"start.y", 0x14, 4, F_INT, 0},
    {"start.z", 0x18, 4, F_INT, 0},
    {"direction.x", 0x1c, 4, F_INT, 0},
    {"direction.y", 0x20, 4, F_INT, 0},
    {"direction.z", 0x24, 4, F_INT, 0},
    {"angles.bank", 0x34, 2, F_INT, 0},
    {"angles.heading", 0x36, 2, F_INT, 0},
    {"angles.pitch", 0x38, 2, F_INT, 0},
    {"speed", 0x3a, 4, F_INT, 0},
    {"range", 0x3e, 4, F_INT, 0},
    {"spawnTick", 0x42, 4, F_INT, 0},
    {"time", 0x46, 4, F_INT, 0},
    {"nextSmokeTick", 0x4a, 4, F_INT, 0},
    {"+0x4e", 0x4e, 4, F_HEAP, 0},
    {"+0x56", 0x56, 4, F_HEAP, 0},
    {"cellX", 0x5a, 2, F_INT, 0},
    {"cellZ", 0x5c, 2, F_INT, 0},
    {"radius", 0x5e, 2, F_INT, 0},
    {"active", 0x60, 2, F_INT, 0},
    {"piece", 0x62, 2, F_INT, 0},
    {"propellerSpin", 0x64, 2, F_INT, 0},
    {"compactIndex", 0x67, 2, F_INT, 0},
    {"flags", 0x69, 2, F_INT, 0},
};

static unsigned int ReadWord(const unsigned char* p, unsigned int size)
{
    unsigned int v = 0;
    for (unsigned int b = size; b > 0; b--)
        v = (v << 8) | p[b - 1];
    return v;
}

static void WriteFields(const StateWriter* w, const unsigned char* base, const Field* fields, unsigned int count, int index)
{
    for (unsigned int k = 0; k < count; k++) {
        const Field* f = &fields[k];
        const unsigned char* p = base + f->offset;
        switch (f->kind) {
        case F_INT: {
            unsigned int v = ReadWord(p, f->size);
            w->Int(f->name, index, f->size, f->mask != 0 ? v & f->mask : v);
            break;
        }
        case F_BYTES:
            w->Bytes(f->name, index, p, f->size);
            break;
        case F_HEAP:
            w->Handle(f->name, index, STATE_HANDLE_HEAP, ReadWord(p, 4));
            break;
        case F_UNIT:
            w->Handle(f->name, index, STATE_HANDLE_UNIT, ReadWord(p, 4));
            break;
        }
    }
}

#define FIELDS(table) table, sizeof(table) / sizeof(table[0])

static void WriteExplodedPieces(const StateWriter* w)
{
    char name[64];
    for (unsigned int s = 0; s < 100; s++) {
        unsigned int offset = g_explodedPieces[s].offset;
        if (offset == 0)
            continue;
        const unsigned char* p = (const unsigned char*)SimBlockBase() + offset;
        sprintf(name, "weaponsState.explodedPieces[%u]", s);
        w->Begin(48, name);
        WriteFields(w, p, FIELDS(kExplodedPiece), 0);
        WriteFields(w, p + 0x30, FIELDS(kPieceRec), 0);
    }
}

static void WriteDebrisAndPieces(const StateWriter* w)
{
    char name[64];
    unsigned char used[300];
    unsigned int n = (unsigned int)g_game->count_1491b;
    memset(used, 0, sizeof used);
    if (n > 300)
        n = 300;
    for (unsigned int d = 0; d < n; d++) {
        const Debris* e = &g_game->debris[d];
        unsigned int pieceOffset = ReadWord((const unsigned char*)e, 4);
        if (pieceOffset == 0)
            continue;
        sprintf(name, "weaponsState.debris[%u]", d);
        w->Begin(48, name);
        WriteFields(w, (const unsigned char*)e, FIELDS(kDebris), 0);
        const unsigned char* obj = (const unsigned char*)SimBlockBase() + pieceOffset;
        const unsigned char* slots = (const unsigned char*)&g_game->slots[0];
        if (obj >= slots && (unsigned int)(obj - slots) % sizeof(DebrisPiece) == 0 && (unsigned int)(obj - slots) / sizeof(DebrisPiece) < 300)
            used[(unsigned int)(obj - slots) / sizeof(DebrisPiece)] = 1;
    }
    for (unsigned int i = 0; i < 300; i++) {
        if (!used[i])
            continue;
        sprintf(name, "weaponsState.pieces[%u]", i);
        w->Begin(48, name);
        WriteFields(w, (const unsigned char*)&g_game->slots[i], FIELDS(kPiece), 0);
        w->Bytes("vertices", 0, g_game->vertices[i], 96);
        for (unsigned int k = 0; k < 6; k++)
            WriteFields(w, (const unsigned char*)&g_game->frames[i][k], FIELDS(kFace), (int)k);
    }
}

static void WriteUnitSlots(const StateWriter* w)
{
    Unit* units = g_game->units.get();
    char name[64];
    for (unsigned int i = 0; units != nullptr && i < g_game->poolCount; i++) {
        Unit* u = &units[i];
        if (u->unitDefIndex == 0)
            continue;
        sprintf(name, "weaponsState.units[%u]", i);
        w->Begin(48, name);
        for (int k = 0; k < 3; k++) {
            UnitWeaponSlot* s = &u->weapons[k];
            w->Handle("weapons[%u].aimCob.vtable", k, STATE_HANDLE_HEAP, (unsigned int)s->aimCob.ppVtbl);
            w->Int("weapons[%u].aimCob.done", k, 4, (unsigned int)s->aimCob.nAimCobDone);
            w->Int("weapons[%u].muzzleAimFromDeltaZ", k, 4, (unsigned int)s->muzzleAimFromDeltaZ);
            w->Int("weapons[%u].reloadTimer", k, 2, s->reloadTimer);
            w->Int("weapons[%u].fireHeading", k, 2, (unsigned short)s->fireHeading);
            w->Int("weapons[%u].firePitch", k, 2, (unsigned short)s->firePitch);
        }
    }
}

static void WriteProjectiles(const StateWriter* w)
{
    char name[64];
    const unsigned char* base = (const unsigned char*)g_game->projectiles.get();
    for (int p = 0; base != nullptr && p < g_game->projectileCount; p++) {
        const unsigned char* e = base + p * sizeof(Projectile);
        if (*(const unsigned short*)(e + 0x69) & 2)
            continue;
        sprintf(name, "weaponsState.projectiles[%u]", (unsigned int)p);
        w->Begin(48, name);
        WriteFields(w, e, FIELDS(kProjectile), 0);
    }
}

void __stdcall WeaponsWriteState(const StateWriter* w)
{
    unsigned int explosions;
    unsigned int debris;
    unsigned int pieces;
    CountExplosionTables(&explosions, &debris, &pieces);
    w->Begin(48, "weaponsState");
    w->Int("projectiles", 0, 4, (unsigned int)g_game->projectileCount);
    w->Int("explosions", 0, 4, explosions);
    w->Int("debris", 0, 4, debris);
    w->Int("pieces", 0, 4, pieces);
    WriteExplodedPieces(w);
    WriteDebrisAndPieces(w);
    WriteUnitSlots(w);
    WriteProjectiles(w);
}
