#ifndef INTERFACES_MAP_H
#define INTERFACES_MAP_H

#include "state_writer.h"

struct Vec3;
struct Point16;
struct Unit;
class UnitDef;
struct Player;
class HapiBank;

// Major version of the map interface, changed when a slot is removed or its signature changes.
#define MAP_VERSION_MAJOR 7
// Minor version of the map interface, changed when a slot is added.
#define MAP_VERSION_MINOR 0

// A movement class's footprint and limits, as units reads them from the game data; map copies them when it builds the pass maps.
struct MapMoveLimits {
    // The class's index, 0 to 31.
    int index;
    // The footprint's width, in cells.
    short footprintX;
    // The footprint's height, in cells.
    short footprintZ;
    // The greatest depth of water the class enters, below sea level.
    short maxWaterDepth;
    // The least depth of water the class needs, below sea level (negative when it can stand above it).
    short minWaterDepth;
    // The steepest slope on land the class crosses (a cell whose lowest corner is at or above sea level).
    unsigned char maxSlope;
    // The slope on land from which a cell is passable only at a cost.
    unsigned char badSlope;
    // The steepest slope under water the class crosses (a cell whose lowest corner is below sea level).
    unsigned char maxWaterSlope;
    // The slope under water from which a cell is passable only at a cost.
    unsigned char badWaterSlope;
};

// The fields every map event and effect starts with.
struct MapEventHeader {
    // A MapEventKind or a MapEffectKind.
    int kind;
    // The unit, as its block offset (an offset from the block base, 0 for none: PASS_MAPS and CELLS_CHANGED name none), as economy's events give a unit.
    unsigned int unit;
};

// The kinds of event other modules publish to map's inbox; each is an intent, validated when Apply consumes it, and applied in the order it was published. An event smaller than its payload, one that names a null unit, a bucket change the unit's bucket does not allow, or a rectangle of negative size changes nothing; outcome 0.
enum MapEventKind {
    // Puts unit on the cells its footprint covers at its cell and in the spatial grid bucket of its position, or in the bucket of units off the map; the header alone; outcome 1.
    MAP_ADD_UNIT = 160,
    // Takes unit off the cells its footprint covers, and lets the units it overlapped claim them again; the header alone; outcome 1.
    MAP_REMOVE_UNIT = 161,
    // Takes unit off the cells, as MAP_REMOVE_UNIT does, and out of the spatial grid, as it dies; the header alone; outcome 1.
    MAP_CLEAR_UNIT = 162,
    // Makes the units whose footprints overlap unit's give up the cells unit covers; the header alone; outcome 1.
    MAP_RECLAIM_FOOTPRINT = 163,
    // Opens or closes the yard of unit when its footprint is clear for that state, and claims its cells for the state (MapSetYard); outcome 1 when it changed, 0 when the footprint was not clear.
    MAP_SET_YARD = 164,
    // Takes unit out of the spatial grid bucket it is in, as it boards a carrier; the header alone; outcome 1, or 0 when unit has no bucket or is not in its bucket's chain.
    MAP_BUCKET_UNLINK = 165,
    // Puts unit at the front of its spatial grid bucket, as it leaves a carrier; the header alone; outcome 1, or 0 when unit has no bucket or is already in its bucket's chain.
    MAP_BUCKET_PREPEND = 166,
    // Computes again the pass maps of every movement class over a rectangle, after the terrain or the features on it changed (MapPassMaps); outcome 1, or 0 when either size is negative.
    MAP_PASS_MAPS = 168
};

// The kinds of effect map publishes to its outbox, for the modules that subscribe to them.
enum MapEffectKind {
    // The pass maps of some movement classes changed over a rectangle (MapCellsChanged).
    MAP_EFFECT_CELLS_CHANGED = 176
};

// The payload of SET_YARD.
struct MapSetYard {
    // The kind and the unit.
    MapEventHeader h;
    // 1 to open the yard, 0 to close it.
    int open;
};

// The payload of PASS_MAPS.
struct MapPassMaps {
    // The kind; the unit is 0.
    MapEventHeader h;
    // The first cell of the rectangle, x and y.
    int cell[2];
    // The width and height of the rectangle, in cells.
    int size[2];
};

// The payload of CELLS_CHANGED.
struct MapCellsChanged {
    // The kind; the unit is 0.
    MapEventHeader h;
    // The rectangle, in cells: left, top, right and bottom.
    int rect[4];
    // The movement classes whose pass maps changed, a bit for each class index.
    unsigned int classes;
    // Why they changed, a PathChange.
    int cause;
};

