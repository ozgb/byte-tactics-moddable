#include "win_types.h"
#include "../state/block.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <vector>

#include "mission.h"
#include "../../interfaces/seams.h"

class StringRef;


#pragma pack(push, 1)
struct Slot_00436860 {
    int active;
    char unknown_4[0x14];
};

#include "../game/game.h"
#pragma pack(pop)

#include "../../link/globals.h"

#include "../network/packet_manager.h"


#include "../files/files_api.h"
#include "../gui/dialogs.h"

#include "../graphics/object_pool.h"

#include "../game/main.h"
#include "../graphics/palette.h"

#include "../util/registry.h"
int __stdcall GetLocalizedString(TdfFile* obj, char* buf, const char* key, int size, int def);

char* __stdcall Translate(char* text);

#include "map_list.h"

#include "../game/mission_conditions.h"

void WalkFrameChain(int*, int*, int, int, int*, int, int*, int*, int, int*);
void RegisterUnitOrders();

#include "meteors.h"

#include "../game/data_files.h"
void __stdcall ShowErrorBox(const char* text, const char* caption);
#include "../util/tdf.h"
int __stdcall GetLocalizedString(TdfFile* obj, char* buf, const char* key, int size, char* def);
void* __cdecl GameReallocTagged(void* p, const char* name, unsigned int size);
void __stdcall SetCursorMode(int n);
#include "../network/net_game.h"

static inline float GetFloat(TdfRecord* section, const char* key)
{
    double value = section->GetFieldDouble(key, 0.0);
    float result = (float)value;
    return result;
}

#include "../graphics/display.h"

int RIReport(int, int, int, int, int, int, int, int, int, int);

struct MissionRule {
    int type;
    int id;
    short x;
    short z;
};

#include "../debug/debug_lib.h"

#include "../util/vec3.h"

#include "../util/string_ref.h"
#include "../state/sim_globals.h"

struct Header_004373a0 {
    int magic;
    int width;
    int height;
    int unknown_0c;
    int plotOffset;
    int unknown_14;
    int unknown_18;
    int features;
    int featureOffset;
    char unknown_24[0x40 - 0x24];
};

#include "../util/math.h"

#include "map_cache_entry.h"

inline MapCacheEntry& MapCacheEntry::SetChecksum(Mission* self)
{
    tntChecksum = self->tntChecksum;
    return *this;
}

extern MapCacheEntry* g_mapCacheBegin;
extern MapCacheEntry* g_mapCacheEnd;

class MapCache : public std::vector<MapCacheEntry> {
public:
    void InsertMapCacheEntry(MapCacheEntry* pos, int count, const MapCacheEntry& val);
};

// FUNCTION: 0x434a30 _$E5
// FUNCTION: 0x434a60 _$E3
static MapCache s_mapCache;

// FUNCTION: 0x434ab0
void __stdcall SetMissionType(int owner)
{
    if (g_game->net != 0) {
        if (g_game->net->type == owner) {
            return;
        }
        delete g_game->net;
    }
    g_game->net = new (kSim) Mission(owner);
}

// FUNCTION: 0x434b90
void FreeOtaEnumCacheAndMission()
{
    if (g_otaEnumFileList) {
        GameFreeThunk(g_otaEnumFileList);
        g_otaEnumFileList = 0;
    }
    g_otaEnumCacheComplete = 0;
    g_otaEnumFileListBytes = 0;
    g_otaEnumFileCount = 0;
    delete g_game->net;
    g_game->net = 0;
}

