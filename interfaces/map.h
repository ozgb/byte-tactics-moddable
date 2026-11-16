#ifndef INTERFACES_MAP_H
#define INTERFACES_MAP_H

struct Vec3;
struct Point16;
struct Unit;
class UnitDef;
struct Player;
class Mission;
class HapiBank;

// Major version of the map interface, changed when a slot is removed or its signature changes.
#define MAP_VERSION_MAJOR 4
// Minor version of the map interface, changed when a slot is added.
#define MAP_VERSION_MINOR 0

// The kinds of event other modules publish to map's inbox; each is an intent, validated when Apply consumes it, and applied in the order it was published. An event that names a null unit, a movement class that does not exist, a bucket change the unit's bucket does not allow, or a rectangle of negative size changes nothing; outcome 0.
enum MapEventKind {
    // Puts unit on the cells its footprint covers at its cell and in the spatial grid bucket of its position, or in the bucket of units off the map; outcome 1.
    MAP_ADD_UNIT = 160,
    // Takes unit off the cells its footprint covers, and lets the units it overlapped claim them again; outcome 1.
    MAP_REMOVE_UNIT = 161,
    // Takes unit off the cells, as MAP_REMOVE_UNIT does, and out of the spatial grid, as it dies; outcome 1.
    MAP_CLEAR_UNIT = 162,
    // Makes the units whose footprints overlap unit's give up the cells unit covers; outcome 1.
    MAP_RECLAIM_FOOTPRINT = 163,
    // Opens (value 1) or closes (value 0) the yard of unit when its footprint is clear for that state, and claims its cells for the state; outcome 1 when it changed, 0 when the footprint was not clear.
    MAP_SET_YARD = 164,
    // Takes unit out of the spatial grid bucket it is in, as it boards a carrier; outcome 1, or 0 when unit has no bucket or is not in its bucket's chain.
    MAP_BUCKET_UNLINK = 165,
    // Puts unit at the front of its spatial grid bucket, as it leaves a carrier; outcome 1, or 0 when unit has no bucket or is already in its bucket's chain.
    MAP_BUCKET_PREPEND = 166,
    // Computes again the pass map of movement class value (its index, 0 to 31) over the rectangle of cell and size, in cells, grown by the class's footprint; outcome 1.
    MAP_REFRESH_PASS_MAP = 167,
    // Computes again the pass maps of every movement class over the rectangle of cell and size, after the terrain or the features on it changed; outcome 1, or 0 when either size is negative.
    MAP_PASS_MAPS = 168
};

// The kinds of effect map publishes to its outbox, for the modules that subscribe to them.
enum MapEffectKind {
    // The pass maps of the movement classes whose index bits are set in classes changed over rect (left, top, right, bottom, in cells), for cause, a PathChange.
    MAP_EFFECT_CELLS_CHANGED = 176
};

// An event (MapEventKind) or an effect (MapEffectKind), as plain data with no pointers; fields a kind does not use are 0.
struct MapEvent {
    // A MapEventKind or a MapEffectKind.
    int kind;
    // The unit, as its block offset (an offset from the block base, 0 for none), as economy's events give a unit.
    unsigned int unit;
    // The argument of the kind, as the kind says.
    int value;
    // The first cell of a rectangle, x and y.
    int cell[2];
    // The width and height of a rectangle, in cells.
    int size[2];
    // The rectangle of an effect, in cells: left, top, right and bottom.
    int rect[4];
    // The movement classes of an effect, a bit for each class index.
    unsigned int classes;
    // The cause of an effect, a PathChange.
    int cause;
};

