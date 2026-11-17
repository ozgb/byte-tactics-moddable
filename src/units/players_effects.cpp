#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/players.h"
#include "../../interfaces/ref.h"
#include "../network/player.h"
#include "../game/game.h"
#include "../util/angles.h"
#include "../util/vec3.h"
#include "../weapons/unit_weapon_slot.h"
#include "unit.h"
#include "players_effects.h"

#include "../../link/globals.h"

static Unit* UnitOfEvent(const PlayersEvent* e)
{
    Ref<Unit> ref;
    ref.offset = e->unit;
    return ref.get();
}

void UnitsOnPlayersKillUnit(const void* event)
{
    const PlayersEvent* e = static_cast<const PlayersEvent*>(event);
    Unit* u = UnitOfEvent(e);
    if (e->value == 0) {
        g_seams.weapons->DamageUnit(u, u, 0x7530, 3, 0);
    } else {
        g_seams.weapons->DetonateUnitWeapon(u, 1);
        u->flags |= 0x4000;
        g_seams.units->KillUnit(u, 3);
    }
}

void UnitsOnPlayersSendUnit(const void* event)
{
    const PlayersEvent* e = static_cast<const PlayersEvent*>(event);
    Unit* unit = UnitOfEvent(e);
    Player* other = &g_game->players[e->player];
    unit->flags &= ~0x10;
    unit->postTransferHoldoff = 0x96;
    g_seams.network->SendOwnershipTransfer(unit, other);
    g_seams.weapons->DamageUnit(0, unit, 30000, 4, 0);
}

void UnitsOnPlayersCaptureUnit(const void* event)
{
    const PlayersEvent* e = static_cast<const PlayersEvent*>(event);
    Unit* unit = UnitOfEvent(e);
    Player* other = &g_game->players[e->player];
    Unit* n = g_seams.units->CreateUnit(other->index, unit->unitDefIndex, *(Vec3*)&unit->pos, 1, unit->flags & 3, 0);
    if (!n)
        return;
    n->flags &= 0xffc3ffff;
    if (e->record) {
        n->health = (short)e->hitPoints;
        n->buildLeft = (float)e->buildPercent;
        Angles16 angles;
        angles.bank = e->angles[0];
        angles.heading = (unsigned short)e->angles[1];
        angles.pitch = e->angles[2];
        n->angles = angles;
        if (n->weapons[0].flags & 2)
            n->weapons[0].stockpile = e->stockpiles[0];
        if (n->weapons[1].flags & 2)
            n->weapons[1].stockpile = e->stockpiles[1];
        if (n->weapons[2].flags & 2)
            n->weapons[2].stockpile = e->stockpiles[2];
    } else {
        n->health = unit->health;
        n->speed_bits = unit->speed_bits;
        n->angles = unit->angles;
        if (n->weapons[0].flags & 2)
            n->weapons[0].stockpile = unit->weapons[0].stockpile;
        if (n->weapons[1].flags & 2)
            n->weapons[1].stockpile = unit->weapons[1].stockpile;
        if (n->weapons[2].flags & 2)
            n->weapons[2].stockpile = unit->weapons[2].stockpile;
        g_seams.weapons->DamageUnit(0, unit, 30000, 4, 0);
    }
    g_seams.units->SetStateBits(n, unit->activateFlags, 1);
    g_seams.units->SetStateBits(n, ~unit->activateFlags, 0);
}
