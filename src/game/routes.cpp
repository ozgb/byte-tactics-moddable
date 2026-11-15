#include "../../interfaces/services.h"
#include "../../interfaces/economy.h"
#include "../../interfaces/wind_meteors.h"
#include "../seams/economy_world.h"
#include "../seams/wind_meteors_world.h"
#include "../units/economy_effects.h"
#include "../../interfaces/features.h"
#include "../seams/features_world.h"
#include "../units/features_effects.h"
#include "../map/map_features_effects.h"
#include "../weapons/features_effects.h"
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
    {SCHED_UNITS, FEATURES_PLACE, SCHED_FEATURES, FeaturesInbox, 1},
    {SCHED_ORDERS, FEATURES_RECLAIM, SCHED_FEATURES, FeaturesInbox, 1},
    {SCHED_ORDERS, FEATURES_REMOVE, SCHED_FEATURES, FeaturesInbox, 1},
    {SCHED_WEAPONS, FEATURES_DAMAGE, SCHED_FEATURES, FeaturesInbox, 1},
    {SCHED_ENGINE, FEATURES_PLACE, SCHED_FEATURES, FeaturesInbox, 0},
    {SCHED_FEATURES, FEATURES_EFFECT_FREE_STATE, SCHED_UNITS, UnitsOnFeaturesFreeState, 1},
    {SCHED_FEATURES, FEATURES_EFFECT_INIT_STATE, SCHED_UNITS, UnitsOnFeaturesInitState, 1},
    {SCHED_FEATURES, FEATURES_EFFECT_PASS_MAPS, SCHED_MAP, MapOnFeaturesPassMaps, 1},
    {SCHED_FEATURES, FEATURES_EFFECT_AREA_DAMAGE, SCHED_WEAPONS, WeaponsOnFeaturesAreaDamage, 1},
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
