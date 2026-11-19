#ifndef INTERFACES_MISSION_H
#define INTERFACES_MISSION_H

struct Vec3;
class TdfFile;

// Major version of the mission interface, changed when a slot is removed or its signature changes.
#define MISSION_VERSION_MAJOR 1
// Minor version of the mission interface, changed when a slot is added.
#define MISSION_VERSION_MINOR 1

// The figures of the mission that the simulation reads, as plain data, as of the mission's last load.
struct MissionParams {
    // The metal of every cell of the map (SurfaceMetal); a negative value leaves the terrain file's.
    int surfaceMetal;
    // The lowest wind speed (minwindspeed); a negative value leaves the terrain file's.
    int minWindSpeed;
    // The highest wind speed (maxwindspeed); a negative value leaves the terrain file's.
    int maxWindSpeed;
    // The gravity (gravity); a negative value leaves the terrain file's.
    int gravity;
    // The tidal strength (tidalstrength); a negative value leaves the terrain file's.
    float tidalStrength;
    // Whether the sea is lava (lavaworld).
    int lavaWorld;
    // Whether reaching the sea level sets nothing off: no splash, and a projectile does not detonate on the sea (nosealeveltrigger).
    int noSeaLevelTrigger;
    // Whether the sea damages the units in it (waterdoesdamage).
    int waterDoesDamage;
    // The damage the sea does to a unit in it every second (waterdamage).
    int waterDamage;
    // The score of a kill (killmul).
    float killMul;
    // The score of a second of play (timemul).
    float timeMul;
    // The metal each player starts a campaign mission with, by player.
    float startMetal[10];
    // The energy each player starts a campaign mission with, by player.
    float startEnergy[10];
};

// A unit the mission places at the start of a match, as plain data; its strings point into the mission's own record and stay valid until the mission loads another.
struct MissionUnitInfo {
    // The name of the unit's type, or 0.
    char* name;
    // The name the mission's other units and conditions know it by, or 0.
    char* ident;
    // The orders it starts with, or 0.
    char* initialMission;
    // Its position across the map, in world units as 16.16 fixed point.
    int x;
    // Its height, in world units as 16.16 fixed point.
    int y;
    // Its position down the map, in world units as 16.16 fixed point.
    int z;
    // Its heading, in 65536ths of a turn.
    short angle;
    // Its health, in percent of the type's.
    short health;
    // The mission's creation countdown of the unit.
    int creationCountdown;
    // The mission's build priority of the unit.
    short buildPriority;
    // Its player, from 1.
    unsigned char player;
    // Bits 0 to 3 its initial group, bit 4 mission critical, bit 5 ignored by the AI, bit 6 a priority target of the AI and bit 7 immune.
    unsigned char flags;
};

// A feature the mission places at the start of a match, as plain data; its name points into the mission's own record and stays valid until the mission loads another.
struct MissionFeatureInfo {
    // The name of the feature's type, or an empty string for none.
    char* name;
    // Its column on the map, in cells.
    int x;
    // Its row on the map, in cells.
    int y;
};

