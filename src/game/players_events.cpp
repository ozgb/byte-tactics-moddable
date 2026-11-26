#include "../../interfaces/ref.h"

#include "mission_conditions.h"
#include "../game/unit_resources.h"
#include "players.h"
#include "../../interfaces/players.h"
#include "../units/unit_commands.h"
#include "../units/units.h"
#include "economy_events.h"
#include "../../interfaces/economy.h"

struct Player;
struct Unit;
class Pathfinder;

#pragma pack(push, 1)

#include "../ai/squad_manager.h"

struct UnitDef;

#include "../units/unit.h"

#include "../../interfaces/seams.h"

#include "../network/player.h"
#include "../network/player_info.h"

#include "game.h"
#include "los_helpers.h"
#pragma pack(pop)

#include "../../link/globals.h"
#include "players_events.h"
#include "../network/net_game.h"
#include "../state/sim_globals.h"

// FUNCTION: 0x464b30
int __stdcall TransferMetal(unsigned char from, unsigned char to, float amount, int flag)
{
    if (from == 10)
        return 0;
    if (to == 10)
        return 0;
    if (flag) {
        float cap = g_game->players[from].energy;
        if (amount > cap)
            amount = cap;
    }
    if (amount == 0.0f)
        return 0;
    if (flag)
        EconSpend(from, 0, ECON_ENERGY, amount, 0);
    EconPublish(SCHED_PLAYERS, ECONOMY_INCOME, to, 0, ECON_ENERGY, amount, 0);
    if (flag)
        g_seams.network->SendShareMetal(from, to, amount);
    return 1;
}

// FUNCTION: 0x464c60
int __stdcall TransferEnergy(unsigned char from, unsigned char to, float amount, int flag)
{
    if (from == 10)
        return 0;
    if (to == 10)
        return 0;
    if (flag) {
        float cap = g_game->players[from].metal;
        if (amount > cap)
            amount = cap;
    }
    if (amount == 0.0f)
        return 0;
    if (flag)
        EconSpend(from, 0, ECON_METAL, 0, amount);
    EconPublish(SCHED_PLAYERS, ECONOMY_INCOME, to, 0, ECON_METAL, 0, amount);
    if (flag)
        g_seams.network->SendShareEnergy(from, to, amount);
    return 1;
}

int __stdcall PlayersApply(const PlayersEventHeader* event, unsigned int size)
{
    if (size < sizeof(PlayersEventHeader))
        return 0;
    if (size < PlayersEventSize(event->kind))
        return 0;
    Ref<Unit> ref;
    ref.offset = event->unit;
    Unit* unit = ref.get();
    switch (event->kind) {
    case PLAYERS_GIVE_UNIT: {
        const PlayersGiveUnit* e = reinterpret_cast<const PlayersGiveUnit*>(event);
        if (unit == nullptr || (unsigned)e->player > 9 || g_game->list_391ed == nullptr)
            return 0;
        return GiveUnitToPlayer(unit, &g_game->players[e->player], e);
    }
    case PLAYERS_KILL_UNITS: {
        const PlayersKillUnits* e = reinterpret_cast<const PlayersKillUnits*>(event);
        if ((unsigned)e->player > 10)
            return 0;
        KillPlayerUnits((unsigned char)e->player);
        return 1;
    }
    case PLAYERS_UNIT_CREATED:
        if (unit == nullptr || g_game->list_391ed == nullptr)
            return 0;
        g_game->list_391ed->NotifyUnitCreated(unit);
        return 1;
    case PLAYERS_UNIT_DIED:
        if (unit == nullptr || g_game->list_391ed == nullptr)
            return 0;
        g_game->list_391ed->NotifyUnitDied(unit);
        return 1;
    case PLAYERS_TRANSFER: {
        const PlayersTransfer* e = reinterpret_cast<const PlayersTransfer*>(event);
        if ((unsigned)e->from > 10 || (unsigned)e->to > 10)
            return 0;
        if (e->resource == 0)
            return TransferMetal((unsigned char)e->from, (unsigned char)e->to, e->amount, e->announce);
        if (e->resource == 1)
            return TransferEnergy((unsigned char)e->from, (unsigned char)e->to, e->amount, e->announce);
        return 0;
    }
    case PLAYERS_SET_STORE: {
        const PlayersSetStore* e = reinterpret_cast<const PlayersSetStore*>(event);
        if ((unsigned)e->player > 9 || (e->resource != 0 && e->resource != 1))
            return 0;
        Player* p = &g_game->players[e->player];
        float* store = e->resource == 0 ? &p->energy : &p->metal;
        if (e->add)
            *store += e->amount;
        else
            *store = e->amount;
        return 1;
    }
    case PLAYERS_SET_TYPE: {
        const PlayersSetType* e = reinterpret_cast<const PlayersSetType*>(event);
        if ((unsigned)e->player > 9)
            return 0;
        Player* p = &g_game->players[e->player];
        p->type = (unsigned char)e->type;
        if (e->type != 3)
            p->info->kind = (char)e->type;
        return 1;
    }
    case PLAYERS_END_GAME: {
        const PlayersEndGame* e = reinterpret_cast<const PlayersEndGame*>(event);
        if (e->victory != 0 && e->victory != 1)
            return 0;
        g_game->endGameVictory = e->victory;
        if (e->victory == 1)
            g_game->bits.flag5 = 1;
        else
            g_game->bits.flag6 = 1;
        g_game->bits.flag2 = 1;
        return 1;
    }
    case PLAYERS_SET_CHEATS: {
        const PlayersSetCheats* e = reinterpret_cast<const PlayersSetCheats*>(event);
        if ((e->clear | e->set) & ~0x788u)
            return 0;
        g_game->uiOptionFlags = (unsigned short)((g_game->uiOptionFlags & ~e->clear) | e->set);
        return 1;
    }
    case PLAYERS_LEAVE_GAME:
        g_game->bits.flag2 = 1;
        return 1;
    case PLAYERS_DEACTIVATE_CONDITIONS:
        g_game->list_391ed->Deactivate();
        return 1;
    case PLAYERS_COMPACT_RANKS:
        if (g_quirks.rankCompactionPerTick)
            return 0;
        CompactPlayerRanks();
        return 1;
    }
    return 0;
}