// FUNCTION: 0x434bf0
int __stdcall LoadMapList(void** out, int param_2, int param_3)
{
    if (g_otaEnumFileList != 0) {
        if (out != 0) {
            if (param_3 != 0 && g_otaEnumCacheComplete == 0) {
                *out = g_otaEnumFileList;
                g_otaEnumFileList = 0;
            } else {
                char* p = (char*)GameAllocIgnoreTag("MULTI MAPS", g_otaEnumFileListBytes);
                *out = p;
                memcpy(p, g_otaEnumFileList, g_otaEnumFileListBytes);
            }
        }
        return g_otaEnumFileCount;
    }

    SetCursorMode(0x14);
    struct S { int count; int bFlag; int i; } s;
    
    if (param_2 == 0) {
        s.bFlag = 1;
        if (g_game->net->type != 3)
            s.bFlag = 0;
    } else {
        s.bFlag = 0;
    }
    int offset = 0;
    g_otaEnumFileListBytes = 1;
    g_otaEnumFileList = (char*)GameAllocIgnoreTag("MULTI MAPS", 1);
    *(char*)g_otaEnumFileList = 0;

    std::vector<StringRef> files;
    ListDirectory("Maps\\*.ota", 0, &files);
    g_otaEnumFileCount = 0;

    s.count = files.size();
    for (s.i = 0; s.i < s.count; s.i++) {
        struct A { char name[256]; char lower[256]; char path[256]; } a;
        BuildDataPath(a.path, "Maps", files[s.i].data, "OTA");
        TdfFile parser;
        if (parser.LoadFile(a.path) != 0
            && g_game->net->SelectSchema(3, &parser, 0) != 0) {
            strcpy(a.name, files[s.i].data);
            StripExtension(a.name);
            strcpy(a.lower, a.name);
            _strlwr(a.lower);
            char* src = Translate(a.lower);
            if (_strcmpi(src, a.lower) == 0)
                src = a.name;
            int len = strlen(src) + 1;
            g_otaEnumFileList = (char*)GameReallocTagged(g_otaEnumFileList, "MULTI MAPS",
                                               len + g_otaEnumFileListBytes);
            strcpy(g_otaEnumFileList + offset, src);
            g_otaEnumFileList[offset + len] = 0;
            offset += len;
            g_otaEnumFileListBytes += len;
            g_otaEnumFileCount++;
            if (param_2 != 0)
                break;
        }
        if (s.bFlag != 0) {
            HandleNetPackets();
            if (g_usePacketManager != 0)
                g_packetManager.SendAllQueued(0);
        }
    }
    SetCursorMode(0x13);
    if (g_otaEnumCacheComplete == 0)
        g_otaEnumCacheComplete = (param_2 == 0);
    int result = LoadMapList(out, param_2, param_3);
    return result;
}

// FUNCTION: 0x434f70
Mission::Mission(int owner_)
{
    ruleCount = 0;
    unitCount = 0;
    featureCount = 0;
    rules = 0;
    units = 0;
    features = 0;
    exists = 0;
    mapList = 0;
    mapCount = 0;
    multi = 0;
    briefing = 0;
    missionName[0] = 0;
    text_b14[0] = 0;
    type = owner_;
    LoadCampaign(DAT_005119b8);
}

// FUNCTION: 0x434ff0
Mission::~Mission()
{
    MissionConditions* obj = g_game->list_391ed;
    if (obj) {
        g_seams.players->FreeConditions(obj);
        operator delete(obj);
        g_game->list_391ed = 0;
    }
    if (units)
        GameFreeThunk(units);
    units = 0;
    unitCount = 0;
    if (rules)
        GameFreeThunk(rules);
    rules = 0;
    ruleCount = 0;
    if (features)
        GameFreeThunk(features);
    features = 0;
    featureCount = 0;
    if (briefing) {
        GameFreeThunk(briefing);
        briefing = 0;
    }
    if (mapList)
        GameFreeThunk(mapList);
    if (features)
        GameFreeThunk(features);
    if (rules)
        GameFreeThunk(rules);
    if (units)
        GameFreeThunk(units);

}

// FUNCTION: 0x435100
int Mission::GetGameType()
{
    return *(int*)this;
}

// FUNCTION: 0x435110
void Mission::LoadCampaign(char* file)
{
    char msg[0x80];

    list.Unload();
    strcpy(campaign, file);
    for (int i = 0; i < 9; i++)
        SetNameSlot(i, DAT_005119b8);
    if (strlen(file) != 0) {
        BuildCampaignFilePath(0, "camps", campaign, "TDF");
        if (strlen(file) != 0) {
            if (!list.LoadFile(GetNameSlot(0))) {
                sprintf(msg, "The requested campaign file, %s, does not exist.", GetNameSlot(0));
                OpenMessageBox((Gui *)g_game->messages, msg, 0x1e0, 1, 1);
                LoadCampaign(DAT_005119b8);
                return;
            }
        }
        tntChecksum = 0;
        missionIndex = 0;
        LoadMission(0);
    }
}

// FUNCTION: 0x4352b0
char* Mission::GetCampaignName()
{
    char* ptr = campaign;
    if (strlen(ptr) > 0) {
        return ptr;
    }
    return 0;
}

