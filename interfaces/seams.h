#ifndef INTERFACES_SEAMS_H
#define INTERFACES_SEAMS_H

#include "wind_meteors.h"
#include "economy.h"
#include "features.h"
#include "ai.h"
#include "pathfinding.h"
#include "weapons.h"
#include "targeting.h"
#include "los_radar.h"
#include "orders.h"
#include "units.h"
#include "players.h"
#include "map.h"
#include "files.h"
#include "network.h"

// The function table each simulation subsystem is called through, and the world view pathfinding reads.
struct Seams {
    WindMeteorsSeam* wind_meteors;
    EconomySeam* economy;
    FeaturesSeam* features;
    AiSeam* ai;
    PathfindingSeam* pathfinding;
    WeaponsSeam* weapons;
    TargetingSeam* targeting;
    LosRadarSeam* los_radar;
    OrdersSeam* orders;
    UnitsSeam* units;
    PlayersSeam* players;
    MapSeam* map;
    FilesSeam* files;
    NetworkSeam* network;
    PathWorldView* pathworld;
};

// The active table, set once at start and never changed during a match.
extern Seams g_seams;

// Points g_seams at the built-in 3.1c implementations.
void InstallNativeSeams();

// 3.1c's pathfinding, whichever implementation g_seams.pathfinding holds.
const PathfindingSeam* DefaultPathfindingSeam();

// 3.1c's economy, whichever implementation g_seams.economy holds.
const EconomySeam* DefaultEconomySeam();

#endif
