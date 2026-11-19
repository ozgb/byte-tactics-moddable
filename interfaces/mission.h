#ifndef INTERFACES_MISSION_H
#define INTERFACES_MISSION_H

struct Vec3;
class TdfFile;

// Major version of the mission interface, changed when a slot is removed or its signature changes.
#define MISSION_VERSION_MAJOR 1
// Minor version of the mission interface, changed when a slot is added.
#define MISSION_VERSION_MINOR 0

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
    // Writes the name of the campaign's mission of that index, in the player's language, to a buffer of size bytes the caller owns, and returns 0 when the campaign has no such mission. Hooks: none.
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
};

#endif
