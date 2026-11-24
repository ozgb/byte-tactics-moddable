#ifndef MISSION_H
#define MISSION_H

#include "../util/tdf.h"
#include "../../interfaces/ref.h"

struct MissionRule {
    int type;
    int id;
    short x;
    short z;
};
struct Vec3;
struct MissionParams;
struct MissionUnitInfo;
struct MissionFeatureInfo;

struct MissionUnit {
    Ref<char> name;
    Ref<char> ident;
    Ref<char> initialMission;
    int x;
    int y;
    int z;
    short angle;
    short health;
    int creationCountdown;
    short buildPriority;
    unsigned char player;
    union {
        struct {
            unsigned char initialGroup : 4;
            unsigned char missionCritical : 1;
            unsigned char aiIgnore : 1;
            unsigned char aiPriorityTarget : 1;
            unsigned char immunity : 1;
        };
        unsigned char flags;
    };
};

struct MissionFeature {
    char name[0x80];
    int x;
    int y;
};

class Mission {
public:
    int type;
    char campaign[0x100];
    char names[9][0x100];
    int exists;
    TdfFile list;
    char missionName[0x100];
    char text_b14[0x100];
    Ref<char> briefing;
    int missionIndex;
    int tntChecksum;
    int headerChecksum;
    char description[0x80];
    char planet[0x80];
    char* mapList;
    int mapCount;
    int multi;
    int surfaceMetal;
    int minWindSpeed;
    int maxWindSpeed;
    int gravity;
    float tidalStrength;
    int lavaWorld;
    int noSeaLevelTrigger;
    int waterDoesDamage;
    int waterDamage;
    float killMul;
    float timeMul;
    float startMetal[10];
    float startEnergy[10];
    Ref<MissionUnit> units;
    int unitCount;
    Ref<MissionRule> rules;
    int ruleCount;
    Ref<MissionFeature> features;
    int featureCount;
    char memory[0x80];
    char numPlayers[0x80];

    Mission(int owner_);
    ~Mission();
    int GetGameType();
    void LoadCampaign(const char* file);
    const char* GetCampaignName();
    void LoadBriefing();
    const char* GetBriefing();
    void SetNameSlot(int index, const char* text);
    void BuildCampaignFilePath(int index, const char* dir, const char* name, const char* ext);
    const char* GetNameSlot(int index);
    int CountMissions();
    int BuildMissionList(char** out);
    int GetCampaignMissionName(int index, char* out, int size);
    int GetTerrainLength();
    const char* GetDescription();
    char* GetPlanet();
    int GetTerrainSizeTier();
    int MissionExists(int index);
    int LoadMissionByName(const char* map);
    int SelectMission(int index);
    const char* GetTranslatedName();
    const char* GetMissionName();
    bool HasMissionName();
    int GetMissionIndex();
    int AdvanceMission();
    void RefreshMapList(int multiplayer);
    int LoadMission(const char* map);
    int SelectSchema(int type, TdfFile* parser, char* schema);
    void LoadMissionData(char* name, TdfFile* parser);
    void FreeMissionData();
    int CountStartPositions();
    int GetStartPosition(Vec3* out, int id);
    int ComputeMapChecksum();
    void GetParams(MissionParams* out);
    int CountUnits();
    int GetUnit(int index, MissionUnitInfo* out);
    void SetUnitPosition(int index, Vec3* pos);
    int CountFeatures();
    int GetFeature(int index, MissionFeatureInfo* out);
    const char* GetMemory();
    const char* GetNumPlayers();
};

#endif
