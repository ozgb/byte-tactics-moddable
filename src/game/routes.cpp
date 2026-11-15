#include "../../interfaces/services.h"
#include "../../interfaces/economy.h"
#include "../../interfaces/wind_meteors.h"
#include "../seams/economy_world.h"
#include "../seams/wind_meteors_world.h"
#include "../units/economy_effects.h"
#include "players.h"
#include "scheduler.h"

static const SchedRoute s_routes[] = {
    {SCHED_UNITS, ECONOMY_RESET, SCHED_ECONOMY, EconomyInbox, 0},
    {SCHED_UNITS, ECONOMY_LOAD, SCHED_ECONOMY, EconomyInbox, 0},
    {SCHED_UNITS, ECONOMY_REQUEST, SCHED_ECONOMY, EconomyInbox, 0},
    {SCHED_UNITS, ECONOMY_INCOME, SCHED_ECONOMY, EconomyInbox, 0},
    {SCHED_ORDERS, ECONOMY_REQUEST, SCHED_ECONOMY, EconomyInbox, 0},
    {SCHED_ORDERS, ECONOMY_INCOME, SCHED_ECONOMY, EconomyInbox, 0},
    {SCHED_PLAYERS, ECONOMY_RESET, SCHED_ECONOMY, EconomyInbox, 0},
    {SCHED_PLAYERS, ECONOMY_INCOME, SCHED_ECONOMY, EconomyInbox, 0},
    {SCHED_FEATURES, ECONOMY_INCOME, SCHED_ECONOMY, EconomyInbox, 0},
    {SCHED_ECONOMY, ECONOMY_EFFECT_CLOAK, SCHED_UNITS, UnitsOnEconomyCloak, 1},
    {SCHED_ECONOMY, ECONOMY_EFFECT_EXTRACTOR, SCHED_UNITS, UnitsOnEconomyExtractor, 0},
    {SCHED_ECONOMY, ECONOMY_EFFECT_SHARE, SCHED_PLAYERS, PlayersOnEconomyShare, 0},
    {SCHED_ENGINE, WIND_METEORS_ENABLE, SCHED_WIND_METEORS, WindMeteorsInbox, 0},
    {SCHED_ENGINE, WIND_METEORS_DISABLE, SCHED_WIND_METEORS, WindMeteorsInbox, 0},
    {SCHED_ENGINE, WIND_METEORS_PARAMS, SCHED_WIND_METEORS, WindMeteorsInbox, 0},
    {SCHED_ENGINE, WIND_METEORS_START, SCHED_WIND_METEORS, WindMeteorsInbox, 0},
    {SCHED_WIND_METEORS, WIND_METEORS_EFFECT_SPAWN, SCHED_WEAPONS, WeaponsOnMeteorSpawn, 0},
};

const SchedRoute* SchedRoutes(int* count)
{
    *count = sizeof(s_routes) / sizeof(s_routes[0]);
    return s_routes;
}
