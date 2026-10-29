#include "win_types.h"
#include <stdio.h>
#include <string.h>
#include "../map/mission.h"
#include "settings.h"

#pragma pack(push, 1)
struct GraphicsSettingFlags {
    unsigned short damagebars : 1;
    unsigned short antiAlias : 1;
    unsigned short shadows : 1;
    unsigned short vehicleShadows : 1;
    unsigned short featureShadows : 1;
    unsigned short shading : 1;
    unsigned short ditheredFog : 1;
    unsigned short unused7 : 1;
    unsigned short switchAlt : 1;
};

struct SoundFlags {
    unsigned short soundMode : 3;
    unsigned short restoreVolume : 1;
    unsigned short ackfx : 1;
    unsigned short buildfx : 1;
    unsigned short speechfx : 1;
};

struct MusicSettingFlags {
    unsigned short musicmode : 1;
};

#include "game.h"

#include "../units/unit_def.h"

#include "../sound/sound.h"
#include "../../interfaces/seams.h"
#include "../host/settings_io.h"

#pragma pack(pop)

#include "../../link/globals.h"
extern char g_onlineLobbyPlayerName[];
extern char g_cmdlineHostGameName[];

#include "../util/registry.h"
#include "../network/online.h"
#include "../debug/debug_lib.h"

// FUNCTION: 0x42f910
void __stdcall SaveTrackSettings(unsigned char* tracks)
{
    char name[12];
    int i;

    for (i = 0; i < 10; i++) {
        sprintf(name, "track%d", i);
        SettingSetInt((void*)"Total Annihilation", name, tracks[i]);
    }
}

// FUNCTION: 0x42f960
void __stdcall WriteGameRegistryValue(void* key, void* buf, int value)
{
    SettingSetBlob((void*)"Total Annihilation", key, buf, value);
}

// FUNCTION: 0x42f980
int __stdcall ReadGameRegistryValue(const char* key, void* buf, unsigned int* size)
{
    return SettingGetBlob("Total Annihilation", key, buf, size);
}

static inline void ReadSettingInt(const char* key, int Game::* field, int defaultValue, int& value)
{
    int ok = SettingGetInt((void*)"Total Annihilation", (void*)key, &value);
    if (ok != 0) {
        g_game->*field = value;
    } else {
        g_game->*field = defaultValue;
        SettingSetInt((void*)"Total Annihilation", (void*)key, g_game->*field);
    }
}

static inline void ReadSettingIntOr(const char* key, int Game::* field, int defaultValue, int& value)
{
    if (SettingGetInt((void*)"Total Annihilation", (void*)key, &value) != 0) {
        g_game->*field = value;
    } else {
        g_game->*field = defaultValue;
    }
}

static inline void ReadSettingInt(const char* key, int SkirmishOptionsTable::* field, int defaultValue, int& value)
{
    int ok = SettingGetInt((void*)"Total Annihilation", (void*)key, &value);
    if (ok != 0) {
        g_game->options->*field = value;
    } else {
        g_game->options->*field = defaultValue;
        SettingSetInt((void*)"Total Annihilation", (void*)key, g_game->options->*field);
    }
}

static inline void ReadPlayerSetting(const char* key, int SkirmishPlayer::* field, int defaultValue, int& value, int i)
{
    if (SettingGetInt((void*)"Total Annihilation\\Skirmish", (void*)key, &value) != 0) {
        g_game->options->players[i].*field = value;
    } else {
        g_game->options->players[i].*field = defaultValue;
    }
}

