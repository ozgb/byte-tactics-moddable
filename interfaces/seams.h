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

// Sets the search steps the default pathfinder spends per tick, for the search console command.
void SetSearchStepsPerTick(int steps);
// Sets the base weight of the default pathfinder's heuristic in 16.16 fixed point, for the search console command.
void SetSearchBaseScale(int scale);

#endif
