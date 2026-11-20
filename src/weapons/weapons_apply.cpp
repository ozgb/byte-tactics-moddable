#include <stddef.h>

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
#include "../../interfaces/weapons.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"
#include "../../interfaces/state_writer.h"
#include "damage.h"
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
    if (slot < 0 || slot > 2)
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
}
