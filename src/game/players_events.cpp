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

#include "game.h"
#include "los_helpers.h"
#pragma pack(pop)

#include "../../link/globals.h"
#include "players_events.h"
#include "../network/net_game.h"
#include "economy_events.h"
#include "../units/units.h"

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

int __stdcall PlayersApply(const PlayersEvent* e)
{
    Ref<Unit> ref;
    ref.offset = e->unit;
    Unit* unit = ref.get();
    switch (e->kind) {
    case PLAYERS_GIVE_UNIT:
        if (unit == nullptr || (unsigned)e->player > 9)
            return 0;
        return GiveUnitToPlayer(unit, &g_game->players[e->player], e);
    case PLAYERS_KILL_UNITS:
        if ((unsigned)e->player > 10)
            return 0;
        KillPlayerUnits((unsigned char)e->player);
        return 1;
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
    case PLAYERS_TRANSFER:
        if ((unsigned)e->from > 10 || (unsigned)e->player > 10)
            return 0;
        if (e->value == 0)
            return TransferMetal((unsigned char)e->from, (unsigned char)e->player, e->amount, e->announce);
        if (e->value == 1)
            return TransferEnergy((unsigned char)e->from, (unsigned char)e->player, e->amount, e->announce);
        return 0;
    }
    return 0;
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
int __stdcall GiveUnitToPlayer(Unit* unit, Player* other, const PlayersEvent* event)
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
