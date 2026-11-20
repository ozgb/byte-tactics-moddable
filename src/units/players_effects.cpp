#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/players.h"
#include "../../interfaces/services.h"
#include "../../interfaces/ref.h"
#include "../network/player.h"
#include "../game/game.h"
#include "../util/angles.h"
#include "../util/vec3.h"
#include "../weapons/unit_weapon_slot.h"
#include "unit.h"
#include "players_effects.h"

#include "../../link/globals.h"
#include "../game/weapons_events.h"

static Unit* UnitOfEvent(const PlayersEventHeader* h)
{
    Ref<Unit> ref;
    ref.offset = h->unit;
    return ref.get();
}

void UnitsOnPlayersKillUnit(const void* event, unsigned int size)
{
    if (size < sizeof(PlayersKillUnit))
        return;
    const PlayersKillUnit* e = static_cast<const PlayersKillUnit*>(event);
    Unit* u = UnitOfEvent(&e->h);
    if (e->selfDestruct == 0) {
        WeaponsPublishDamage(SCHED_UNITS, u, u, 0x7530, 3, 0);
    } else {
        WeaponsPublishDetonate(SCHED_UNITS, u, 1);
        u->flags |= 0x4000;
        g_seams.units->KillUnit(u, 3);
    }
}

void UnitsOnPlayersSendUnit(const void* event, unsigned int size)
{
    if (size < sizeof(PlayersSendUnit))
        return;
    const PlayersSendUnit* e = static_cast<const PlayersSendUnit*>(event);
    Unit* unit = UnitOfEvent(&e->h);
    Player* other = &g_game->players[e->player];
    unit->flags &= ~0x10;
    unit->postTransferHoldoff = 0x96;
    g_seams.network->SendOwnershipTransfer(unit, other);
    WeaponsPublishDamage(SCHED_UNITS, 0, unit, 30000, 4, 0);
}

void UnitsOnPlayersCaptureUnit(const void* event, unsigned int size)
{
    if (size < sizeof(PlayersCaptureUnit))
        return;
    const PlayersCaptureUnit* e = static_cast<const PlayersCaptureUnit*>(event);
    Unit* unit = UnitOfEvent(&e->h);
    Player* other = &g_game->players[e->player];
    Unit* n = g_seams.units->CreateUnit(other->index, unit->unitDefIndex, *(Vec3*)&unit->pos, 1, unit->flags & 3, 0);
    if (!n)
        return;
    n->flags &= 0xffc3ffff;
    if (e->record.present) {
        n->health = (short)e->record.hitPoints;
        n->buildLeft = (float)e->record.buildPercent;
        Angles16 angles;
        angles.bank = (short)e->record.angles[0];
        angles.heading = (unsigned short)e->record.angles[1];
        angles.pitch = (short)e->record.angles[2];
        n->angles = angles;
        WeaponsPublishSetStockpiles(SCHED_UNITS, n, (unsigned char)e->record.stockpiles[0], (unsigned char)e->record.stockpiles[1], (unsigned char)e->record.stockpiles[2]);
    } else {
        n->health = unit->health;
        n->speed_bits = unit->speed_bits;
        n->angles = unit->angles;
        WeaponsPublishSetStockpiles(SCHED_UNITS, n, unit->weapons[0].stockpile, unit->weapons[1].stockpile, unit->weapons[2].stockpile);
        WeaponsPublishDamage(SCHED_UNITS, 0, unit, 30000, 4, 0);
    }
    g_seams.units->SetStateBits(n, unit->activateFlags, 1);
    g_seams.units->SetStateBits(n, ~unit->activateFlags, 0);
}