// FUNCTION: 0x42f9a0
void LoadSettings()
{
    int value;
    char name[32];
    char buf[256];
    int i;

    int ok1 = SettingGetInt((void*)"Total Annihilation", (void*)"Interface Type", &value);
    if (ok1 != 0) {
        if (value > 1) value = 1;
        g_game->interfaceType = value;
    } else {
        g_game->interfaceType = 0;
        SettingSetInt((void*)"Total Annihilation", (void*)"Interface Type", g_game->interfaceType);
    }
    ReadSettingInt("DisplaymodeWidth", &Game::width_37f1b, 0x280, value);
    ReadSettingInt("DisplaymodeHeight", &Game::height_37f1f, 0x1e0, value);
    ReadSettingInt("side", &Game::flag_37ef2, 0, value);
    int ok5 = SettingGetInt((void*)"Total Annihilation", (void*)"Difficulty", &value);
    if (ok5 != 0) {
        g_game->difficulty = value & 0xffff;
    } else {
        g_game->difficulty = 1;
        SettingSetInt((void*)"Total Annihilation", (void*)"Difficulty", g_game->difficulty);
    }
    int ok6 = SettingGetInt((void*)"Total Annihilation", (void*)"scrollspeed", &value);
    if (ok6 != 0) {
        g_game->scrollSpeed = (unsigned char)value;
    } else {
        g_game->scrollSpeed = 0x20;
        SettingSetInt((void*)"Total Annihilation", (void*)"scrollspeed", g_game->scrollSpeed);
    }
    ReadSettingInt("SingleCommanderDeath", &Game::singleCommanderDeath, 1, value);
    ReadSettingInt("SingleMapping", &Game::singleMapping, 1, value);
    ReadSettingInt("SingleLineOfSight", &Game::singleLineOfSight, 1, value);
    ReadSettingInt("SingleLOSType", &Game::singleLOSType, 1, value);
    ReadSettingInt("screenchat", &Game::screenchat, 1, value);
    int ok12 = SettingGetInt((void*)"Total Annihilation", (void*)"damagebars", &value);
    if (ok12 != 0) {
        (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).damagebars = value;
    } else {
        (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).damagebars = 0;
        SettingSetInt((void*)"Total Annihilation", (void*)"damagebars", (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).damagebars);
    }
    int ok13 = SettingGetInt((void*)"Total Annihilation", (void*)"Sound Mode", &value);
    if (ok13 != 0) {
        if (value == 2) {
            ((Sound*)g_game->sound)->Enable3D();
        } else {
            ((Sound*)g_game->sound)->Disable3D();
        }
        (*(struct SoundFlags*)&g_game->soundFlags).soundMode = value;
    } else {
        SettingSetInt((void*)"Total Annihilation", (void*)"Sound Mode",
                     (((Sound*)g_game->sound)->Is3DEnabled() != 0) + 1);
        (*(struct SoundFlags*)&g_game->soundFlags).soundMode = 1;
    }
    int ok14 = SettingGetInt((void*)"Total Annihilation", (void*)"MixingBuffers", &value);
    if (ok14 != 0) {
        ((Sound*)g_game->sound)->SetMaxBuffers(value);
    } else {
        ((Sound*)g_game->sound)->SetMaxBuffers(8);
    }
    int ok15 = SettingGetInt((void*)"Total Annihilation", (void*)"RestoreVolume", &value);
    if (ok15 != 0) {
        (*(struct SoundFlags*)&g_game->soundFlags).restoreVolume = value;
    } else {
        (*(struct SoundFlags*)&g_game->soundFlags).restoreVolume = 0;
    }
    if ((*(struct SoundFlags*)&g_game->soundFlags).restoreVolume) {
        int ok16 = SettingGetInt((void*)"Total Annihilation", (void*)"WaveOutVolume", &value);
        if (ok16 != 0) {
            ((Sound*)g_game->sound)->SetWaveVolume(value);
        }
        int ok17 = SettingGetInt((void*)"Total Annihilation", (void*)"CDAudioVolume", &value);
        if (ok17 != 0) {
            ((Sound*)g_game->sound)->SetAuxVolume(value, 0);
        }
    }
    int ok18 = SettingGetInt((void*)"Total Annihilation", (void*)"Anti-Alias", &value);
    if (ok18 != 0) {
        (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).antiAlias = value;
    } else {
        (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).antiAlias = 1;
        SettingSetInt((void*)"Total Annihilation", (void*)"Anti-Alias", (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).antiAlias);
    }
    int ok19 = SettingGetInt((void*)"Total Annihilation", (void*)"Shadows", &value);
    if (ok19 != 0) {
        (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).shadows = value;
    } else {
        (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).shadows = 1;
        SettingSetInt((void*)"Total Annihilation", (void*)"Shadows", (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).shadows);
    }
    int ok20 = SettingGetInt((void*)"Total Annihilation", (void*)"FeatureShadows", &value);
    if (ok20 != 0) {
        (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).featureShadows = value;
    } else {
        (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).featureShadows = 1;
        SettingSetInt((void*)"Total Annihilation", (void*)"FeatureShadows", (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).featureShadows);
    }
    int ok21 = SettingGetInt((void*)"Total Annihilation", (void*)"VehicleShadows", &value);
    if (ok21 != 0) {
        (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).vehicleShadows = value;
    } else {
        (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).vehicleShadows = 1;
        SettingSetInt((void*)"Total Annihilation", (void*)"VehicleShadows", (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).vehicleShadows);
    }
    int ok22 = SettingGetInt((void*)"Total Annihilation", (void*)"Shading", &value);
    if (ok22 != 0) {
        (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).shading = value;
    } else {
        (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).shading = 1;
        SettingSetInt((void*)"Total Annihilation", (void*)"Shading", (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).shading);
    }
    int ok23 = SettingGetInt((void*)"Total Annihilation", (void*)"DitheredFog", &value);
    if (ok23 != 0) {
        (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).ditheredFog = value;
    } else {
        (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).ditheredFog = 0;
        SettingSetInt((void*)"Total Annihilation", (void*)"DitheredFog", (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).ditheredFog);
    }
    if (SettingGetInt((void*)"Total Annihilation", (void*)"Gamma", &value) != 0) {
        if (value == 10) {
            g_game->brightness = 0xc;
        } else {
            g_game->brightness = value;
        }
    } else {
        g_game->brightness = 0xc;
        SettingSetInt((void*)"Total Annihilation", (void*)"Gamma", g_game->brightness);
    }
    if (SettingGetInt((void*)"Total Annihilation", (void*)"SwitchAlt", &value) != 0) {
        (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).switchAlt = value;
    } else {
        (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).switchAlt = 0;
        SettingSetInt((void*)"Total Annihilation", (void*)"SwitchAlt", (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).switchAlt);
    }
    value = 0xb;
    if (SettingGetString((void*)"Total Annihilation", (void*)"Password", g_game->password_2be3_1176a, &value) == 0) {
        g_game->password_2be3_1176a[0] = 0;
    }
    if (IsOnlineConfigLoaded() != 0 && g_onlineLobbyPlayerName[0] != 0) {
        g_game->nickname[0] = 0;
        strncat(g_game->nickname, g_onlineLobbyPlayerName, 0x10);
    } else {
        value = 0x11;
        if (SettingGetString((void*)"Total Annihilation", (void*)"Nickname", g_game->nickname, &value) == 0) {
            g_game->nickname[0] = 0;
        }
    }
    if (g_cmdlineHostGameName[0] != 0) {
        g_game->gameName_2bc1[0] = 0;
        strncat(g_game->gameName_2bc1, g_cmdlineHostGameName, 0x10);
    } else {
        value = 0x11;
        if (SettingGetString((void*)"Total Annihilation", (void*)"Game Name", g_game->gameName_2bc1, &value) == 0) {
            g_game->gameName_2bc1[0] = 0;
        }
    }
    value = 0x100;
    if (SettingGetString((void*)"Total Annihilation", (void*)"Image Output Directory",
                     g_game->imageOutputDirectory, &value) == 0) {
        if (GetWindowsUserName(buf) == 0) {
            strcpy(buf, "user_images");
        }
        sprintf(g_game->imageOutputDirectory, "%s\\%s", ((char*)g_game->displayContext) + 0x628, buf);
    }
    ReadSettingIntOr("Movie Output Rate", &Game::movieOutputRate, 10, value);
    ReadSettingIntOr("textlines", &Game::textLines, 10, value);
    ReadSettingIntOr("textscroll", &Game::textScroll, 10, value);
    ReadSettingIntOr("mousespeed", &Game::mousespeed, 10, value);
    if (SettingGetInt((void*)"Total Annihilation", (void*)"gamespeed", &value) != 0) {
        g_game->gameSpeed = (unsigned short)value;
    } else {
        g_game->gameSpeed = 10;
    }
    g_game->effectiveGameSpeed = (short)(g_game->gameSpeed);
    if (SettingGetInt((void*)"Total Annihilation", (void*)"unitchat", &value) != 0) {
        g_game->unitChat = (unsigned char)value;
    } else {
        g_game->unitChat = 10;
    }
    if (SettingGetInt((void*)"Total Annihilation", (void*)"unitchattext", &value) != 0) {
        g_game->unitChatText = (unsigned char)value;
    } else {
        g_game->unitChatText = 5;
    }
    if (SettingGetInt((void*)"Total Annihilation", (void*)"musicmode", &value) != 0) {
        (*(struct MusicSettingFlags*)&g_game->flags_37f14_37f14).musicmode = value;
    } else {
        (*(struct MusicSettingFlags*)&g_game->flags_37f14_37f14).musicmode = 1;
    }
    if (SettingGetInt((void*)"Total Annihilation", (void*)"cdmode", &value) != 0) {
        g_game->cdMode = (unsigned char)value;
    } else {
        g_game->cdMode = 4;
    }
    if (SettingGetInt((void*)"Total Annihilation", (void*)"ackfx", &value) != 0) {
        (*(struct SoundFlags*)&g_game->soundFlags).ackfx = value;
    } else {
        (*(struct SoundFlags*)&g_game->soundFlags).ackfx = 1;
    }
    if (SettingGetInt((void*)"Total Annihilation", (void*)"buildfx", &value) != 0) {
        (*(struct SoundFlags*)&g_game->soundFlags).buildfx = value;
    } else {
        (*(struct SoundFlags*)&g_game->soundFlags).buildfx = 1;
    }
    if (SettingGetInt((void*)"Total Annihilation", (void*)"speechfx", &value) != 0) {
        (*(struct SoundFlags*)&g_game->soundFlags).speechfx = value;
    } else {
        (*(struct SoundFlags*)&g_game->soundFlags).speechfx = 1;
    }
    ReadSettingIntOr("fxvol", &Game::volume1, 0x1b, value);
    ReadSettingIntOr("musicvol", &Game::volume2, 0x20, value);
    if (SettingGetInt((void*)"Total Annihilation", (void*)"clock", &value) != 0) {
        g_game->uiOptionFlags_37f2f.clock = value;
    } else {
        g_game->uiOptionFlags_37f2f.clock = 0;
    }
    if (SettingGetInt((void*)"Total Annihilation", (void*)"NumSkirmishPlayers", &value) != 0) {
        if (value > 1 && value <= 10) {
            g_game->playerCount = value;
        } else {
            g_game->playerCount = value;
        }
    } else {
        g_game->playerCount = 4;
    }
    ReadSettingInt("MultiCommanderDeath", &Game::multiCommanderDeath, 1, value);
    ReadSettingInt("MultiMapping", &Game::multiMapping, 1, value);
    ReadSettingInt("MultiLineOfSight", &Game::multiLineOfSight, 1, value);
    ReadSettingInt("MultiLOSType", &Game::multiLOSType, 1, value);
    ReadSettingInt("SkirmishCommanderDeath", &SkirmishOptionsTable::commanderDeath, 1, value);
    ReadSettingInt("SkirmishMapping", &SkirmishOptionsTable::mapping, 1, value);
    ReadSettingInt("SkirmishLineOfSight", &SkirmishOptionsTable::lineOfSight, 1, value);
    ReadSettingInt("SkirmishLOSType", &SkirmishOptionsTable::losType, 1, value);
    if (SettingGetInt((void*)"Total Annihilation", (void*)"SkirmishDifficulty", &value) != 0) {
        g_game->options->difficulty = value & 0xffff;
    } else {
        g_game->options->difficulty = 1;
        SettingSetInt((void*)"Total Annihilation", (void*)"SkirmishDifficulty",
                     g_game->options->difficulty);
    }
    ReadSettingInt("SkirmishLocation", &SkirmishOptionsTable::fixedLocations, 1, value);
    value = 0x100;
    if (SettingGetString((void*)"Total Annihilation", (void*)"SkirmishMap",
                     g_game->options->mapName, &value) == 0) {
        g_seams.map->SetMissionType(2);
        g_seams.map->RefreshMapList(g_game->net, 0, 0);
        strncpy(g_game->options->mapName,
                g_seams.map->GetMissionName(g_game->net), 0x100);
        g_seams.map->SetMissionType(0);
        SettingSetString((void*)"Total Annihilation", (void*)"SkirmishMap", g_game->options->mapName);
    }
    for (i = 0; i < g_game->playerCount; i++) {
        sprintf(name, "Player%dController", i);
        ReadPlayerSetting(name, &SkirmishPlayer::controller, 0, value, i);
        sprintf(name, "Player%dSide", i);
        ReadPlayerSetting(name, &SkirmishPlayer::side, i % 2, value, i);
        sprintf(name, "Player%dColor", i);
        ReadPlayerSetting(name, &SkirmishPlayer::color, i, value, i);
        sprintf(name, "Player%dAllyGroup", i);
        ReadPlayerSetting(name, &SkirmishPlayer::allyGroup, 5, value, i);
        sprintf(name, "Player%dMetal", i);
        ReadPlayerSetting(name, &SkirmishPlayer::metal, 1000, value, i);
        sprintf(name, "Player%dEnergy", i);
        ReadPlayerSetting(name, &SkirmishPlayer::energy, 1000, value, i);
    }
    int ok26 = SettingGetInt((void*)"Total Annihilation", (void*)"PlayMovie", &value);
    if (ok26 != 0) {
        g_game->playMovie = value;
    } else {
        g_game->playMovie = 1;
    }
    int ok27 = SettingGetInt((void*)"Total Annihilation", (void*)"DisplaymodeDepth", &value);
    if (ok27 == 0) {
        value = 0;
    }
    if (value == 0x100) {
        int ok28 = SettingGetInt((void*)"Total Annihilation", (void*)"Games", &value);
        if (ok28 == 0) value=0;
        if (value == 1) g_game->uiOptionFlags_37f2f.bit1=1;
        else g_game->uiOptionFlags_37f2f.bit1=0;
    } else g_game->uiOptionFlags_37f2f.bit1=0;
setUiOptionDefaults:
    g_game->uiOptionFlags_37f2f.bit2 = 1;
    g_game->uiOptionFlags_37f2f.bit3 = 1;
    g_game->uiOptionFlags_37f2f.bit4 = 0;
    int ok29 = SettingGetInt((void*)"Total Annihilation", (void*)"AllMissions", &value);
    if (ok29 != 0) {
        g_game->allMissionsFlags.allMissions = value;
        return;
    }
    g_game->allMissionsFlags.allMissions = 0;
}

