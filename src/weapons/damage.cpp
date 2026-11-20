#include <stddef.h>

#pragma pack(push, 1)
#include "../orders/order.h"
#include "../units/unit_def.h"
#include "../network/player.h"
#include "../units/unit.h"
#include "../network/wire_packets.h"
#include "../game/game.h"
#pragma pack(pop)

#include "../../interfaces/seams.h"
#include "../../interfaces/network.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"
#include "../game/weapons_events.h"
#include "../state/sim_effects.h"
#include "../util/math.h"
#include "damage.h"

#include "../../link/globals.h"

// FUNCTION: 0x489bb0
void __stdcall DamageUnit(Unit* source, Unit* target, int amount, int type, unsigned short extra)
{
    int dmg;
    if (type != 10) {
        if ((target->activateFlags & 2) && amount < 0x7530)
            amount = (int)(((__int64)target->def->damagemodifier * amount) >> 0x10);
        int armour = target->killCount / 5;
        if (armour > 5)
            armour = 5;
        dmg = (25 - armour) * amount * 4 / 100;
    } else {
        dmg = amount;
    }
    UnitDamagePacketOut d;
    d.type = 11;
    d.victimId = !target ? 0 : target->id;
    d.attackerId = !source ? 0 : source->id;
    d.amount = dmg;
    d.headingByte = (unsigned char)(extra >> 8);
    d.damageKind = type;
    ApplyUnitDamage((UnitDamagePacket *)&d);
    if (target->player->active != 0 && target->player->type == 3 && type != 11)
        g_seams.network->SendUnitDamage(source, target, dmg, type, extra);
}

// FUNCTION: 0x489ce0
void __stdcall ApplyUnitDamage(UnitDamagePacket* ev)
{
    Unit* unit = ev->victimId == 0 ? 0 : &g_game->units[ev->victimId];
    Unit* target = ev->attackerId == 0 ? 0 : &g_game->units[ev->attackerId];

    if (unit == 0)
        return;
    if (!(unit->flags & 0x10000000))
        return;
    if (unit->flags & 0x4000)
        return;

    if (ev->damageKind == 10) {
        WeaponsPublishHealth(unit, (short)((unsigned int)(unit->health + ev->amount) < unit->def->maxHealth
                           ? unit->health + ev->amount : unit->def->maxHealth), 0);
        return;
    }

    WeaponsPublishMarkDamaged(unit);

    if (ev->damageKind != 11)
        WeaponsPublishAttacked(unit, target, ev->amount);

    WeaponsPublishDamageRecord(unit, ev->damageKind, target);
    if (target) {
        if (target->playerIndex == g_game->localPlayer || unit->playerIndex == g_game->localPlayer)
            RecordCdActivity(1, target->playerIndex, unit->playerIndex);
    }

    if (ev->damageKind == 2) {
        int ticks = ev->amount;
        if (unit->flags & 0x10000000) {
            if (!(unit->flags & 0x4000)) {
                Player* owner = unit->player;
                if (owner->active && (owner->type == 1 || owner->type == 2)) {
                    if (!(unit->def->flags1 & 0x4000000))
                        WeaponsPublishParalyze(unit, ticks);
                }
            }
        }
    } else {
        short health = (short)(unit->health - ev->amount);
        if (health <= 0) {
            if (unit->player->active && (unit->player->type == 1 || unit->player->type == 2)) {
                WeaponsPublishHealth(unit, health, 1);
                return;
            }
            health = 0;
        }
        WeaponsPublishHealth(unit, health, 0);
        if (ev->damageKind == 1) {
            int a = ScaledCos((unsigned short)(ev->headingByte << 8), 400);
            int b = ScaledSin((short)(ev->headingByte << 8), 400);
            WeaponsPublishStartScriptWithArgs(unit, WEAPONS_SCRIPT_HIT_BY_WEAPON, 0, 0, 2, a, b, 0, 0);
            int pct = health * 100 / unit->def->maxHealth;
            if (pct < 0)
                pct = 0;
            if (pct > 100)
                pct = 100;
            WeaponsPublishStartScriptWithArgs(unit, WEAPONS_SCRIPT_TAKE_DAMAGE, 0, 0, 1, pct, 0, 0, 0);
        }
    }
}

// FUNCTION: 0x489fa0
void __stdcall ParalyzeUnit(Unit* unit, int ticks)
{
    if (!(unit->flags & 0x10000000))
        return;
    if (unit->flags & 0x4000)
        return;
    Player* owner = unit->player;
    if (!owner->active)
        return;
    if (owner->type == 1 || owner->type == 2) {
        if (!(unit->def->flags1 & 0x4000000))
            WeaponsPublishParalyze(unit, ticks);
    }
}