// FUNCTION: 0x435320
void Mission::LoadBriefing()
{
    if (briefing)
        GameFreeThunk(briefing);
    char* name = strlen(names[2]) > 0 ? names[2] : 0;
    if (name == 0) {
        briefing = 0;
        return;
    }
    int size = HAPI_FileLengthByName(name);
    if (size != 0) {
        briefing = (char*)SimAllocTagged("Briefing", size + 1);
        HAPI_ReadFileAt(name, briefing, 0, size);
        briefing[size] = 0;
    }
}

// FUNCTION: 0x4353a0
char* Mission::GetBriefing()
{
    return briefing;
}

// FUNCTION: 0x4353b0
void Mission::SetNameSlot(int index, char* text)
{
    strcpy(names[index], text);
    if (index == 1) {
        if (strlen(text) != 0)
            exists = HAPI_FileLengthByName(text);
        else
            exists = 0;
    }
}

// FUNCTION: 0x435430
void Mission::BuildCampaignFilePath(int index, char* dir, char* name, char* ext)
{
    char path[256];
    if (strlen(name) == 0) {
        SetNameSlot(index, DAT_005119b8);
        return;
    }
    char* side = (char*)GetPreferredLanguage();
    if (side) {
        sprintf(path, "%s-%s\\%s", dir, side, name);
        StripExtension(path);
        strcat(path, ".");
        strcat(path, ext);
        FileHandle* file = HAPI_OpenFileRead(path);
        if (file) {
            HAPI_CloseFile(file);
            SetNameSlot(index, path);
            return;
        }
    }
    sprintf(path, "%s\\%s", dir, name);
    StripExtension(path);
    strcat(path, ".");
    strcat(path, ext);
    SetNameSlot(index, path);
}

// FUNCTION: 0x4356c0
char* Mission::GetNameSlot(int index)
{
    char* ptr = names[index];
    if (strlen(ptr) > 0)
        return ptr;
    return 0;
}

// FUNCTION: 0x4356f0
int Mission::CountMissions()
{
    char buf[128];
    if (strlen(campaign) == 0)
        return 0;
    int n = 0;
    while (1) {
        sprintf(buf, "MISSION%d", n);
        list.ResetCurrentRecord();
        if (list.SelectRecord(buf) == 0)
            break;
        n++;
    }
    return n;
}

// FUNCTION: 0x435760
int Mission::BuildMissionList(char** out)
{
    char buf[128];
    char temp[256];
    if (strlen(campaign) == 0)
        return 0;
    int n;
    if (strlen(&campaign[0]) == 0) {
        n = 0;
    } else {
        int m = 0;
        while (1) {
            sprintf(buf, "MISSION%d", m);
            list.ResetCurrentRecord();
            if (list.SelectRecord(buf) == 0)
                break;
            m++;
        }
        n = m;
    }
    if (n != 0) {
        char* p = (char*)GameAllocIgnoreTag("MissionList", n << 8);
        *out = p;
        *p = 0;
        for (int i = 0; i < n; i++) {
            sprintf(buf, "MISSION%d", i);
            list.ResetCurrentRecord();
            if (list.SelectRecord(buf) == 0)
                return 0;
            if (GetLocalizedString(&list, temp, "missionname", 0x100, 0) != 0)
                strcpy(SkipTextLines(*out, i), temp);
            else
                strcpy(SkipTextLines(*out, i), "Error -- Unnamed Mission");
        }
    }
    return n;
}

// FUNCTION: 0x4358f0
int Mission::GetTerrainLength()
{
    return exists;
}

// FUNCTION: 0x435900
char* Mission::GetDescription()
{
    return description;
}

// FUNCTION: 0x435910
char* Mission::GetPlanet()
{
    return planet;
}

// FUNCTION: 0x435920
int Mission::GetTerrainSizeTier()
{
    int v = exists;
    if (v < 0x3e6666)
        return 0x10;
    if (v < 0x600000)
        return 0x18;
    if (v < 0x800000)
        return 0x20;
    if (v < 0xa00000)
        return 0x30;
    if (v < 0xc00000)
        return 0x40;
    return 0x80;
}

// FUNCTION: 0x435980
int Mission::MissionExists(int index)
{
    char buf[128];
    int n;
    if (strlen(campaign) == 0) {
        n = 0;
    } else {
        n = 0;
        sprintf(buf, "MISSION%d", n);
        list.ResetCurrentRecord();
        while (list.SelectRecord(buf)) {
            n++;
            sprintf(buf, "MISSION%d", n);
            list.ResetCurrentRecord();
        }
    }
    return n > index;
}

