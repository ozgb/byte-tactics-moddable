#include <stddef.h>

#pragma pack(push, 1)
#include "../units/unit_type_set.h"
#include "../weapons/weapon_def.h"
#include "../weapons/unit_weapon_slot.h"
#include "../orders/order.h"
#include "../units/unit_def.h"
#include "../network/player.h"
#include "../units/unit.h"
#include "../game/game.h"
#pragma pack(pop)

#include "../util/vec3.h"
#include "../../interfaces/seams.h"
#include "../../interfaces/weapons.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"
#include "../../interfaces/state_writer.h"
#include "damage.h"
#include "explosions.h"
#include "piece_explode.h"
#include "weapon_slot_flags.h"
#include "weapons.h"

#include "../../link/globals.h"

extern void* g_weaponAimCobVtable[];

static Unit* UnitOfOffset(unsigned int offset)
{
    Ref<Unit> ref;
    ref.offset = offset;
    return ref.get();
}

static int KnownKind(int kind)
{
    return kind == WEAPONS_DAMAGE || kind == WEAPONS_REMOTE_DAMAGE || (kind >= WEAPONS_DETONATE && kind <= WEAPONS_LOAD_SLOT);
}

static int Valid(const WeaponsEvent* e, Unit* unit)
{
    if (!KnownKind(e->kind))
        return 0;
    if (e->kind != WEAPONS_SPAWN && e->kind != WEAPONS_AREA_DAMAGE && e->kind != WEAPONS_AIM_DONE && unit == nullptr)
        return 0;
    switch (e->kind) {
    case WEAPONS_RELEASE:
    case WEAPONS_CLAIM:
        return e->value >= 0 && e->value <= 3;
    case WEAPONS_SET_STOCKPILES:
    case WEAPONS_LOAD_SLOT:
        return e->value >= 0 && e->value <= 2;
    case WEAPONS_AIM_DONE:
        return e->callback != 0;
    case WEAPONS_SPAWN:
        return e->value >= -1 && e->value < 256;
    case WEAPONS_AREA_DAMAGE:
        return e->value >= 0 && e->value < 256;
    }
    return 1;
}

static void LoadSlot(Unit* unit, int slot, const int* args)
{
    UnitWeaponSlot* s = &unit->weapons[slot];
    s->aimCob.nAimCobDone = args[0];
    ((unsigned char*)s->weapon.get())[0x10a] = (unsigned char)args[1];
    s->muzzleAimFromDeltaZ = args[2];
    s->reloadTimer = (unsigned short)args[3];
    s->fireHeading = (short)args[4];
    s->firePitch = (short)args[5];
    s->stockpile = (unsigned char)args[6];
    WeaponSlotFlagBits* flags = (WeaponSlotFlagBits*)&s->flags;
    flags->bits.bit0 = args[7] & 1;
    flags->bits.bit1 = args[7] >> 1 & 1;
    flags->bits.bit2 = args[7] >> 2 & 1;
    flags->bits.bit3 = args[7] >> 3 & 1;
    flags->bits.bit4 = args[7] >> 4 & 1;
}

int __stdcall WeaponsApply(const WeaponsEvent* e)
{
    Unit* unit = UnitOfOffset(e->unit);
    if (!Valid(e, unit))
        return 0;
    Unit* other = UnitOfOffset(e->target);
    switch (e->kind) {
    case WEAPONS_DAMAGE:
        DamageUnit(other, unit, e->value, e->extra, (unsigned short)e->args[0]);
        break;
    case WEAPONS_REMOTE_DAMAGE: {
        UnitDamagePacket packet;
        packet.type = 11;
        packet.victimId = unit->id;
        packet.attackerId = other == nullptr ? 0 : other->id;
        packet.amount = (unsigned short)e->value;
        packet.headingByte = (unsigned char)(e->args[0] >> 8);
        packet.damageKind = (unsigned char)e->extra;
        ApplyUnitDamage(&packet);
        break;
    }
    case WEAPONS_DETONATE:
        DetonateUnitWeapon(unit, e->value);
        break;
    case WEAPONS_RELEASE:
        unit->ReleaseWeapons((unsigned char)e->value);
        break;
    case WEAPONS_CLAIM:
        unit->ClaimWeapons((unsigned char)e->value);
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
        Ref<WeaponAimCobCb> callback;
        callback.offset = e->callback;
        callback.get()->OnAimCobReturn(e->value);
        break;
    }
    case WEAPONS_EXPLODE_PIECE:
        ExplodeUnitPiece(unit, e->value, (unsigned int)e->extra);
        break;
    case WEAPONS_AREA_DAMAGE: {
        Vec3 pos;
        pos.x = e->pos[0];
        pos.y = e->pos[1];
        pos.z = e->pos[2];
        ApplyAreaDamageAt(&g_game->weapons[e->value], &pos);
        break;
    }
    case WEAPONS_SPAWN: {
        Vec3 pos;
        pos.x = e->pos[0];
        pos.y = e->pos[1];
        pos.z = e->pos[2];
        Vec3 vel;
        vel.x = e->vel[0];
        vel.y = e->vel[1];
        vel.z = e->vel[2];
        WeaponDef* weapon = e->value < 0 ? nullptr : &g_game->weapons[e->value];
        return SpawnProjectile(weapon, &pos, &vel, e->extra);
    }
    case WEAPONS_SET_STOCKPILES:
        for (int i = 0; i < 3; i++)
            if (unit->weapons[i].flags & 2)
                unit->weapons[i].stockpile = (unsigned char)e->args[i];
        break;
    case WEAPONS_LOAD_SLOT:
        LoadSlot(unit, e->value, e->args);
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
