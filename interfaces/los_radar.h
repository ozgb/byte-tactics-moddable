#ifndef INTERFACES_LOS_RADAR_H
#define INTERFACES_LOS_RADAR_H

#include "state_writer.h"

namespace std {
template<class T> class allocator;
template<class T, class A> class vector;
}

template <class T> struct Ref;
struct Cell;
struct Player;
struct Unit;
struct Vec3;
class HapiBank;

// Major version of the los_radar interface, changed when a slot is removed or its signature changes.
#define LOS_RADAR_VERSION_MAJOR 4
// Minor version of the los_radar interface, changed when a slot is added.
#define LOS_RADAR_VERSION_MINOR 1

// The field every los_radar event starts with.
struct LosRadarEventHeader {
    // A LosRadarEventKind.
    int kind;
};

// The kinds of event other modules and the engine publish to los_radar's inbox; each is an intent, validated when Apply consumes it, and applied in the order it was published. An event smaller than its payload, or one that names a unit with a null unit, changes nothing; outcome 0.
enum LosRadarEventKind {
    // Moves the line of sight of a unit to where it now is (LosRadarUnit); outcome 1.
    LOS_RADAR_UPDATE_UNIT = 128,
    // Takes the line of sight of a unit off the map, as it dies (LosRadarUnit); outcome 1.
    LOS_RADAR_REMOVE_UNIT = 129,
    // Gives a new unit its first line of sight (LosRadarUnit); outcome 1.
    LOS_RADAR_REVEAL_UNIT = 130,
    // Adds an eyeball, a temporary view of the viewing player (LosRadarAddEyeball); outcome 1 when it was added, 0 when line of sight is off or no eyeball is free.
    LOS_RADAR_ADD_EYEBALL = 131,
    // Recomputes the terrain height range of the cells of a rectangle (LosRadarCellHeights); outcome 1.
    LOS_RADAR_CELL_HEIGHTS = 132,
    // Rebuilds the line of sight of every unit and the radar picture (LosRadarRecalculate); outcome 1.
    LOS_RADAR_RECALCULATE = 133,
    // Toggles whether a player shares its map with its allies and tells the other players, in a network game (LosRadarSharePlayer); outcome 1 when it toggled, 0 outside a network game.
    LOS_RADAR_SHARE_MAPPING = 134,
    // Toggles whether a player shares its radar with its allies and tells the other players, in a network game (LosRadarSharePlayer); outcome 1 when it toggled, 0 outside a network game.
    LOS_RADAR_SHARE_RADAR = 135,
    // Shows a player every cell that another player sees now (LosRadarShareMap); outcome 1.
    LOS_RADAR_SHARE_MAP = 136,
    // Sets the sight cache of a unit back to what units' set-up wrote, no cell and no frame (LosRadarUnit); outcome 1.
    LOS_RADAR_SIGHT_RESET = 137,
    // Sets the sight cache of a unit to the one a saved game holds (LosRadarSightLoad); outcome 1.
    LOS_RADAR_SIGHT_LOAD = 138
};

// What los_radar remembers of the line of sight it gave a unit, in the order a saved game stores it.
struct LosSightCache {
    // The cell the sight circle was last drawn at, the x of its top left in the low 16 bits and the z in the high 16 bits.
    int cell;
    // The frame of the sight circle table last drawn.
    unsigned char frameIdx;
};

// The map a match starts on.
struct LosRadarMapInfo {
    // The number of unit slots.
    int unitSlots;
};

// The payload of UPDATE_UNIT, REMOVE_UNIT, REVEAL_UNIT and SIGHT_RESET: the unit they are about.
struct LosRadarUnit {
    // The kind.
    LosRadarEventHeader h;
    // The unit, as its block offset (an offset from the block base, 0 for none), as economy's events give a unit.
    unsigned int unit;
};

