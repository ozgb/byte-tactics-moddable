#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/services.h"
#include "../../interfaces/ref.h"
#include "../network/player.h"
#include "../units/unit_def.h"
#include "../units/unit.h"
#include "game.h"
#include "scheduler.h"

#include "../../link/globals.h"

#if BT_ACCESS_LOG
#include "../../lib/accesslog/access_log.h"
#endif

static Unit* UnitOfHandle(EconUnit handle)
{
    Ref<Unit> ref;
    ref.offset = handle;
    return ref.get();
}

int __stdcall ScheduleEconomyEvent(const EconomyEvent* event)
{
    int outcome = g_seams.economy->Apply(event);
    HarnessEconomyEvent(event, outcome);
    return outcome;
}

void SchedEconomyEffect(const EconomyEvent* effect)
{
    HarnessEconomyEvent(effect, 1);
    switch (effect->kind) {
    case ECONOMY_EFFECT_CLOAK:
        g_seams.units->SetStateBits(UnitOfHandle(effect->unit), 4, effect->value);
        break;
    case ECONOMY_EFFECT_EXTRACTOR: {
        Unit* unit = UnitOfHandle(effect->unit);
        if (unit->script)
            g_seams.units->StartScriptWithArgs((CobScript*)unit->script.get(), "SetSpeed", 0, 0, 1, effect->value, 0, 0, 0);
        break;
    }
    case ECONOMY_EFFECT_SHARE:
        if (effect->resources & ECON_METAL)
            g_seams.players->TransferEnergy((unsigned char)effect->player, (unsigned char)effect->value, effect->metal, 1);
        else
            g_seams.players->TransferMetal((unsigned char)effect->player, (unsigned char)effect->value, effect->energy, 1);
        break;
    }
}

void __stdcall ScheduleMetalExtraction(unsigned int unit)
{
    g_seams.economy->UpdateMetalExtraction(unit);
}

void SchedUnitsPhase()
{
#if BT_ACCESS_LOG
    AccessLogPassScope accessPass(ACCESS_PASS_UNITS_PLAYERS);
#endif
    g_game->f14353 = 0;
    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (p->active == 0)
            continue;
        unsigned char k = p->type;
        if (k != 1 && k != 2 && k != 3)
            continue;
        if (p->index == 0xa)
            continue;
#if BT_ACCESS_LOG
        AccessLogEntity();
#endif
        {
            Unit* last = p->unitsEnd;
            Unit* u = p->unitsBegin;
#if BT_ACCESS_LOG
            {
                AccessLogPassScope unitsPass(ACCESS_PASS_UNITS);
#endif
                while (u <= last) {
                    if (u->unitDefIndex != 0) {
#if BT_ACCESS_LOG
                        AccessLogEntity();
#endif
                        g_seams.units->UpdateUnit(u);
                    }
                    u = (Unit*)((char*)u + 0x118);
                }
#if BT_ACCESS_LOG
            }
#endif
            if (g_game->netGameFlags & 1) {
                if (p->active != 0) {
                    unsigned char k4 = p->type;
                    if (k4 == 1 || k4 == 2) {
                        p->syncTick = g_game->ticks;
                        g_seams.network->SendUnitStates(p);
                    }
                }
            }
        }
    }
}

void SchedPlayersPhase(RandStream stream)
{
#if BT_ACCESS_LOG
    AccessLogPassScope accessPass(ACCESS_PASS_PLAYERS);
#endif
    g_seams.pathfinding->Step();
    PathEvent event;
    PathPoint waypoints[64];
    while (g_seams.pathfinding->Poll(&event, waypoints, 64)) {
        HarnessPathEvent(&event, waypoints, 64);
        g_seams.orders->ApplyPathEvent(&event, waypoints);
    }
    for (int player = 0; player < 10; player++) {
        int step = g_seams.players->UpdatePlayerHead(player, stream);
        if (step == 0)
            continue;
        if (step == 2)
            g_seams.economy->UpdatePlayerEconomy(player);
        g_seams.players->UpdatePlayerTail(player);
    }
    g_seams.players->UpdatePlayersFinish();
}