// The mission: the game type and the files of the match, the campaign and its missions, the start positions and the list of maps. The engine holds one mission, which every slot works on; a char* a slot returns points into the mission's own record and stays valid until the mission loads another.
struct MissionSeam {
    // Returns the game type of the mission (0 none, 1 campaign, 2 skirmish, 3 multiplayer); a query of a record nothing in the tick writes. Hooks: none.
    int (__stdcall* GetGameType)(void);
    // Returns one of the file names of the mission by slot number (0 the campaign file, 1 the terrain file, 2 the briefing), or 0 when the slot is empty; a query of a record nothing in the tick writes. Hooks: none.
    char* (__stdcall* GetNameSlot)(int index);
    // Makes a new, empty mission of a game type, unless the mission already has that type, outside the tick. Hooks: none.
    void (__stdcall* SetMissionType)(int type);
    // Frees the cache of map names and the mission, outside the tick. Hooks: none.
    void (__stdcall* FreeOtaEnumCacheAndMission)(void);
    // Moves a campaign on to its next mission and loads it, and returns 1 when there is one. Hooks: none.
    int (__stdcall* AdvanceMission)(void);
    // Returns the name of the campaign file, or 0 when the mission has no campaign. Hooks: none.
    char* (__stdcall* GetCampaignName)(void);
    // Returns the index of the current mission of the campaign. Hooks: none.
    int (__stdcall* GetMissionIndex)(void);
    // Returns the name of the current mission. Hooks: none.
    char* (__stdcall* GetMissionName)(void);
    // Writes the start position of a player and returns 1 when the mission has one. Hooks: none.
    int (__stdcall* GetStartPosition)(Vec3* out, int id);
    // Loads a campaign file and its first mission, or empties the campaign for an empty name. Hooks: none.
    void (__stdcall* LoadCampaign)(char* file);
    // Loads a map (skirmish and multiplayer) or a mission of the campaign by its name, and returns 1 when it exists. Hooks: none.
    int (__stdcall* LoadMissionByName)(char* name);
    // Returns 1 when the campaign has a mission of that index. Hooks: none.
    int (__stdcall* MissionExists)(int index);
    // Builds the list of maps again when it has none or when it leaves multiplayer, for multiplayer when set, and loads its first map. Hooks: none.
    void (__stdcall* RefreshMapList)(int multiplayer);
    // Makes the mission of that index of the campaign the current one, loads it and returns 1 when it exists. Hooks: none.
    int (__stdcall* SelectMission)(int index);
    // Returns the checksum of the mission's header and its terrain file, read once per file and cached. Hooks: none.
    int (__stdcall* ComputeMapChecksum)(void);
    // Returns the size of the terrain file, or 0 when the mission has none. Hooks: none.
    int (__stdcall* GetTerrainLength)(void);
    // Returns how many missions the campaign has, or 0 when the mission has no campaign. Hooks: none.
    int (__stdcall* CountMissions)(void);
    // Writes the name of the campaign's mission of that index, in the player's language, to a buffer of size bytes the caller owns, and returns 0 when the campaign has no such mission or size is below 1. Hooks: none.
    int (__stdcall* GetCampaignMissionName)(int index, char* out, int size);
    // Returns how many start positions the mission has. Hooks: none.
    int (__stdcall* CountStartPositions)(void);
    // Returns the text of the mission's briefing, or 0 when it has none. Hooks: none.
    char* (__stdcall* GetBriefing)(void);
    // Returns the description of the map. Hooks: none.
    char* (__stdcall* GetDescription)(void);
    // Returns the planet of the map. Hooks: none.
    char* (__stdcall* GetPlanet)(void);
    // Returns the memory a player needs for the terrain file, in megabytes, by its size. Hooks: none.
    int (__stdcall* GetTerrainSizeTier)(void);
    // Returns the name of the map in the player's language. Hooks: none.
    char* (__stdcall* GetTranslatedName)(void);
    // Returns 1 when the mission has a name. Hooks: none.
    int (__stdcall* HasMissionName)(void);
    // Reads from a map's parsed file the schema of a game type for the players of the match, by name when schema is given, and returns 1 when the map has one. Hooks: none.
    int (__stdcall* SelectSchema)(int type, TdfFile* parser, char* schema);
    // Writes the figures of the mission that the simulation reads to a struct the caller owns; a query of a record nothing in the tick writes. Hooks: none.
    void (__stdcall* GetMissionParams)(MissionParams* out);
    // Returns how many units the mission places at the start of a match. Hooks: none.
    int (__stdcall* CountMissionUnits)(void);
    // Writes the mission's unit of that index to a struct the caller owns, and returns 0 when the mission has no such unit. Hooks: none.
    int (__stdcall* GetMissionUnit)(int index, MissionUnitInfo* out);
    // Sets the position of the mission's unit of that index, as the load of a match snaps it to the unit's footprint, outside the tick. Hooks: none.
    void (__stdcall* SetMissionUnitPosition)(int index, Vec3* pos);
    // Returns how many features the mission places at the start of a match. Hooks: none.
    int (__stdcall* CountMissionFeatures)(void);
    // Writes the mission's feature of that index to a struct the caller owns, and returns 0 when the mission has no such feature. Hooks: none.
    int (__stdcall* GetMissionFeature)(int index, MissionFeatureInfo* out);
    // Returns the memory the map says a player needs, as the map's text (memory). Hooks: none.
    char* (__stdcall* GetMemory)(void);
    // Returns the numbers of players the map is for, as the map's text (numplayers). Hooks: none.
    char* (__stdcall* GetNumPlayers)(void);
};

#endif
