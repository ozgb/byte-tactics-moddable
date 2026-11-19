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
    void LoadCampaign(char* file);
    char* GetCampaignName();
    void LoadBriefing();
    char* GetBriefing();
    void SetNameSlot(int index, char* text);
    void BuildCampaignFilePath(int index, char* dir, char* name, char* ext);
    char* GetNameSlot(int index);
    int CountMissions();
    int BuildMissionList(char** out);
    int GetTerrainLength();
    char* GetDescription();
    char* GetPlanet();
    int GetTerrainSizeTier();
    int MissionExists(int index);
    int LoadMissionByName(char* map);
    int SelectMission(int index);
    char* GetTranslatedName();
    char* GetMissionName();
    bool HasMissionName();
    int GetMissionIndex();
    int AdvanceMission();
    void RefreshMapList(int multiplayer);
    int LoadMission(char* map);
    int SelectSchema(int type, TdfFile* parser, char* schema);
    void LoadMissionData(char* name, TdfFile* parser);
    void FreeMissionData();
    int CountStartPositions();
    int GetStartPosition(Vec3* out, int id);
    int ComputeMapChecksum();
};

#endif
