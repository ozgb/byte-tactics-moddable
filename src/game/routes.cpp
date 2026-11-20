#include "../../interfaces/services.h"
#include "../../interfaces/economy.h"
#include "../../interfaces/wind_meteors.h"
#include "../seams/economy_world.h"
#include "../seams/wind_meteors_world.h"
#include "../units/economy_effects.h"
#include "../../interfaces/features.h"
#include "../seams/features_world.h"
#include "../units/features_effects.h"
#include "../../interfaces/targeting.h"
#include "../seams/targeting_world.h"
#include "../units/targeting_effects.h"
#include "../../interfaces/los_radar.h"
#include "../seams/los_radar_world.h"
#include "../../interfaces/map.h"
#include "../seams/map_world.h"
#include "../../interfaces/seams.h"
#include "../../interfaces/pathfinding.h"
#include "../../interfaces/players.h"
#include "../seams/players_world.h"
#include "../units/players_effects.h"
#include "../../interfaces/ai.h"
#include "../seams/ai_world.h"
#include "../orders/ai_effects.h"
#include "../units/ai_effects.h"
#include "../../interfaces/weapons.h"
#include "../orders/weapons_effects.h"
#include "../units/weapons_effects.h"
#include "../scripts/weapons_effects.h"
#include "../seams/weapons_world.h"
#include "players.h"
#include "scheduler.h"