// FUNCTION: 0x430f00
void SaveSettings()
{
    char name[32];
    int i;

    SettingSetInt((void*)"Total Annihilation", (void*)"Interface Type", g_game->interfaceType);
    SettingSetInt((void*)"Total Annihilation", (void*)"DisplaymodeWidth", g_game->width_37f1b);
    SettingSetInt((void*)"Total Annihilation", (void*)"DisplaymodeHeight", g_game->height_37f1f);
    SettingSetInt((void*)"Total Annihilation", (void*)"side", g_game->flag_37ef2);
    SettingSetInt((void*)"Total Annihilation", (void*)"FixedLocations", g_game->options->fixedLocations);
    SettingSetInt((void*)"Total Annihilation", (void*)"scrollspeed", g_game->scrollSpeed);
    SettingSetInt((void*)"Total Annihilation", (void*)"SingleCommanderDeath", g_game->singleCommanderDeath);
    SettingSetInt((void*)"Total Annihilation", (void*)"SingleMapping", g_game->singleMapping);
    SettingSetInt((void*)"Total Annihilation", (void*)"SingleLineOfSight", g_game->singleLineOfSight);
    SettingSetInt((void*)"Total Annihilation", (void*)"SingleLOSType", g_game->singleLOSType);
    SettingSetInt((void*)"Total Annihilation", (void*)"screenchat", g_game->screenchat);
    SettingSetInt((void*)"Total Annihilation", (void*)"damagebars", (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).damagebars);
    SettingSetInt((void*)"Total Annihilation", (void*)"Sound Mode", (*(struct SoundFlags*)&g_game->soundFlags).soundMode);
    SettingSetInt((void*)"Total Annihilation", (void*)"RestoreVolume", (*(struct SoundFlags*)&g_game->soundFlags).restoreVolume);
    SettingSetInt((void*)"Total Annihilation", (void*)"MixingBuffers",
                 ((Sound*)g_game->sound)->GetMaxBuffers());
    if ((*(struct SoundFlags*)&g_game->soundFlags).restoreVolume) {
        SettingSetInt((void*)"Total Annihilation", (void*)"WaveOutVolume",
                     ((Sound*)g_game->sound)->QueryWaveVolume());
        SettingSetInt((void*)"Total Annihilation", (void*)"CDAudioVolume",
                     ((Sound*)g_game->sound)->QueryAuxVolume());
    }
    SettingSetInt((void*)"Total Annihilation", (void*)"Anti-Alias", (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).antiAlias);
    SettingSetInt((void*)"Total Annihilation", (void*)"Shadows", (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).shadows);
    SettingSetInt((void*)"Total Annihilation", (void*)"FeatureShadows", (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).featureShadows);
    SettingSetInt((void*)"Total Annihilation", (void*)"VehicleShadows", (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).vehicleShadows);
    SettingSetInt((void*)"Total Annihilation", (void*)"Shading", (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).shading);
    SettingSetInt((void*)"Total Annihilation", (void*)"DitheredFog", (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).ditheredFog);
    SettingSetInt((void*)"Total Annihilation", (void*)"Difficulty", g_game->difficulty);
    SettingSetInt((void*)"Total Annihilation", (void*)"Gamma", g_game->brightness);
    SettingSetInt((void*)"Total Annihilation", (void*)"SwitchAlt", (*(struct GraphicsSettingFlags*)&g_game->flags_37f06).switchAlt);
    SettingSetString((void*)"Total Annihilation", (void*)"Password", g_game->password_2be3_1176a);
    SettingSetString((void*)"Total Annihilation", (void*)"Nickname", g_game->nickname);
    SettingSetString((void*)"Total Annihilation", (void*)"Game Name", g_game->gameName_2bc1);
    if (g_game->valueSet) {
        SettingSetString((void*)"Total Annihilation", (void*)"Image Output Directory",
                     g_game->imageOutputDirectory);
        g_game->valueSet = 0;
    }
    if (g_game->valueSet_38c63) {
        SettingSetInt((void*)"Total Annihilation", (void*)"Movie Output Rate", g_game->movieOutputRate);
        g_game->valueSet_38c63 = 0;
    }
    SettingSetInt((void*)"Total Annihilation", (void*)"unitchat", g_game->unitChat);
    SettingSetInt((void*)"Total Annihilation", (void*)"unitchattext", g_game->unitChatText);
    SettingSetInt((void*)"Total Annihilation", (void*)"textlines", g_game->textLines);
    SettingSetInt((void*)"Total Annihilation", (void*)"textscroll", g_game->textScroll);
    SettingSetInt((void*)"Total Annihilation", (void*)"mousespeed", g_game->mousespeed);
    SettingSetInt((void*)"Total Annihilation", (void*)"gamespeed", g_game->gameSpeed);
    SettingSetInt((void*)"Total Annihilation", (void*)"clock", g_game->uiOptionFlags_37f2f.clock);
    SettingSetInt((void*)"Total Annihilation", (void*)"musicmode", g_game->musicMode & 1);
    SettingSetInt((void*)"Total Annihilation", (void*)"cdmode", g_game->cdMode);
    SettingSetInt((void*)"Total Annihilation", (void*)"ackfx", (*(struct SoundFlags*)&g_game->soundFlags).ackfx);
    SettingSetInt((void*)"Total Annihilation", (void*)"buildfx", (*(struct SoundFlags*)&g_game->soundFlags).buildfx);
    SettingSetInt((void*)"Total Annihilation", (void*)"speechfx", (*(struct SoundFlags*)&g_game->soundFlags).speechfx);
    SettingSetInt((void*)"Total Annihilation", (void*)"fxvol", g_game->volume1);
    SettingSetInt((void*)"Total Annihilation", (void*)"musicvol", g_game->volume2);
    SettingSetInt((void*)"Total Annihilation", (void*)"MultiCommanderDeath", g_game->multiCommanderDeath);
    SettingSetInt((void*)"Total Annihilation", (void*)"MultiMapping", g_game->multiMapping);
    SettingSetInt((void*)"Total Annihilation", (void*)"MultiLineOfSight", g_game->multiLineOfSight);
    SettingSetInt((void*)"Total Annihilation", (void*)"MultiLOSType", g_game->multiLOSType);
    SettingSetInt((void*)"Total Annihilation", (void*)"SkirmishCommanderDeath",
                 g_game->options->commanderDeath);
    SettingSetInt((void*)"Total Annihilation", (void*)"SkirmishMapping", g_game->options->mapping);
    SettingSetInt((void*)"Total Annihilation", (void*)"SkirmishLineOfSight",
                 g_game->options->lineOfSight);
    SettingSetInt((void*)"Total Annihilation", (void*)"SkirmishLOSType", g_game->options->losType);
    SettingSetInt((void*)"Total Annihilation", (void*)"SkirmishLocation", g_game->options->fixedLocations);
    SettingSetInt((void*)"Total Annihilation", (void*)"SkirmishDifficulty",
                 g_game->options->difficulty);
    SettingSetString((void*)"Total Annihilation", (void*)"SkirmishMap", g_game->options->mapName);
    for (i = 0; i < g_game->playerCount; i++) {
        sprintf(name, "Player%dController", i);
        SettingSetInt((void*)"Total Annihilation\\Skirmish", name,
                     g_game->options->players[i].controller);
        sprintf(name, "Player%dSide", i);
        SettingSetInt((void*)"Total Annihilation\\Skirmish", name, g_game->options->players[i].side);
        sprintf(name, "Player%dColor", i);
        SettingSetInt((void*)"Total Annihilation\\Skirmish", name, g_game->options->players[i].color);
        sprintf(name, "Player%dAllyGroup", i);
        SettingSetInt((void*)"Total Annihilation\\Skirmish", name,
                     g_game->options->players[i].allyGroup);
        sprintf(name, "Player%dMetal", i);
        SettingSetInt((void*)"Total Annihilation\\Skirmish", name, g_game->options->players[i].metal);
        sprintf(name, "Player%dEnergy", i);
        SettingSetInt((void*)"Total Annihilation\\Skirmish", name,
                     g_game->options->players[i].energy);
    }
    SettingSetInt((void*)"Total Annihilation", (void*)"PlayMovie", g_game->playMovie);
}

// FUNCTION: 0x431740
void ApplyUseOnlyUnits()
{
    TdfFile parser;
    char name[256];
    char* file = g_seams.map->GetNameSlot(g_game->net, 6);
    if (file == 0)
        return;
    if (!parser.LoadFile(file))
        return;
    {
        ProtectBlockReadWrite(g_game->defs);
        for (int i = 1; i < g_game->unitDefCount; i++)
            g_game->defs[i].flags1 &= 0xff7fffff;
        parser.ResetCurrentRecord();
        for (int j = 0; parser.SelectRecordAt(j); j++, parser.ResetCurrentRecord()) {
            parser.current->CopyRecordName(name, 0x100);
            for (int k = 0; k < g_game->unitDefCount; k++) {
                if (_strcmpi(g_game->defs[k].unitname, name) == 0) {
                    g_game->defs[k].flags1 |= 0x800000;
                    break;
                }
            }
        }
        ProtectBlockReadOnly(g_game->defs);
    }
}