// FUNCTION: 0x435a20
int Mission::LoadMissionByName(char* map)
{
    int res;

    tntChecksum = 0;
    if (type != 1) {
        if (type > 1 && type <= 3) {
            res = LoadMission(map);
            if (res && GetPreferredLanguage() && _strcmpi((char*)GetPreferredLanguage(), "english")) {
                char lower[200];
                strcpy(lower, map);
                _strlwr(lower);
                strncpy(text_b14, Translate(lower), 0xff);
                if (_strcmpi(text_b14, map) == 0)
                    strcpy(text_b14, map);
            } else {
                strcpy(text_b14, map);
            }
            return res;
        }
    } else {
        int count = BuildMissionList((char**)&res);
        if (count > 0) {
            char* p = (char*)res;
            for (int i = 0; i < count; i++) {
                if (_strcmpi(p, map) == 0) {
                    GameFreeThunk((void*)res);
                    tntChecksum = 0;
                    missionIndex = i;
                    return LoadMission(0);
                }
                p += strlen(p) + 1;
            }
            GameFreeThunk((void*)res);
        }
    }
    return 0;
}

// FUNCTION: 0x435c00
int Mission::SelectMission(int index)
{
    tntChecksum = 0;
    missionIndex = index;
    return LoadMission(0);
}

// FUNCTION: 0x435c20
char* Mission::GetTranslatedName()
{
    return text_b14;
}

// FUNCTION: 0x435c30
char* Mission::GetMissionName()
{
    return missionName;
}

// FUNCTION: 0x435c40
bool Mission::HasMissionName()
{
    return missionName[0] != 0;
}

// FUNCTION: 0x435c50
int Mission::GetMissionIndex()
{
    return missionIndex;
}

// FUNCTION: 0x435c60
int Mission::AdvanceMission()
{
    char buf[128];
    int n;
    int index = missionIndex + 1;
    if (strlen(campaign) == 0) {
        n = 0;
    } else {
        n = 0;
        sprintf(buf, "MISSION%d", n);
        list.ResetCurrentRecord();
        while (list.SelectRecord(buf)) {
            n++;
            sprintf(buf, "MISSION%d", n);
            list.ResetCurrentRecord();
        }
    }
    if (n > index) {
        tntChecksum = 0;
        missionIndex++;
        LoadMission(0);
        return 1;
    }
    return 0;
}

// FUNCTION: 0x435d30
void Mission::RefreshMapList(int multiplayer)
{
    if (multi != 0 && multiplayer == 0 && mapList != 0) {
        GameFreeThunk((int*)mapList);
        mapList = 0;
    }
    if (mapList == 0) {
        mapCount = LoadMapList((void **)&mapList, multiplayer, multiplayer);
    }
    LoadMissionByName(mapList);
    multi = multiplayer;
}