static bool CountsForRank(const Player* p)
{
    if (p->active == 0)
        return false;
    if (p->type != 1 && p->type != 2 && p->type != 3)
        return false;
    if (p->index == 0xa)
        return false;
    if (p->unitCount == 0 && p->unitsCreated != 0)
        return false;
    return (p->info->flags_9b & 0x40) == 0;
}

void __stdcall CompactPlayerRanks()
{
    for (int i = 0; i < (int)g_game->numPlayers; i++) {
        int held = 0;
        for (int n = 0; n < 10; n++) {
            const Player* p = &g_game->players[n];
            if (CountsForRank(p) && p->rank == i) {
                held = 1;
                break;
            }
        }
        if (held)
            continue;
        for (int n = 0; n < 10; n++) {
            Player* p = &g_game->players[n];
            if (CountsForRank(p) && p->rank > i)
                p->rank--;
        }
    }
}

// FUNCTION: 0x486f10
void __stdcall KillPlayerUnits(unsigned char player)
{
    Player* p = &g_game->players[player];
    if (p != 0) {
        if (p->unitCount != 0) {
            Unit* u = g_game->players[player].unitsBegin;
            Unit* last = g_game->players[player].unitsEnd;
            if (u != 0) {
                for (; u <= last; u++) {
                    unsigned int flags = u->flags;
                    if (flags & 0x10000000) {
                        if (!(flags & 0x4000)) {
                            Player* d = u->player;
                            if (d->active != 0 &&
                                (d->type == 1 || d->type == 2)) {
                                PlayersPublishKillUnit(u, 0);
                            } else {
                                PlayersPublishKillUnit(u, 1);
                            }
                        }
                    }
                }
            }
        }
    }
}

// FUNCTION: 0x488570
int __stdcall GiveUnitToPlayer(Unit* unit, Player* other, const PlayersGiveUnit* event)
{
    if (unit->player == other)
        return 0;
    if (!(unit->flags & 0x10000000))
        return 0;
    if (unit->flags & 0x4000)
        return 0;
    g_game->list_391ed->NotifyUnitCaptured(unit);

    Player* cur = unit->player;
    if (cur->active != 0 && (cur->type == 1 || cur->type == 2)) {
        if (other->active == 0)
            return 0;
        if (other->type == 3) {
            PlayersPublishSendUnit(unit, (int)(other - g_game->players));
            return 1;
        }
    }
    if (other->active == 0)
        return 0;
    if (other->type != 1 && other->type != 2)
        return 0;

    PlayersPublishCaptureUnit(unit, (int)(other - g_game->players), event);
    return 1;
}