// The payload of ADD_EYEBALL.
struct LosRadarAddEyeball {
    // The kind.
    LosRadarEventHeader h;
    // The eyeball's position, in world coordinates with 16 fractional bits.
    int pos[3];
    // The eyeball's sight distance, in world units.
    int sightDistance;
    // The height of the eyeball's eye above its position, in world units.
    int eyeHeight;
    // The eyeball's lifetime, in ticks.
    int lifetime;
};

// The payload of CELL_HEIGHTS.
struct LosRadarCellHeights {
    // The kind.
    LosRadarEventHeader h;
    // The first cell of the rectangle, x and y.
    int cell[2];
    // The width and height of the rectangle, in cells.
    int size[2];
};

// The payload of RECALCULATE.
struct LosRadarRecalculate {
    // The kind.
    LosRadarEventHeader h;
    // 1 to first reset what each cell shows: to nothing for every player when mapping is on, to everything when it is off.
    int reset;
};

// The payload of SHARE_MAPPING and SHARE_RADAR: the player whose sharing toggles.
struct LosRadarSharePlayer {
    // The kind.
    LosRadarEventHeader h;
    // The player, 0 to 9.
    int player;
};

// The payload of SHARE_MAP.
struct LosRadarShareMap {
    // The kind.
    LosRadarEventHeader h;
    // The player whose sight is shown, 0 to 9.
    int from;
    // The player it is shown to, 0 to 9.
    int to;
};

// The payload of SIGHT_LOAD.
struct LosRadarSightLoad {
    // The kind.
    LosRadarEventHeader h;
    // The unit, as its block offset.
    unsigned int unit;
    // The cell of the sight cache, as LosSightCache::cell.
    int cell;
    // The frame of the sight cache, as LosSightCache::frameIdx (0 to 255).
    int frameIdx;
};

