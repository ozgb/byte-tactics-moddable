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
#include "../../interfaces/targeting.h"
#include "../seams/targeting_world.h"
#include "../units/targeting_effects.h"
#include "../../interfaces/los_radar.h"
#include "../seams/los_radar_world.h"
#include "../../interfaces/map.h"
#include "../seams/map_world.h"
#include "../../interfaces/seams.h"
#include "../../interfaces/pathfinding.h"
#include "players.h"
#include "scheduler.h"

static void PathfindingOnMapCellsChanged(const void* event)
{
    const MapEvent* e = static_cast<const MapEvent*>(event);
    g_seams.pathfinding->CellsChanged(e->rect[0], e->rect[1], e->rect[2], e->rect[3], e->classes, e->cause);
}

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
    {SCHED_ORDERS, TARGETING_SET_UNIT, SCHED_TARGETING, TargetingInbox, 1},
    {SCHED_ORDERS, TARGETING_SET_POS, SCHED_TARGETING, TargetingInbox, 1},
    {SCHED_ORDERS, TARGETING_CLEAR, SCHED_TARGETING, TargetingInbox, 1},
    {SCHED_AI, TARGETING_SET_UNIT, SCHED_TARGETING, TargetingInbox, 1},
    {SCHED_AI, TARGETING_SET_POS, SCHED_TARGETING, TargetingInbox, 0},
    {SCHED_AI, TARGETING_CLEAR, SCHED_TARGETING, TargetingInbox, 1},
    {SCHED_UNITS, TARGETING_RESET, SCHED_TARGETING, TargetingInbox, 0},
    {SCHED_WEAPONS, TARGETING_CLEAR, SCHED_TARGETING, TargetingInbox, 1},
    {SCHED_TARGETING, TARGETING_EFFECT_CLEARED, SCHED_UNITS, UnitsOnTargetingCleared, 1},
    {SCHED_UNITS, LOS_RADAR_UPDATE_UNIT, SCHED_LOS_RADAR, LosRadarInbox, 1},
    {SCHED_ORDERS, LOS_RADAR_UPDATE_UNIT, SCHED_LOS_RADAR, LosRadarInbox, 1},
    {SCHED_PLAYERS, LOS_RADAR_UPDATE_UNIT, SCHED_LOS_RADAR, LosRadarInbox, 1},
    {SCHED_UNITS, LOS_RADAR_REMOVE_UNIT, SCHED_LOS_RADAR, LosRadarInbox, 1},
    {SCHED_UNITS, LOS_RADAR_REVEAL_UNIT, SCHED_LOS_RADAR, LosRadarInbox, 1},
    {SCHED_UNITS, LOS_RADAR_ADD_EYEBALL, SCHED_LOS_RADAR, LosRadarInbox, 1},
    {SCHED_MAP, LOS_RADAR_CELL_HEIGHTS, SCHED_LOS_RADAR, LosRadarInbox, 0},
    {SCHED_PLAYERS, LOS_RADAR_RECALCULATE, SCHED_LOS_RADAR, LosRadarInbox, 1},
    {SCHED_ENGINE, LOS_RADAR_RECALCULATE, SCHED_LOS_RADAR, LosRadarInbox, 0},
    {SCHED_ENGINE, LOS_RADAR_SHARE_MAPPING, SCHED_LOS_RADAR, LosRadarInbox, 0},
    {SCHED_ENGINE, LOS_RADAR_SHARE_RADAR, SCHED_LOS_RADAR, LosRadarInbox, 0},
    {SCHED_ECONOMY, LOS_RADAR_SHARE_MAPPING, SCHED_LOS_RADAR, LosRadarInbox, 0},
    {SCHED_ECONOMY, LOS_RADAR_SHARE_RADAR, SCHED_LOS_RADAR, LosRadarInbox, 0},
    {SCHED_ENGINE, LOS_RADAR_SHARE_MAP, SCHED_LOS_RADAR, LosRadarInbox, 1},
    {SCHED_UNITS, MAP_ADD_UNIT, SCHED_MAP, MapInbox, 1},
    {SCHED_ORDERS, MAP_ADD_UNIT, SCHED_MAP, MapInbox, 1},
    {SCHED_UNITS, MAP_REMOVE_UNIT, SCHED_MAP, MapInbox, 1},
    {SCHED_ORDERS, MAP_REMOVE_UNIT, SCHED_MAP, MapInbox, 1},
    {SCHED_UNITS, MAP_CLEAR_UNIT, SCHED_MAP, MapInbox, 1},
    {SCHED_UNITS, MAP_RECLAIM_FOOTPRINT, SCHED_MAP, MapInbox, 0},
    {SCHED_UNITS, MAP_SET_YARD, SCHED_MAP, MapInbox, 1},
    {SCHED_UNITS, MAP_BUCKET_UNLINK, SCHED_MAP, MapInbox, 1},
    {SCHED_UNITS, MAP_BUCKET_PREPEND, SCHED_MAP, MapInbox, 1},
    {SCHED_UNITS, MAP_REFRESH_PASS_MAP, SCHED_MAP, MapInbox, 1},
    {SCHED_MAP, MAP_EFFECT_CELLS_CHANGED, SCHED_PATHFINDING, PathfindingOnMapCellsChanged, 1},
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