static void PathfindingOnMapCellsChanged(const void* event)
{
    const MapCellsChanged* e = static_cast<const MapCellsChanged*>(event);
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
    {SCHED_UNITS, PLAYERS_KILL_UNITS, SCHED_PLAYERS, PlayersInbox, 1},
    {SCHED_ENGINE, PLAYERS_KILL_UNITS, SCHED_PLAYERS, PlayersInbox, 1},
    {SCHED_UNITS, PLAYERS_UNIT_CREATED, SCHED_PLAYERS, PlayersInbox, 0},
    {SCHED_UNITS, PLAYERS_UNIT_DIED, SCHED_PLAYERS, PlayersInbox, 1},
    {SCHED_ORDERS, PLAYERS_GIVE_UNIT, SCHED_PLAYERS, PlayersInbox, 1},
    {SCHED_ENGINE, PLAYERS_GIVE_UNIT, SCHED_PLAYERS, PlayersInbox, 1},
    {SCHED_ENGINE, PLAYERS_TRANSFER, SCHED_PLAYERS, PlayersInbox, 1},
    {SCHED_PLAYERS, PLAYERS_EFFECT_KILL_UNIT, SCHED_UNITS, UnitsOnPlayersKillUnit, 1},
    {SCHED_PLAYERS, PLAYERS_EFFECT_SEND_UNIT, SCHED_UNITS, UnitsOnPlayersSendUnit, 1},
    {SCHED_PLAYERS, PLAYERS_EFFECT_CAPTURE_UNIT, SCHED_UNITS, UnitsOnPlayersCaptureUnit, 1},
    {SCHED_WEAPONS, WEAPONS_EFFECT_ATTACKED, SCHED_UNITS, UnitsOnWeaponsAttacked, 1},
    {SCHED_WEAPONS, WEAPONS_EFFECT_ATTACKED, SCHED_AI, AiOnWeaponsAttacked, 1},
    {SCHED_WEAPONS, WEAPONS_EFFECT_ATTACKED, SCHED_ORDERS, OrdersOnWeaponsAttacked, 1},
    {SCHED_WEAPONS, WEAPONS_EFFECT_ATTACKED, SCHED_UNITS, UnitsOnWeaponsAttackedSpeech, 1},
    {SCHED_WEAPONS, WEAPONS_EFFECT_HIT, SCHED_ORDERS, OrdersOnWeaponsHit, 1},
    {SCHED_ORDERS, WEAPONS_DAMAGE, SCHED_WEAPONS, WeaponsInbox, 1},
    {SCHED_ORDERS, WEAPONS_RELEASE, SCHED_WEAPONS, WeaponsInbox, 1},
    {SCHED_ORDERS, WEAPONS_CLAIM, SCHED_WEAPONS, WeaponsInbox, 1},
    {SCHED_ORDERS, WEAPONS_ADD_STOCKPILE, SCHED_WEAPONS, WeaponsInbox, 1},
    {SCHED_UNITS, WEAPONS_DAMAGE, SCHED_WEAPONS, WeaponsInbox, 1},
    {SCHED_UNITS, WEAPONS_DETONATE, SCHED_WEAPONS, WeaponsInbox, 1},
    {SCHED_UNITS, WEAPONS_RELEASE, SCHED_WEAPONS, WeaponsInbox, 1},
    {SCHED_UNITS, WEAPONS_INIT_SLOTS, SCHED_WEAPONS, WeaponsInbox, 1},
    {SCHED_UNITS, WEAPONS_REMOVE_PROJECTILES, SCHED_WEAPONS, WeaponsInbox, 1},
    {SCHED_UNITS, WEAPONS_SET_STOCKPILES, SCHED_WEAPONS, WeaponsInbox, 1},
    {SCHED_ENGINE, WEAPONS_REMOTE_DAMAGE, SCHED_WEAPONS, WeaponsInbox, 1},
    {SCHED_SCRIPTS, WEAPONS_AIM_DONE, SCHED_WEAPONS, WeaponsInbox, 1},
    {SCHED_SCRIPTS, WEAPONS_EXPLODE_PIECE, SCHED_WEAPONS, WeaponsInbox, 1},
    {SCHED_WEAPONS, WEAPONS_EFFECT_MARK_DAMAGED, SCHED_UNITS, UnitsOnWeaponsMarkDamaged, 1},
    {SCHED_WEAPONS, WEAPONS_EFFECT_DAMAGE_RECORD, SCHED_UNITS, UnitsOnWeaponsDamageRecord, 1},
    {SCHED_WEAPONS, WEAPONS_EFFECT_HEALTH, SCHED_UNITS, UnitsOnWeaponsHealth, 1},
    {SCHED_WEAPONS, WEAPONS_EFFECT_PARALYZE, SCHED_ORDERS, OrdersOnWeaponsParalyze, 1},
    {SCHED_WEAPONS, WEAPONS_EFFECT_START_SCRIPT, SCHED_SCRIPTS, ScriptsOnWeaponsStartScript, 1},
    {SCHED_WEAPONS, WEAPONS_EFFECT_DETACH_PIECE, SCHED_SCRIPTS, ScriptsOnWeaponsDetachPiece, 1},
    {SCHED_WEAPONS, WEAPONS_EFFECT_SHOT_FIRED, SCHED_UNITS, UnitsOnWeaponsNotice, 1},
    {SCHED_WEAPONS, WEAPONS_EFFECT_COMMAND_SHOT_FIRED, SCHED_UNITS, UnitsOnWeaponsNotice, 1},
    {SCHED_WEAPONS, WEAPONS_EFFECT_OUT_OF_REACH, SCHED_UNITS, UnitsOnWeaponsNotice, 1},
    {SCHED_WEAPONS, WEAPONS_EFFECT_BUSY, SCHED_UNITS, UnitsOnWeaponsBusy, 1},
    {SCHED_UNITS, AI_SET_SQUAD, SCHED_AI, AiInbox, 1},
    {SCHED_ENGINE, AI_SET_SQUAD, SCHED_AI, AiInbox, 0},
    {SCHED_ENGINE, AI_PLAN, SCHED_AI, AiInbox, 0},
    {SCHED_ENGINE, AI_WEIGHT, SCHED_AI, AiInbox, 0},
    {SCHED_ENGINE, AI_LIMIT, SCHED_AI, AiInbox, 0},
    {SCHED_AI, AI_EFFECT_ADD_ORDER, SCHED_ORDERS, OrdersOnAiAddOrder, 1},
    {SCHED_AI, AI_EFFECT_QUEUE_BUILD, SCHED_ORDERS, OrdersOnAiQueueBuild, 1},
    {SCHED_AI, AI_EFFECT_STOP, SCHED_ORDERS, OrdersOnAiStop, 1},
    {SCHED_AI, AI_EFFECT_ACTIVATE, SCHED_UNITS, UnitsOnAiActivate, 1},
    {SCHED_AI, AI_EFFECT_SET_STATES, SCHED_UNITS, UnitsOnAiSetStates, 1},
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
    {SCHED_UNITS, TARGETING_LOAD, SCHED_TARGETING, TargetingInbox, 0},
    {SCHED_WEAPONS, TARGETING_CLEAR, SCHED_TARGETING, TargetingInbox, 1},
    {SCHED_TARGETING, TARGETING_EFFECT_CLEARED, SCHED_UNITS, UnitsOnTargetingCleared, 1},
    {SCHED_UNITS, LOS_RADAR_UPDATE_UNIT, SCHED_LOS_RADAR, LosRadarInbox, 1},
    {SCHED_ORDERS, LOS_RADAR_UPDATE_UNIT, SCHED_LOS_RADAR, LosRadarInbox, 1},
    {SCHED_PLAYERS, LOS_RADAR_UPDATE_UNIT, SCHED_LOS_RADAR, LosRadarInbox, 1},
    {SCHED_UNITS, LOS_RADAR_REMOVE_UNIT, SCHED_LOS_RADAR, LosRadarInbox, 1},
    {SCHED_UNITS, LOS_RADAR_REVEAL_UNIT, SCHED_LOS_RADAR, LosRadarInbox, 1},
    {SCHED_UNITS, LOS_RADAR_ADD_EYEBALL, SCHED_LOS_RADAR, LosRadarInbox, 1},
    {SCHED_UNITS, LOS_RADAR_SIGHT_RESET, SCHED_LOS_RADAR, LosRadarInbox, 1},
    {SCHED_UNITS, LOS_RADAR_SIGHT_LOAD, SCHED_LOS_RADAR, LosRadarInbox, 1},
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
    {SCHED_UNITS, MAP_RECLAIM_FOOTPRINT, SCHED_MAP, MapInbox, 1},
    {SCHED_UNITS, MAP_SET_YARD, SCHED_MAP, MapInbox, 1},
    {SCHED_UNITS, MAP_BUCKET_UNLINK, SCHED_MAP, MapInbox, 1},
    {SCHED_UNITS, MAP_BUCKET_PREPEND, SCHED_MAP, MapInbox, 1},
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