// FUNCTION: 0x435da0
int Mission::LoadMission(char* map)
{
    TdfFile parser;
    char schema[0x20];
    char value[0x100];
    char path[0x100];
    MeteorParams meteor;
    char desc[0x80];
    char lower[0x80];

    g_seams.players->DeleteConditions(g_game->list_391ed);
    g_game->list_391ed = new (kSim) MissionConditions;
    surfaceMetal = -1;
    minWindSpeed = -1;
    maxWindSpeed = -1;
    gravity = -1;
    tidalStrength = -1.0f;
    lavaWorld = 0;
    noSeaLevelTrigger = 0;
    planet[0] = 0;
    description[0] = 0;
    if (units)
        GameFreeThunk(units);
    units = 0;
    unitCount = 0;
    if (rules)
        GameFreeThunk(rules);
    rules = 0;
    ruleCount = 0;
    if (features)
        GameFreeThunk(features);
    features = 0;
    featureCount = 0;
    if (briefing) {
        GameFreeThunk(briefing);
        briefing = 0;
    }

    switch (type) {
    case 1: {
        char key[0x100];
        sprintf(key, "MISSION%d", missionIndex);
        list.ResetCurrentRecord();
        if (!list.SelectRecord(key)) {
            char msg[0x100];
            sprintf(msg, "The requested mission file, %s, does not exist.", key);
            OpenMessageBox((Gui *)g_game->messages, msg, 0x1e0, 1, 1);
            return 0;
        }
        GetLocalizedString(&list, missionName, "missionname", 0x100, 0);
        int found = list.current->GetFieldString(path, "missionfile", 0x100, DAT_005119b8);
        if (found) {
            char file[0x100];
            BuildDataPath(file, "Maps", path, "OTA");
            if (!parser.LoadFile(file)) {
                char msg[0x100];
                sprintf(msg, "Hey, joker!  There is no mission defintion for this mission: %s", path);
                OpenMessageBox((Gui *)g_game->messages, msg, 0x1e0, 1, 1);
                return 0;
            }
            parser.ResetCurrentRecord();
            if (parser.SelectRecord("GlobalHeader")) {
                (*(short*)&g_game->maxUnitsSetting) = parser.current->GetFieldInt("maxunits", 200);
                parser.ResetCurrentRecord();
                BuildCampaignFilePath(1, "Maps", path, "TNT");
            } else {
                char msg[0x100];
                sprintf(msg, "Hey, joker!  Mission file %s is corrupt (no header found).", path);
                OpenMessageBox((Gui *)g_game->messages, msg, 0x1e0, 1, 1);
                return 0;
            }
        } else {
            OpenMessageBox((Gui *)g_game->messages, "Old TED format no longer supported!", 0x1e0, 1, 1);
            return 0;
        }
        break;
    }
    case 2:
    case 3:
        exists = 0;
        strcpy(missionName, map);
        BuildDataPath(path, "Maps", map, "OTA");
        if (parser.LoadFile(path)) {
            BuildCampaignFilePath(1, "Maps", map, "TNT");
            break;
        }
        map = (char *)FindTranslation(map);
        if (map == 0)
            return 0;
        strcpy(missionName, map);
        BuildDataPath(path, "Maps", map, "OTA");
        if (!parser.LoadFile(path))
            return 0;
        BuildCampaignFilePath(1, "Maps", map, "TNT");
        break;
    case 0:
        return 0;
    default:
        return 0;
    }

    if (!parser.SelectRecord("GlobalHeader")) {
        OpenMessageBox((Gui *)g_game->messages, "No GlobalHeader block in mission file!", 0x1e0, 1, 1);
        return 0;
    }
    headerChecksum = *(int*)((char*)parser.current + 0x25);
    GetLocalizedString(&parser, value, "brief", 0x100, DAT_005119b8);
    BuildCampaignFilePath(2, "camps\\briefs", value, "TXT");
    LoadBriefing();
    GetLocalizedString(&parser, value, "narration", 0x100, DAT_005119b8);
    BuildCampaignFilePath(3, "camps\\briefs", value, "WAV");
    GetLocalizedString(&parser, value, "missionhint", 0x100, DAT_005119b8);
    BuildCampaignFilePath(4, "camps\\hints", value, "TXT");
    parser.current->GetFieldString(value, "glamour", 0x100, DAT_005119b8);
    BuildCampaignFilePath(5, DAT_005119b8, value, "PCX");
    parser.current->GetFieldString(value, "glamoursound", 0x100, DAT_005119b8);
    BuildCampaignFilePath(8, "camps\\briefs", value, "WAV");
    parser.current->GetFieldString(value, "UseOnlyUnits", 0x100, DAT_005119b8);
    BuildCampaignFilePath(6, "camps\\useonly", value, "TDF");
    g_game->singleMapping = parser.current->GetFieldInt("mapping", 0);
    g_game->singleLineOfSight = parser.current->GetFieldInt("lineofsight", 0);
    g_game->singleLOSType = 1;
    g_game->singleCommanderDeath = 0;
    parser.current->GetFieldString(memory, "memory", 0x80, DAT_005119b8);
    parser.current->GetFieldString(numPlayers, "numplayers", 0x80, DAT_005119b8);
    parser.current->GetFieldString(planet, "Planet", 0x80, DAT_005119b8);
    g_game->skip = parser.current->GetFieldInt("nomovie", 0);
    parser.current->GetFieldString(desc, "missiondescription", 0x80, "No description available");
    strcpy(lower, desc);
    _strlwr(lower);
    strcpy(description, Translate(lower));
    if (_strcmpi(description, lower) == 0)
        strcpy(description, desc);
    minWindSpeed = parser.current->GetFieldInt("minwindspeed", 0);
    maxWindSpeed = parser.current->GetFieldInt("maxwindspeed", 0);
    gravity = parser.current->GetFieldInt("gravity", 0);
    tidalStrength = GetFloat(parser.current, "tidalstrength");
    lavaWorld = parser.current->GetFieldInt("lavaworld", 0);
    noSeaLevelTrigger = parser.current->GetFieldInt("nosealeveltrigger", 0);
    waterDoesDamage = parser.current->GetFieldInt("waterdoesdamage", 0);
    waterDamage = parser.current->GetFieldInt("waterdamage", 0);
    g_seams.players->RegisterConditions(g_game->list_391ed, 0, &parser);
    killMul = GetFloat(parser.current, "killmul");
    timeMul = GetFloat(parser.current, "timemul");
    if (!SelectSchema(type, &parser, schema)) {
        ShowErrorBox("No suitable schema type in mission file!", "Map error");
        return 0;
    }
    startMetal[0] = (float)parser.current->GetFieldInt("HumanMetal", 0);
    startEnergy[0] = (float)parser.current->GetFieldInt("HumanEnergy", 0);
    startMetal[1] = (float)parser.current->GetFieldInt("ComputerMetal", 0);
    startEnergy[1] = (float)parser.current->GetFieldInt("ComputerEnergy", 0);
    surfaceMetal = parser.current->GetFieldInt("SurfaceMetal", 0);
    parser.current->GetFieldString(value, "aiprofile", 0x100, DAT_005119b8);
    BuildCampaignFilePath(7, "ai", value, "txt");
    if (!GetNameSlot(7))
        BuildCampaignFilePath(7, "ai", "default", "txt");
    parser.current->GetFieldString(meteor.name, "MeteorWeapon", 0x20, DAT_005119b8);
    if (strlen(meteor.name) != 0) {
        meteor.radius = parser.current->GetFieldInt("MeteorRadius", 0);
        meteor.density = GetFloat(parser.current, "MeteorDensity");
        meteor.duration = GetFloat(parser.current, "MeteorDuration");
        meteor.interval = GetFloat(parser.current, "MeteorInterval");
        if (meteor.radius == 0 || meteor.density == 0.0f || meteor.duration == 0.0f || meteor.interval == 0.0f)
            g_seams.wind_meteors->LoadMeteorDefaults(&meteor);
        g_seams.wind_meteors->EnableMeteors();
    } else {
        g_seams.wind_meteors->DisableMeteors();
        g_seams.wind_meteors->LoadMeteorDefaults(&meteor);
    }
    g_seams.wind_meteors->SetMeteorParams(&meteor);
    LoadMissionData(schema, &parser);
    return 1;
}

