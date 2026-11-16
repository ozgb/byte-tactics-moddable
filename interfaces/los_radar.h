#ifndef INTERFACES_LOS_RADAR_H
#define INTERFACES_LOS_RADAR_H

struct Cell;
struct Player;
struct Unit;
struct Vec3;

// Major version of the los_radar interface, changed when a slot is removed or its signature changes.
#define LOS_RADAR_VERSION_MAJOR 3
// Minor version of the los_radar interface, changed when a slot is added.
#define LOS_RADAR_VERSION_MINOR 0

// The kinds of event other modules and the engine publish to los_radar's inbox; each is an intent, validated when Apply consumes it, and applied in the order it was published. An event that names a unit with a null unit changes nothing; outcome 0.
enum LosRadarEventKind {
    // Moves the line of sight of unit to where it now is; outcome 1.
    LOS_RADAR_UPDATE_UNIT = 128,
    // Takes the line of sight of unit off the map, as it dies; outcome 1.
    LOS_RADAR_REMOVE_UNIT = 129,
    // Gives a new unit its first line of sight; outcome 1.
    LOS_RADAR_REVEAL_UNIT = 130,
    // Adds an eyeball, a temporary view of the viewing player at pos, with sightDistance and eyeHeight, for lifetime ticks; outcome 1 when it was added, 0 when line of sight is off or no eyeball is free.
    LOS_RADAR_ADD_EYEBALL = 131,
    // Recomputes the terrain height range of the cells in the rectangle of cell and size, in cells; outcome 1.
    LOS_RADAR_CELL_HEIGHTS = 132,
    // Rebuilds the line of sight of every unit and the radar picture; value 1 first resets what each cell shows to every player; outcome 1.
    LOS_RADAR_RECALCULATE = 133,
    // Toggles whether player shares its map with its allies and tells the other players, in a network game; outcome 1 when it toggled, 0 outside a network game.
    LOS_RADAR_SHARE_MAPPING = 134,
    // Toggles whether player shares its radar with its allies and tells the other players, in a network game; outcome 1 when it toggled, 0 outside a network game.
    LOS_RADAR_SHARE_RADAR = 135,
    // Shows player value every cell that player sees now; outcome 1.
    LOS_RADAR_SHARE_MAP = 136
};

// An event (LosRadarEventKind), as plain data with no pointers; fields a kind does not use are 0.
struct LosRadarEvent {
    // A LosRadarEventKind.
    int kind;
    // The unit, as the handle economy's events use.
    unsigned int unit;
    // The player, 0 to 9.
    int player;
    // The argument of the kind, as the kind says.
    int value;
    // The eyeball's position, in world coordinates with 16 fractional bits.
    int pos[3];
    // The eyeball's sight distance, in world units.
    int sightDistance;
    // The height of the eyeball's eye above its position, in world units.
    int eyeHeight;
    // The eyeball's lifetime, in ticks.
    int lifetime;
    // The first cell of the rectangle, x and y.
    int cell[2];
    // The width and height of the rectangle, in cells.
    int size[2];
};

// Line of sight and radar. The engine delivers los_radar's pending inbox, through Apply, before each pass below and at each phase boundary.
struct LosRadarSeam {
    // Consumes one queued event of the inbox and returns its outcome, which the engine records in the trace's dump and returns to no one.
    int (__stdcall* Apply)(const LosRadarEvent* event);
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
};

#endif