// Line of sight and radar. The engine delivers los_radar's pending inbox, through Apply, before each pass below and at each phase boundary.
struct LosRadarSeam {
    // Consumes one queued event of the inbox, its payload of size bytes, and returns its outcome, which the engine records in the trace's dump and returns to no one.
    int (__stdcall* Apply)(const LosRadarEventHeader* event, unsigned int size);
    // Returns the map cell at a position, or null outside the map; the cell records are read only, a view as of each owner's last run. Hooks: none.
    Cell* (__stdcall* GetMapCell)(int x, int y);
    // Returns the map cell under a world position, or null outside the map; a view, as GetMapCell. Hooks: none.
    Cell* (__stdcall* GetMapCellAtPosition)(Vec3* pos);
    // Returns the cell a footprint starting under a world position begins on; a view, as GetMapCell. Hooks: none.
    Cell* (__stdcall* GetOriginCellAtPosition)(Vec3* pos);
    // Writes the map position of a world position, kept inside the terrain, to out; a query. Hooks: none.
    void (__stdcall* ClampWorldPosToTerrain)(int x, int y, Vec3* out);
    // Returns 1 when a unit is visible to a player's side, by sight, radar or sonar; a query. Hooks: none.
    int (__stdcall* IsUnitVisibleToPlayer)(Player* player, Unit* unit);
    // Returns the players who hear a sound at `at`: bit p set when player p has explored, or with LOS on, sees the cell; a query. Hooks: none.
    unsigned int (__stdcall* SoundHeardMask)(const Vec3* at);
    // The players phase's line of sight pass, run by the engine for each unit slot of a player that holds a unit: moves the unit's line of sight to where it now is. Hooks: none.
    void (__stdcall* UpdateUnitLineOfSight)(Unit* unit);
    // Run by the engine after the line of sight pass of the viewing player: records a snapshot of the units and projectiles the radar shows, for the radar picture. Hooks: none.
    void (__stdcall* RecordRadarUnits)(void);
    // Run by the engine after the economy of the viewing player: updates which units radar, sonar and cloaking show and hide for each player. Hooks: none.
    void (__stdcall* UpdateSensorRadarAndCloak)(void);
    // Run by the engine after UpdateSensorRadarAndCloak: records a snapshot of the cells the players have mapped, when they changed, for the radar picture. Hooks: none.
    void (__stdcall* RecordRadarMapped)(void);
    // Run by the engine once a tick, after the last phase: removes the line of sight of each eyeball whose time is up. Hooks: none.
    void (__stdcall* ExpireEyeballs)(void);
    // Loads the terrain map file of the mission, outside the tick. Hooks: none.
    void (__stdcall* LoadTntMap)(void);
    // Builds the radar picture and its surfaces for a match, outside the tick. Hooks: none.
    void (__stdcall* InitRadar)(void);
    // Releases the radar picture surfaces, outside the tick. Hooks: none.
    void (__stdcall* FreeRadar)(void);
    // Releases the map's line of sight, sorting and icon data when a match shuts down. Hooks: none.
    void (__stdcall* FreeMapResources)(void);
    // Writes to out, as Ref<Unit>, the first capacity of the ready enemy units within a radius of a position that a player saw at its last RefreshUnitIndex, or, when it saw none there and has a finished unit of a type with flag 0x400 switched on, of the ready enemy units with flag 0x100 there, and returns how many there are; flags is unused. Hooks: none.
    int (__stdcall* GetVisibleEnemiesInRadius)(int player, const Vec3* pos, int radius, int flags, Ref<Unit>* out, int capacity);
    // Returns whether an enemy unit a player saw at its last RefreshUnitIndex is within a range of a position, ready (flag 0x10000000) and not dying (flag 0x4000). Hooks: none.
    bool (__stdcall* HasReadyUnitInRange)(int player, Vec3* pos, int range);
    // Returns 1 when the viewing player sees a position, on the grid of a player's explored map. Hooks: none.
    int (__stdcall* IsPointVisible)(Player* player, Vec3* pos);
    // Writes to out, as Ref<Unit>, the first capacity of the enemy units within a radius of a position that a player saw at its last RefreshUnitIndex, ready or not, and returns how many there are. Hooks: none.
    int (__stdcall* GetSeenUnitsInRadius)(int player, const Vec3* pos, int radius, Ref<Unit>* out, int capacity);
    // Creates the empty index of the enemy units a player sees, refreshed by the next RefreshUnitIndex, outside the tick. Hooks: none.
    void (__stdcall* CreateUnitIndex)(int player);
    // Frees the index of the enemy units a player sees, outside the tick. Hooks: none.
    void (__stdcall* FreeUnitIndex)(int player);
    // Every 30 ticks of a player that has an index, refills it from the ready units: the enemy units the player sees, the enemy units with flag 0x100, and whether it has a finished unit of a type with flag 0x400 switched on. Hooks: none.
    void (__stdcall* RefreshUnitIndex)(int player);
    // The bytes of private state the implementation needs for a map; called before Init.
    unsigned int (__stdcall* RegionSize)(const LosRadarMapInfo* map);
    // Builds the state for a match in a region the engine allocated of the size RegionSize gave; called once per match, before the first tick and before any event.
    void (__stdcall* Init)(const LosRadarMapInfo* map, void* region, unsigned int size);
    // Ends the match and returns the region, which the engine frees.
    void* (__stdcall* Shutdown)(void);
    // Fills the sight cache of a unit as it stands, for a saved game; a query. Hooks: none.
    void (__stdcall* SightCache)(Unit* unit, LosSightCache* out);
    // Writes the state that decides what the implementation does next and that the engine does not already hash elsewhere; it reads only, and keeps nothing.
    void (__stdcall* WriteState)(const StateWriter* writer);
    // Reads the visibility layer of a saved game, the "Mapping" account, from a bank, outside the tick. Hooks: none.
    void (__stdcall* LoadMappingData)(HapiBank* file);
    // Writes the visibility layer of a saved game, the "Mapping" account, to a bank, outside the tick. Hooks: none.
    void (__stdcall* SaveMappingData)(HapiBank* file);
};

#endif