// FUNCTION: 0x436860
int Mission::SelectSchema(int type, TdfFile* parser, char* schema)
{
    int order[4];
    char name[0x10];
    char what[0x10];
    char kind[0x20];
    char* names[7];
    memset(order, -1, sizeof(order));
    names[0] = "Easy";
    names[1] = "Medium";
    names[2] = "Hard";
    names[3] = "Network 1";
    names[4] = "Network 2";
    names[5] = "Network 3";
    names[6] = "Network 4";
    int players = g_game->numPlayers;
    switch (type) {
    case 1:
        switch (g_game->difficulty) {
        case 0:
            order[0] = 0;
            order[1] = 1;
            order[2] = 2;
            break;
        case 1:
            order[0] = 1;
            order[1] = 0;
            order[2] = 2;
            break;
        case 2:
            order[0] = 2;
            order[1] = 1;
            order[2] = 0;
            break;
        default:
            return 0;
        }
        break;
    case 3: {
        for (int i = 0; i < 10; i++) {
            if (g_game->table_2c28[i] != -1 && g_game->table_2c28[i] != 0)
                players = i + 1;
        }
        order[0] = 3;
        order[1] = 4;
        order[2] = 5;
        order[3] = 6;
        break;
    }
    case 2: {
        for (int i = 0; i < 10; i++) {
            if (((Slot_00436860 *)g_game->options)[i].active != 0)
                players = i + 1;
        }
        order[0] = 3;
        order[1] = 4;
        order[2] = 5;
        order[3] = 6;
        break;
    }
    case 0:
        return 0;
    default:
        return 0;
    }

    int bestCount = 0;
    int found = 0;
    TdfRecord* best = 0;
    for (int k = 0; k < 4; k++) {
        int d = order[k];
        if (d == -1)
            break;
        for (int n = 0; ; n++) {
            parser->ResetCurrentRecord();
            if (!parser->SelectRecord("GlobalHeader"))
                FatalError("Very bad news!  No MSG!");
            sprintf(name, "Schema %i", n);
            if (!parser->SelectRecord(name))
                break;
            if (!parser->current->GetFieldString(kind, "type", 0x20, DAT_005119b8))
                continue;
            if (_strcmpi(kind, names[d]) != 0)
                continue;
            if (type != 3 && type != 2 || schema == 0) {
                if (schema)
                    strcpy(schema, name);
                return 1;
            }
            TdfRecord* section = parser->current;
            int count = 0;
            if (parser->SelectRecord("specials")) {
                TdfRecord* specials = parser->current;
                TdfRecord* s;
                for (int i = 0; (s = specials->GetSubRecord(i)) != 0; i++) {
                    if (s->GetFieldString(what, "specialwhat", 0x10, DAT_005119b8)) {
                        static int len = strlen("StartPos");
                        if (_strnicmp(what, "StartPos", len) == 0)
                            count++;
                    }
                }
            }
            if (count != 0 && (count == players || players == 0 || (count > bestCount && bestCount != players))) {
                best = section;
                bestCount = count;
                found = 1;
                strcpy(schema, name);
            }
        }
    }
    if (best)
        parser->current = best;
    return found;
}

