#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/economy.h"
#include "../../interfaces/ref.h"
#include "../network/player.h"
#include "../network/player_info.h"
#include "../game/game.h"
#include "../map/cell.h"
#include "../units/unit_def.h"
#include "../units/unit.h"
#include "../../interfaces/services.h"
#include "../harness/replay.h"
#include "../harness/replay_services.h"
#include "../harness/harness.h"
#include "../game/scheduler.h"
#include "economy_world.h"

#include "../../link/globals.h"

static Unit* UnitOf(EconUnit unit)
{
    Ref<Unit> ref;
    ref.offset = unit;
    return ref.get();
}

static unsigned int __stdcall WorldTick()
{
    return (unsigned int)g_game->ticks;
}

static int __stdcall WorldDifficulty()
{
    return g_game->difficulty;
}

static int __stdcall WorldSharingEnabled()
{
    return (g_game->netGameFlags & 1) != 0 || HarnessSharing();
}

static float __stdcall WorldWind()
{
    return g_game->windStrength;
}

static float __stdcall WorldTide()
{
    return g_game->tidal;
}

static void __stdcall WorldPlayerInfo(int player, EconPlayerInfo* out)
{
    Player* p = &g_game->players[player];
    PlayerInfo* info = p->info.get();
    out->active = p->active;
    out->controller = p->type;
    out->index = p->index;
    out->human = info != nullptr && info->kind == 1;
    out->shareMetal = info != nullptr && info->shareMetal;
    out->shareEnergy = info != nullptr && info->shareEnergy;
    out->shareMapping = info != nullptr && info->shareMapping;
    out->inGame = p->unitCount != 0 || p->unitsCreated == 0;
    out->storage = (p->flags & 1) != 0;
}

static int __stdcall WorldAllied(int from, int to)
{
    return g_game->players[from].allied[to] != 0;
}

static EconUnit __stdcall WorldFirstUnit(int player)
{
    return g_game->players[player].unitsBegin.offset;
}

static EconUnit __stdcall WorldLastUnit(int player)
{
    return g_game->players[player].unitsEnd.offset;
}

static EconUnit __stdcall WorldNextUnit(EconUnit unit)
{
    return Ref<Unit>(UnitOf(unit) + 1).offset;
}

static int __stdcall WorldUnitSlot(EconUnit unit)
{
    return (int)(UnitOf(unit) - g_game->units.get());
}

static int __stdcall WorldUnitInfo(EconUnit unit, EconUnitInfo* out)
{
    Unit* u = UnitOf(unit);
    out->player = u->playerIndex;
    out->type = u->unitDefIndex;
    out->structure = (u->flags & 0x20000000) != 0;
    out->active = (u->activateFlags & 1) != 0;
    out->cloakOn = u->cloakOn != 0;
    out->cloakBlocked = (u->flags & 0x1000) != 0;
    out->moving = (u->flags & 0xc) != 0;
    out->workTime = (unsigned int)u->workTime;
    out->buildLeft = u->buildLeft;
    out->extraction = *(float*)&u->extraction;
    out->cellX = u->cell.x;
    out->cellY = u->cell.y;
    out->footprintX = u->footprint.x;
    out->footprintY = u->footprint.y;
    out->hasScript = u->script != nullptr;
    return (u->flags & 0x10000000) != 0;
}

static void __stdcall WorldTypeInfo(EconType type, EconTypeInfo* out)
{
    UnitDef* def = &g_game->defs.get()[type];
    out->energyMake = def->energymake;
    out->energyUse = def->energyuse;
    out->metalMake = def->metalmake;
    out->extractsMetal = def->extractsMetal;
    out->windGenerator = def->windgenerator;
    out->tidalGenerator = def->tidalgenerator;
    out->cloakCost = def->cloakcost;
    out->cloakCostMoving = def->cloakcostmoving;
    out->energyStorage = def->energystorage;
    out->metalStorage = def->metalstorage;
    out->makesMetal = def->makesMetal;
}

static int __stdcall WorldCellMetal(int x, int y)
{
    Cell* c = g_seams.los_radar->GetMapCell(x, y);
    return c != nullptr ? c->metal : -1;
}

static void __stdcall WorldEmit(const EconomyEvent* effect)
{
    HarnessEconomyEvent(effect, 1);
    SchedPublish(SCHED_ECONOMY, effect, sizeof(*effect));
}

static EconomyWorldView s_world = {
    WorldTick, WorldDifficulty, WorldSharingEnabled, WorldWind, WorldTide, WorldPlayerInfo, WorldAllied,
    WorldFirstUnit, WorldLastUnit, WorldNextUnit, WorldUnitSlot, WorldUnitInfo, WorldTypeInfo, WorldCellMetal,
    WorldEmit,
};

const EconomyWorldView* __stdcall NativeEconomyWorld()
{
    return &s_world;
}

void EconomyInbox(const void* event)
{
    const EconomyEvent* e = static_cast<const EconomyEvent*>(event);
    HarnessEconomyEvent(e, g_seams.economy->Apply(e));
}

int __stdcall ScheduleEconomySpend(const EconomyEvent* spend)
{
    ReplayImport call(RI_SCHEDULE_ECONOMY_SPEND);
    call.Ptr(spend);
    SchedDeliver(SCHED_ECONOMY);
    int outcome = g_seams.economy->Spend(spend);
    HarnessEconomyEvent(spend, outcome);
    return call.Result(outcome);
}

int __stdcall ScheduleEconomyRequestGranted(unsigned int unit, unsigned int resources)
{
    ReplayImport call(RI_SCHEDULE_ECONOMY_REQUEST_GRANTED);
    call.Int((int)unit);
    call.Int((int)resources);
    SchedDeliver(SCHED_ECONOMY);
    return call.Result(g_seams.economy->RequestGranted(unit, resources));
}

void __stdcall ScheduleMetalExtraction(unsigned int unit)
{
    ReplayImport call(RI_SCHEDULE_METAL_EXTRACTION);
    call.Int((int)unit);
    SchedDeliver(SCHED_ECONOMY);
    g_seams.economy->UpdateMetalExtraction(unit);
}
