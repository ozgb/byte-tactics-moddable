#ifndef GAME_SETTINGS_H
#define GAME_SETTINGS_H

#include "mission_option_flags.h"

#pragma pack(push, 1)
struct SkirmishPlayer {
    int controller;
    int side;
    int allyGroup;
    int metal;
    int energy;
    int color;
};

struct SkirmishOptionsTable {
    SkirmishPlayer players[11];
    union {
        struct {
            int commanderDeath;
            int mapping;
            int lineOfSight;
            int losType;
        };
        MissionOptionFlags missionFlags;
    };
    int fixedLocations;
    char mapName[0x220 - 0x11c];
    int baseGadgetCount;
    int current;
    int difficulty;
};

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(SkirmishOptionsTable) == 0x22c, "SkirmishOptionsTable");
#endif

struct Options {
    SkirmishPlayer players[10];
    char unknown_f0[0x108 - 10 * 0x18];
    int skirmishCommanderDeath;
    int skirmishMapping;
    int skirmishLineOfSight;
    int skirmishLOSType;
    int fixedLocations;
    char skirmishMap[0x228 - 0x11c];
    int skirmishDifficulty;
};
#pragma pack(pop)

void __stdcall SaveTrackSettings(unsigned char* tracks);
void __stdcall LoadSettings();
void __stdcall ApplyUseOnlyUnits();

void __stdcall WriteGameRegistryValue(void* key, void* buf, int value);
int __stdcall ReadGameRegistryValue(const char* key, void* buf, unsigned int* size);
void __stdcall SaveSettings();

#endif