// FUNCTION: 0x436c30
void Mission::LoadMissionData(char* name, TdfFile* parser)
{
    char text[0x100];
    char buf[0x400];
    int i;
    int count;
    int total = 0;

    parser->ResetCurrentRecord();
    if (!parser->SelectRecord("globalheader"))
        return;
    if (!parser->SelectRecord(name))
        return;
    TdfRecord* root = parser->current;

    TdfRecord* list = root->FindSubRecord("units");
    if (list)
        count = list->GetSubRecordCount();
    else
        count = 0;
    for (i = 0; i < count; i++) {
        TdfRecord* s = list->GetSubRecord(i);
        if (s->GetFieldString(buf, "Unitname", 0x400, DAT_005119b8))
            total += strlen(buf) + 1;
        if (s->GetFieldString(buf, "Ident", 0x400, DAT_005119b8))
            total += strlen(buf) + 1;
        if (s->GetFieldString(buf, "InitialMission", 0x400, DAT_005119b8))
            total += strlen(buf) + 1;
    }
    int unitBytes = count * sizeof(MissionUnit);
    unitCount = count;
    units = (MissionUnit*)SimAllocTagged(
        "MISSIONUNIT DATA",
        (total / 0x400 + 2) * 0x400 + unitBytes);
    char* strings = (char*)units.get() + unitBytes;
    for (i = 0; i < count; i++) {
        MissionUnit* u = &units[i];
        TdfRecord* s = list->GetSubRecord(i);
        if (s->GetFieldString(strings, "Unitname", 0x400, DAT_005119b8)) {
            u->name = strings;
            strings += strlen(strings) + 1;
        } else {
            u->name = 0;
        }
        if (s->GetFieldString(strings, "Ident", 0x400, DAT_005119b8)) {
            u->ident = strings;
            strings += strlen(strings) + 1;
        } else {
            u->ident = 0;
        }
        if (s->GetFieldString(strings, "InitialMission", 0x400, DAT_005119b8)) {
            u->initialMission = strings;
            strings += strlen(strings) + 1;
        } else {
            u->initialMission = 0;
        }
        u->x = s->GetFieldInt("XPos", 0) << 16;
        u->y = s->GetFieldInt("YPos", 0) << 16;
        u->z = s->GetFieldInt("ZPos", 0) << 16;
        u->angle = (s->GetFieldInt("Angle", 0) << 16) / 360;
        u->player = s->GetFieldInt("Player", 0);
        if (!u->player)
            u->player = 1;
        u->health = s->GetFieldInt("HealthPercentage", 100);
        u->buildPriority = s->GetFieldInt("BuildPriority", 0);
        u->creationCountdown = s->GetFieldInt("CreationCountdown", 0);
        u->missionCritical = s->GetFieldInt("MissionCriticalUnit", 0);
        u->aiIgnore = s->GetFieldInt("AiIgnore", 0);
        u->aiPriorityTarget = s->GetFieldInt("AiPriorityTarget", 0);
        u->initialGroup = s->GetFieldInt("InitialGroup", 0);
        u->immunity = s->GetFieldInt("Immunity", 0);
    }

    list = root->FindSubRecord("specials");
    if (list)
        count = list->GetSubRecordCount();
    else
        count = 0;
    ruleCount = count;
    rules = (MissionRule*)SimAllocTagged("MISSIONRULE DATA", count * sizeof(MissionRule));
    int startPos = 0;
    for (i = 0; i < count; i++) {
        MissionRule* r = &rules[i];
        r->type = 0;
        TdfRecord* s = list->GetSubRecord(i);
        if (s->GetFieldString(text, "specialwhat", 0x100, DAT_005119b8)) {
            static int len = strlen("StartPos");
            if (_strnicmp(text, "StartPos", len) == 0) {
                r->type = 1;
                r->x = s->GetFieldInt("XPos", 0);
                r->z = s->GetFieldInt("ZPos", 0);
                int id;
                if (!isdigit(text[len]))
                    id = ++startPos;
                else
                    id = atoi(text + len);
                r->id = id;
                if (id > 0)
                    r->id = id - 1;
            }
        }
    }

    list = root->FindSubRecord("features");
    if (list)
        count = list->GetSubRecordCount();
    else
        count = 0;
    featureCount = count;
    if (features)
        GameFreeThunk(features);
    features = (MissionFeature*)SimAllocTagged("MISSIONFEATURE DATA", count * sizeof(MissionFeature));
    for (i = 0; i < count; i++) {
        MissionFeature* f = &features[i];
        TdfRecord* s = list->GetSubRecord(i);
        if (!s->GetFieldString(f->name, "Featurename", 0x80, DAT_005119b8))
            f->name[0] = 0;
        f->x = s->GetFieldInt("XPos", -1);
        f->y = s->GetFieldInt("ZPos", -1);
        if (f->x < 0 || f->y < 0)
            f->name[0] = 0;
    }
}