// Terrain and map data: the cells units stand on, the spatial grid of units, the pass maps and the mission. The engine delivers map's pending inbox, through Apply, before each synchronous slot below, and at each phase boundary.
struct MapSeam {
    // Consumes one queued event of the inbox and returns its outcome, which the engine records in the trace's dump and returns to no one.
    int (__stdcall* Apply)(const MapEvent* event);
    // Writes to ids the unit ids of the units within range of pos, in the order the spatial grid holds them, skipping the first skip of them, and returns how many it wrote, at most capacity; a query. Hooks: none.
    int (__stdcall* UnitsInRange)(const Vec3* pos, int range, int skip, unsigned short* ids, int capacity);
    // Returns the mean ground height of the cell under a position; a query. Hooks: none.
    int (__stdcall* GetCellMeanHeight)(Vec3* pos);
    // Returns the game type of a mission (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* GetGameType)(Mission* mission);
    // Returns one of the file names of a mission, by slot number (thiscall in 3.1c, this first). Hooks: none.
    char* (__stdcall* GetNameSlot)(Mission* mission, int index);
    // Synchronous: returns 1 when a unit type can be built at a cell, with a visibility check when player is given, and leaves the metal of the cells it read and the height of a fit in the placement record, which the caller reads in the same statement. Hooks: none.
    int (__stdcall* CanBuildAt)(UnitDef* unit, Point16 cell, short type, Player* player);
    // Synchronous: returns 1 when the footprint of a unit type fits at a cell (flags 2 counts a cell off the map as a fit); for a type that does not move it answers and fills the placement record as CanBuildAt does. Hooks: none.
    int (__stdcall* CanPlaceUnitFootprint)(UnitDef* unit, unsigned short otherTypeId, Point16 cell, int flags);
    // Builds the pass map of every movement class for the loaded terrain, outside the tick. Hooks: none.
    void (__stdcall* BuildAllPassMaps)(void);
    // Frees the cache of mission names and the mission object. Hooks: none.
    void (__stdcall* FreeOtaEnumCacheAndMission)(void);
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
    // Sets the kind of mission the game runs (campaign, skirmish or multiplayer). Hooks: none.
    void (__stdcall* SetMissionType)(int type);
    // Moves a position to where the footprint of a unit type sits on the grid; a query that writes only the position it is given. Hooks: none.
    void (__stdcall* SnapWorldPosToFootprint)(UnitDef* unit, Vec3* pos);
    // Returns 1 when the footprint of a unit fits at a position; a query. Hooks: none.
    int (__stdcall* CanPlaceFootprintAt)(Unit* unit, Vec3* pos);
    // Returns 1 when a slot of a landing pad is free; a query. Hooks: none.
    int (__stdcall* IsPadSlotFree)(Unit* pad, int id);
    // Synchronous: picks a random energy and a random metal feature to reclaim within a radius, drawing from the game logic stream, writes their positions and amounts, and returns 1 when it found either. Hooks: none.
    int (__stdcall* PickRandomReclaimableResourcesInRadius)(Vec3* pos, int range, Vec3** energy, float* energyAmount, Vec3** metal, float* metalAmount);
    // Moves a campaign on to its next mission and returns 1 when there is one (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* AdvanceMission)(Mission* mission);
    // Returns the name of the campaign file of a mission (thiscall in 3.1c: this in ecx). Hooks: none.
    char* (__fastcall* GetCampaignName)(Mission* mission);
    // Returns the index of the current mission of a campaign (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* GetMissionIndex)(Mission* mission);
    // Returns the name of the current mission (thiscall in 3.1c: this in ecx). Hooks: none.
    char* (__fastcall* GetMissionName)(Mission* mission);
    // Writes the start position of a player and returns 1 when the mission has one (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* GetStartPosition)(Mission* mission, int unusedEdx, Vec3* out, int id);
    // Loads a campaign file (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* LoadCampaign)(Mission* mission, int unusedEdx, char* file);
    // Loads a mission by its name and returns 1 when it exists (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* LoadMissionByName)(Mission* mission, int unusedEdx, char* name);
    // Returns 1 when a campaign has a mission of that index (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* MissionExists)(Mission* mission, int unusedEdx, int index);
    // Rebuilds the list of maps, for multiplayer when set (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* RefreshMapList)(Mission* mission, int unusedEdx, int multiplayer);
    // Makes the mission of that index of a campaign the current one and returns 1 when it exists (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* SelectMission)(Mission* mission, int unusedEdx, int index);
    // Returns the checksum of the terrain file of a mission (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* ComputeMapChecksum)(Mission* mission);
    // Returns the size of the terrain file of a mission, or 0 when it has none (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* GetTerrainLength)(Mission* mission);
};

#endif