// Terrain and map data: the cells units stand on and their heights, the spatial grid of units, the pass maps and when they are current, and the placement tests. The engine delivers map's pending inbox, through Apply, before each synchronous slot below, and at each phase boundary.
struct MapSeam {
    // Consumes one queued event of the inbox, its payload of size bytes, and returns its outcome, which the engine records in the trace's dump and returns to no one.
    int (__stdcall* Apply)(const MapEventHeader* event, unsigned int size);
    // Writes to ids the unit ids of the units within range of pos, in the order the spatial grid holds them, skipping the first skip of them, and returns how many it wrote, at most capacity; a query. Hooks: none.
    int (__stdcall* UnitsInRange)(const Vec3* pos, int range, int skip, unsigned short* ids, int capacity);
    // Returns the mean ground height of the cell under a position; a query. Hooks: none.
    int (__stdcall* GetCellMeanHeight)(Vec3* pos);
    // Synchronous: returns 1 when a unit type can be built at a cell, with a visibility check when player is given, and leaves the metal of the cells it read and the height of a fit in the placement record, which the caller reads in the same statement. Hooks: none.
    int (__stdcall* CanBuildAt)(UnitDef* unit, Point16 cell, short type, Player* player);
    // Synchronous: returns 1 when the footprint of a unit type fits at a cell (flags 2 counts a cell off the map as a fit); for a type that does not move it answers and fills the placement record as CanBuildAt does. Hooks: none.
    int (__stdcall* CanPlaceUnitFootprint)(UnitDef* unit, unsigned short otherTypeId, Point16 cell, int flags);
    // Copies the footprint and limits of count movement classes and builds their pass maps for the loaded terrain, outside the tick; the classes not given have none. Hooks: none.
    void (__stdcall* BuildAllPassMaps)(const MapMoveLimits* limits, int count);
    // Frees the pass maps when a match shuts down, outside the tick. Hooks: none.
    void (__stdcall* FreePassMaps)(void);
    // Reads the explored-map state of a saved game from a bank. Hooks: none.
    void (__stdcall* LoadMappingData)(HapiBank* file);
    // Reads the metal plot map of a saved game from a bank. Hooks: none.
    void (__stdcall* LoadMetalPlotmap)(HapiBank* file);
    // Reads the player feature plot map of a saved game from a bank. Hooks: none.
    void (__stdcall* LoadPlayerFeaturesPlotmap)(HapiBank* file);
    // Writes the explored-map state of a saved game to a bank. Hooks: none.
    void (__stdcall* SaveMappingData)(HapiBank* file);
    // Writes the metal plot map of a saved game to a bank. Hooks: none.
    void (__stdcall* SaveMetalPlotmap)(HapiBank* file);
    // Writes the player feature plot map of a saved game to a bank. Hooks: none.
    void (__stdcall* SavePlayerFeaturesPlotmap)(HapiBank* file);
    // Moves a position to where the footprint of a unit type sits on the grid; a query that writes only the position it is given. Hooks: none.
    void (__stdcall* SnapWorldPosToFootprint)(UnitDef* unit, Vec3* pos);
    // Returns 1 when the footprint of a unit fits at a position; a query. Hooks: none.
    int (__stdcall* CanPlaceFootprintAt)(Unit* unit, Vec3* pos);
    // Returns 1 when a slot of a landing pad is free; a query. Hooks: none.
    int (__stdcall* IsPadSlotFree)(Unit* pad, int id);
    // Synchronous: picks a random energy and a random metal feature to reclaim within a radius, drawing from the game logic stream, writes their positions and amounts, and returns 1 when it found either. Hooks: none.
    int (__stdcall* PickRandomReclaimableResourcesInRadius)(Vec3* pos, int range, Vec3** energy, float* energyAmount, Vec3** metal, float* metalAmount);
    // Allocates the spatial grid buckets, and the bucket of units off the map, for the terrain the map holds, and fills their heights and edge flags; called when the terrain's derived layers are built, outside the tick. Hooks: none.
    void (__stdcall* AllocateBuckets)(void);
    // Frees the spatial grid buckets and the bucket of units off the map when a match shuts down, outside the tick. Hooks: none.
    void (__stdcall* FreeBuckets)(void);
    // Returns the height of the ground at a position, between the heights of the four cells around it, or -1 off the map; a query. Hooks: none.
    int (__stdcall* GetGroundHeight)(Vec3* pos);
    // Returns the 2-bit passability of a cell in the pass map of the movement class of an index; a query. Hooks: none.
    int (__stdcall* PassMapGet)(int moveClass, int x, int y);
    // Returns 1 when a cell is inside the pass map of the movement class of an index; a query. Hooks: none.
    int (__stdcall* PassMapInBounds)(int moveClass, unsigned int x, unsigned int y);
    // Synchronous, before a path search for a unit: moves the window of stale stamps of the movement class of an index on to the 30 ticks before now, computes again the class's pass map over the cells of unit when its stamp is older than the window was, and over the cells of each moving unit whose stamp fell in the ticks the window left, publishing CELLS_CHANGED for each. Hooks: none.
    void (__stdcall* RefreshMovedUnits)(int moveClass, Unit* unit);
    // Synchronous, after a path search for a unit: computes again the pass map of the movement class of an index over the cells of unit when its stamp is older than the class's window, publishing CELLS_CHANGED. Hooks: none.
    void (__stdcall* RefreshUnitIfStale)(int moveClass, Unit* unit);
    // Writes the state that decides what the implementation does next and that the engine does not already hash elsewhere; it reads only, and keeps nothing.
    void (__stdcall* WriteState)(const StateWriter* writer);
};

#endif