// FUNCTION: 0x437280
void Mission::FreeMissionData()
{
    if (units)
        GameFreeThunk(units);
    units = 0;
    unitCount = 0;
    if (rules)
        GameFreeThunk(rules);
    rules = 0;
    ruleCount = 0;
    if (features)
        GameFreeThunk(features);
    features = 0;
    featureCount = 0;
    if (briefing) {
        GameFreeThunk(briefing);
        briefing = 0;
    }
}

// FUNCTION: 0x437300
int Mission::CountStartPositions()
{
    int n = 0;
    for (int i = 0; i < ruleCount; i++) {
        if (rules[i].type == 1) {
            n++;
        }
    }
    return n;
}

// FUNCTION: 0x437320
int Mission::GetStartPosition(Vec3* out, int id)
{
    for (int i = 0; i < ruleCount; i++) {
        if (rules[i].type == 1 && rules[i].id == id) {
            out->x = rules[i].x << 16;
            out->z = rules[i].z << 16;
            out->y = 0;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: 0x4373a0
int Mission::ComputeMapChecksum()
{
    if (tntChecksum != 0) {
        return headerChecksum ^ tntChecksum;
    }
    char* name = GetNameSlot(1);
    MapCacheEntry* it;
    for (it = g_mapCacheBegin; it != g_mapCacheEnd; it++) {
        if (_strcmpi(it->handle.data, name) == 0) {
            tntChecksum = it->tntChecksum;
            return headerChecksum ^ tntChecksum;
        }
    }
    FileHandle* file = HAPI_OpenFileRead(name);
    if (file == 0) {
        return 0;
    }
    Header_004373a0 header;
    HAPI_readfromfile(file, &header, 0x40);
    if (header.magic != 0x2000) {
        return 0;
    }
    tntChecksum = tntChecksum ^ ComputeChecksum((unsigned char*)&header, 0x40);
    int size = header.width * header.height * 4;
    void* data = GameAllocIgnoreTag("Raw Plot Data", size);
    HAPI_SeekFile(file, header.plotOffset);
    HAPI_readfromfile(file, data, size);
    tntChecksum = tntChecksum ^ ComputeChecksum((unsigned char*)data, size);
    GameFreeThunk(data);
    size = header.features * 0x84;
    if (size > 0) {
        data = GameAllocIgnoreTag("Raw Feature Data", size);
        HAPI_SeekFile(file, header.featureOffset);
        HAPI_readfromfile(file, data, size);
        tntChecksum = tntChecksum ^ ComputeChecksum((unsigned char*)data, size);
        GameFreeThunk(data);
    }
    HAPI_CloseFile(file);
    s_mapCache.InsertMapCacheEntry(g_mapCacheEnd, 1, MapCacheEntry(StringRef(name)).SetChecksum(this));
    return headerChecksum ^ tntChecksum;
}
