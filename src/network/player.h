#ifndef PLAYER_H
#define PLAYER_H
#include "../../interfaces/ref.h"

struct PlayerInfo;
struct Unit;
class SquadManager;
struct Group;
struct UnitResources;

#pragma pack(push, 1)
struct Player {
    int active;
    unsigned int id;
    int joinTime;
    int lobbyDataSynced;
    int messages;
    int ping;
    int syncTick;
    int lastHeard;
    unsigned char progress;
    unsigned char keepaliveFlags;
    unsigned char rejectReason;
    char unknown_23[4];
    Ref<PlayerInfo> info;
    char name[0x1e];
    char fullName[0x1e];
    Ref<Unit> unitsBegin;
    Ref<Unit> unitsEnd;
    unsigned short firstIndex;
    unsigned short lastIndex;
    unsigned char type;
    Ref<SquadManager> ai;
    Ref<Group> groups;
    Ref<unsigned char> explored;
    int exploredWidth;
    int exploredHeight;
    int exploredSize;
    union {
        float energy;
        int energyBits;
    };
    float energyIncome;
    float energyUsage;
    union {
        float metal;
        int metalBits;
    };
    float metalIncome;
    float metalUsage;
    union {
        float energyCapacity;
        int energyCapacityBits;
    };
    union {
        float metalCapacity;
        int metalCapacityBits;
    };
    union {
        double totalEnergyProduced;
        struct {
            int totalEnergyProducedLo;
            int totalEnergyProducedHi;
        };
    };
    union {
        double totalMetalProduced;
        struct {
            int totalMetalProducedLo;
            int totalMetalProducedHi;
        };
    };
    union {
        double totalEnergyConsumed;
        struct {
            int totalEnergyConsumedLo;
            int totalEnergyConsumedHi;
        };
    };
    union {
        double totalMetalConsumed;
        struct {
            int totalMetalConsumedLo;
            int totalMetalConsumedHi;
        };
    };
    union {
        double energyWasted;
        struct {
            int energyWastedLo;
            int energyWastedHi;
        };
    };
    union {
        double metalWasted;
        struct {
            int metalWastedLo;
            int metalWastedHi;
        };
    };
    float energyStorageBonus;
    float metalStorageBonus;
    float shareMetal;
    float shareEnergy;
    Ref<UnitResources> econ;
    int updateTime;
    int winLoseTime;
    int displayTimer;
    short kills;
    short losses;
    unsigned short unused_100;
    unsigned short unused_102;
    short commanderKills;
    short commanderLosses;
    unsigned char allied[11];
    unsigned char alliedBy[11];
    unsigned char shareLos[11];
    unsigned char shareVision[11];
    unsigned char shareMapping[11];
    unsigned char alliance;
    int unitsCreated;
    unsigned short unitCount;
    unsigned char index;
    unsigned char startPos;
    union {
        struct {
            unsigned char rank;
            union {
                unsigned short flags;
                struct {
                    unsigned short storage : 1;
                    unsigned short bits_149_1 : 15;
                } bits_149;
            };
        };
        struct {
            unsigned short bits0 : 8;
            unsigned short started : 1;
        } bits_148;
    };

    Player();
    void FreeSideDataAndFogSightCounts();
    void SetType(int newType);
    int IsPlayableSlot();
    void Clear() { active = 0; type = 0; }
    float GetEnergy() { return energy; }
    float GetMetal() { return metal; }
};
#pragma pack(pop)

#endif
