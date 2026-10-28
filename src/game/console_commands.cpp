#define WIN32_LEAN_AND_MEAN
#include "win_types.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include "../../interfaces/host.h"
#include "../../interfaces/ref.h"
#include <wctype.h>

#pragma pack(push, 1)

#include "../util/vec3.h"

struct GafFrame;
struct Script;
struct UnitMotion;
struct PathOrder;
struct MissionOrder;
struct TdfParser;

#include "../orders/mission_type.h"

#include "../network/player.h"
#include "../network/player_info.h"

struct Movement {
    char unknown_0[0x10];
    int width;
    char unknown_14[4];
    Ref<unsigned int> states;
};

struct UnitDef;
#include "../units/unit_def.h"

#include "../units/unit.h"

struct Tile {
    unsigned short unit;
    unsigned short feature;
    unsigned char height;
    char unknown_5[2];
    unsigned char metal;
    unsigned short object;
    char unknown_a[2];
    unsigned char flags;
};

#include "../ai/pathfinder.h"

#include "../graphics/memory_cache.h"

#include "mission_conditions.h"
struct AccountList;

struct GameFlags {
    unsigned short damagebars : 1;
    unsigned short antiAlias : 1;
    unsigned short shadows : 1;
    unsigned short vehicleShadows : 1;
    unsigned short featureShadows : 1;
    unsigned short shading : 1;
    unsigned short ditheredFog : 1;
    unsigned short unused7 : 1;
    unsigned short switchAlt : 1;
    unsigned short rest : 7;
};

struct ViewFlags {
    unsigned short bit0 : 1;
    unsigned short bit1 : 1;
    unsigned short bit2 : 1;
    unsigned short rest : 13;
};

#include "../game/game.h"

#pragma pack(pop)

class Sound {
public:
    void PlayCdTrack(int index, int flag);
    int Is3DEnabled();
    void Disable3D();
    void Enable3D();
    void SetTrackCategory(int);
    int StopCdAudio();
    int GetTrackCount();
    int GetTrackCategory();
    int IsFirstTrackData();
    int GetCurrentTrack();
    int IsCdPlaying();
    int GetPlayState();
};

int ScanDirectory(char*, char*);

struct Display_00417a60 {
    char unknown_0[0xf0];
    unsigned short bit0 : 1;
    unsigned short windowed : 1;
    unsigned short bit2_15 : 14;
};

struct Pos_00417bb0 {
    unsigned short x_frac;
    short x;
    unsigned short y_frac;
    short y;
    unsigned short z_frac;
    short z;
};

#include "../ai/ai_search_goal.h"

class Iface_00417f30 {
public:
    virtual void Method_00();
    virtual void Method_04();
    virtual void Method_08();
    virtual void Method_0c();
    virtual void Method_10();
    virtual void Method_14();
    virtual void Method_18();
    virtual void Method_1c();
    virtual void Method_20();
    virtual void Method_24();
    virtual void Method_28(int ctx);
};

struct Point_00417f60 {
    int x;
    int y;
};

#include "../graphics/rect.h"
#include "../state/sim_globals.h"

#include "../network/net_stats.h"

#include "console_commands.h"
#include "console_commands.h"
#include "share_commands.h"
#include "share_commands.h"

// GLOBAL: 0x511de8
extern Game* g_game;
extern char g_commandLineBuf[];
extern int g_messageBytesReceived;
extern int g_messageBytesSent;
extern int g_packetBytesSent;
extern int DAT_00511c20;
extern int g_packetsSent;
extern int g_packetsReceived;
extern int g_packetBytesReceived;
extern int g_contourSpacing;
extern int g_contourOffset;
extern int g_cmdSingEnabled;
extern int g_messageCountByType[45][2];
extern int g_messageBytesByType[45][2];
extern unsigned char g_contourColors[];
extern unsigned char g_movementClassColors[];
extern signed char g_dirDeltaX[], g_dirDeltaZ[];

void __stdcall AddMessage(char* text, int key, int value, int last);
#include "../map/features.h"
void* __stdcall PlaceFeature(void* target, unsigned short id, void* pos, void* active,
                             unsigned char owner);
void __stdcall IssueOrderToSelection(void* a, int b, MissionType kind, int d, int e, int f);
#include "settings.h"
#include "../graphics/model_render.h"
void __stdcall SetBrightness(float value);
#include "../files/files_api.h"
#include "settings_campaign.h"
#include "../ingame/control_panel.h"
#include "../network/net_stats.h"
void __stdcall WriteScreenshot(char* name, char* description, int x, int y, int w, int h);
unsigned int GetTicks();
#include "../map/meteors.h"
#include "../util/commands.h"
#include "../graphics/object_pool.h"
extern Display_00417a60* GetDisplay();
#include "../graphics/display.h"
#include "../debug/debug_lib.h"
void __stdcall DrawLine(void* surface, int x1, int y1, int x2, int y2, int color);
void __stdcall DrawMapTileSelectionOutline(void* a, short* b, int c, int d);
void __stdcall SetFont(int font);
#include "../graphics/draw.h"
void __stdcall SetTextColors(int color, int background);
void __stdcall DrawString(void* surface, const char* text, int x, int y, int maxWidth);
void __stdcall FillPolygon(void* surface, Point_00417f60* points, int count, int color);
void __stdcall FillRectangle(void* surface, Rect* rect, int color);
void __stdcall DrawCellContours(void* surface, Point_00417f60* corners, unsigned char* heights);
Unit* __stdcall FindNextSelectedUnit(int, int);

// FUNCTION: 0x416240
void __stdcall CmdMemDump(char* args)
{
    void* file = host_file_create("memdump.txt", 0);
    if (file != 0) {
        host_file_close(file);
    }
}

// FUNCTION: 0x416270
void __stdcall CmdMem(int)
{
}

// FUNCTION: 0x416280
void __stdcall CmdAI(CommandArgs* args)
{
    unsigned char i = args->GetIntArg(1, g_game->localPlayer);
    if (i < 10) {
        Player* p = &g_game->players[i];
        if (p->active != 0 && (p->type == 1 || p->type == 2 || p->type == 3)
            && p->index != 10) {
            Player* q = &g_game->players[i];
            if (q->active != 0 && q->type == 1)
                q->SetType(2);
            else
                q->SetType(1);
        }
    }
}

// FUNCTION: 0x416310
void __stdcall CmdAssign(CommandArgs* args)
{
    MissionType kind(args->GetArg(1, DAT_005119b8));
    if (kind.index) {
        int a = args->GetIntArg(1, 0);
        int b = args->GetIntArg(2, 0);
        IssueOrderToSelection(g_game->orders_2c76, 0, kind, 0, a, b);
    }
}

// FUNCTION: 0x416370
void __stdcall CmdAssert(int)
{
}

// FUNCTION: 0x416380
void __stdcall CmdDPrint(int)
{
}

// FUNCTION: 0x416390
void __stdcall CmdBurnAll(int arg1)
{
    RemoveAllFeatures();
}

#include "../../interfaces/seams.h"

// FUNCTION: 0x4163a0
void __stdcall CmdBurnOne(void* args)
{
    if (g_game->cellFeature < 0xfffb) {
        void* target = g_seams.los_radar->GetMapCell(g_game->point.x, g_game->point.y);
        RemoveFeature((Cell *)target, 1);
    }
}

// FUNCTION: 0x4163d0
void __stdcall CmdFeature(CommandArgs* args)
{
    unsigned short id = FindFeatureType(args->GetArg(1, DAT_005119b8));
    if (id != 0xffff) {
        void* target = g_seams.los_radar->GetMapCell(g_game->point.x, g_game->point.y);
        PlaceFeature(target, id, 0, 0, 10);
    }
}

// FUNCTION: 0x416420
void __stdcall CmdShading(int unused)
{
    g_game->visualBits.shading = !g_game->visualBits.shading;
    ((CMemoryCache*)g_game->unitTable)->FlushCache();
    SaveSettings();
}

// FUNCTION: 0x416460
void __stdcall CmdSelectable(int unused)
{
    for (Unit* u = &g_game->units[1]; u <= g_game->unitsEnd; u++) {
        if (u->flags & 0x10000000) {
            u->flags |= 0x20;
        }
    }
}

// FUNCTION: 0x4164b0
void __stdcall CmdKill(CommandArgs* args)
{
    if (args->count == 1) {
        g_seams.units->KillAllUnits();
    } else {
        g_seams.players->KillPlayerUnits(args->GetIntArg(1, 0));
    }
    g_seams.players->DeactivateConditions(g_game->list_391ed);
}

// FUNCTION: 0x416500
void __stdcall CmdZBuffer(int)
{
}

// FUNCTION: 0x416510
void __stdcall CmdAntiAlias(int unused)
{
    g_game->visualBits.antiAlias = !g_game->visualBits.antiAlias;
    ((CMemoryCache*)g_game->unitTable)->FlushCache();
    SaveSettings();
}

// FUNCTION: 0x416550
void __stdcall CmdShadow(int unused)
{
    g_game->visualBits.shadows = !g_game->visualBits.shadows;
    ((CMemoryCache*)g_game->unitTable)->FlushCache();
    SaveSettings();
}

// FUNCTION: 0x416590
void __stdcall CmdDither(int unused)
{
    g_game->visualBits.ditheredFog = !g_game->visualBits.ditheredFog;
    SaveSettings();
}

// FUNCTION: 0x4165c0
void __stdcall CmdSwitchAlt(CommandArgs* args)
{
    if (args->count < 2) {
        g_game->visualBits.switchAlt = !g_game->visualBits.switchAlt;
        SaveSettings();
        return;
    }
    g_game->visualBits.switchAlt = args->GetIntArg(1, 0);
}

// FUNCTION: 0x416630
void __stdcall CmdTShadow(int unused)
{
    g_game->visualBits.vehicleShadows = !g_game->visualBits.vehicleShadows;
}

// FUNCTION: 0x416660
void __stdcall CmdFShadow(int unused)
{
    g_game->visualBits.featureShadows = !g_game->visualBits.featureShadows;
}

// FUNCTION: 0x416690
void __stdcall CmdLOSType(int unused)
{
    g_game->bits_14281.bit2_14281 = !g_game->bits_14281.bit2_14281;
    g_seams.los_radar->RecalculateLineOfSight(0);
}

// FUNCTION: 0x4166c0
void __stdcall CmdLight(CommandArgs* args)
{
    SetLightVector(args->GetIntArg(1, 0), args->GetIntArg(2, 0), args->GetIntArg(3, 0));
    ((CMemoryCache*)g_game->unitTable)->FlushCache();
}

// FUNCTION: 0x416710
void __stdcall CmdRCache(int unused)
{
    ((CMemoryCache*)g_game->unitTable)->FlushCache();
}

// FUNCTION: 0x416730
void __stdcall CmdEdge(CommandArgs* args)
{
    g_game->mapWidth = g_game->baseX - args->GetIntArg(1, 0x20);
    g_game->mapHeight = g_game->baseY - args->GetIntArg(2, 0x80);
}

// FUNCTION: 0x416780
void __stdcall CmdSearch(CommandArgs* args)
{
    if (args->GetIntArg(1, 0)) {
        g_game->pathfinder->stepsPerTick = args->GetIntArg(1, 0);
    }
    if (args->count == 3) {
        g_game->pathfinder->baseScale = (int)(atof(args->GetArg(2, DAT_005119b8)) * 65536.0);
    }
}

// FUNCTION: 0x4167f0
void __stdcall CmdCDPlay(CommandArgs* args)
{
    ((Sound*)g_game->sound)->PlayCdTrack(args->GetIntArg(1, 0), 1);
}

// FUNCTION: 0x416810
void __stdcall CmdCDStop(int unused)
{
    ((Sound*)g_game->sound)->StopCdAudio();
}

// FUNCTION: 0x416820
void __stdcall CmdSound3D(int unused)
{
    if (((Sound*)g_game->sound)->Is3DEnabled()) {
        ((Sound*)g_game->sound)->Disable3D();
        SaveSettings();
    } else {
        ((Sound*)g_game->sound)->Enable3D();
        SaveSettings();
    }
}

// FUNCTION: 0x416860
void __stdcall CmdMove(CommandArgs* args)
{
    if (_strcmpi(args->GetArg(0, DAT_005119b8), "move") == 0) {
        int dx = args->GetIntArg(1, 0);
        g_game->pos.x += dx << 20;
        int dz = args->GetIntArg(2, 0);
        g_game->pos.z += dz << 20;
    }
}

// FUNCTION: 0x4168d0
void __stdcall CmdLogo(CommandArgs* args)
{
    int n = args->GetIntArg(1, 0);
    if (n >= 0) {
        int m = args->GetIntArg(1, 0);
        unsigned int limit = 0;
        limit = *(unsigned short*)g_game->logos32;
        if (m < (int)limit) {
            unsigned char i = args->GetIntArg(2, 0);
            if (i < 10) {
                Player* p = &g_game->players[i];
                if (p->active != 0 && (p->type == 1 || p->type == 2 || p->type == 3)
                    && p->index != 10) {
                    int v = args->GetIntArg(1, 0);
                    int j = args->GetIntArg(2, 0);
                    g_game->players[j].info->color = v;
                    ((CMemoryCache*)g_game->unitTable)->FlushCache();
                    return;
                }
            }
        }
    }
    AddMessage("Invalid logo setting", 2, 0, 10);
}

// FUNCTION: 0x4169d0
void __stdcall CmdIWin(CommandArgs* args)
{
    if (g_game->players[g_game->localPlayer].info->side == 1)
        g_seams.players->KillPlayerUnits(0);
    else
        g_seams.players->KillPlayerUnits(1);
    g_game->bits.flag4 = 1;
    g_game->bits.flag5 = 1;
    g_game->bits.flag2 = 1;
}

// FUNCTION: 0x416a30
void __stdcall CmdILose(CommandArgs* args)
{
    g_seams.players->KillPlayerUnits(g_game->players[g_game->localPlayer].info->side);
    g_game->bits.flag4 = 0;
    g_game->bits.flag6 = 1;
    g_game->bits.flag2 = 1;
}

// FUNCTION: 0x416a90
void __stdcall CmdSeaLevel(CommandArgs* args)
{
    g_game->seaLevel = args->GetIntArg(1, 0);
}

// FUNCTION: 0x416ab0
void __stdcall CmdControl(CommandArgs* args)
{
    unsigned char i = args->GetIntArg(1, 0);
    if (i < 10) {
        Player* p = &g_game->players[i];
        if (p->active != 0 && (p->type == 1 || p->type == 2 || p->type == 3)
            && p->index != 10) {
            g_game->localPlayer = args->GetIntArg(1, 0);
            g_game->playerIndex = args->GetIntArg(1, 0);
        }
    }
}

// FUNCTION: 0x416b50
void __stdcall CmdView(CommandArgs* args)
{
    unsigned char i = args->GetIntArg(1, 0);
    if (i < 10) {
        Player* p = &g_game->players[i];
        if (p->active != 0 && (p->type == 1 || p->type == 2 || p->type == 3)
            && p->index != 10) {
            g_game->playerIndex = args->GetIntArg(1, 0);
        }
    }
}

// FUNCTION: 0x416bd0
void __stdcall CmdGive(CommandArgs* args)
{
    unsigned char i = args->GetIntArg(1, 0);
    if (i < 10) {
        Player* p = &g_game->players[i];
        if (p->active != 0 && (p->type == 1 || p->type == 2 || p->type == 3)
            && p->index != 10) {
            if (_strcmpi(args->GetArg(3, DAT_005119b8), "metal") == 0) {
                g_seams.players->TransferEnergy(g_game->localPlayer, args->GetIntArg(1, 0),
                             (float)args->GetIntArg(2, 0), 1);
            }
            if (_strcmpi(args->GetArg(3, DAT_005119b8), "energy") == 0) {
                g_seams.players->TransferMetal(g_game->localPlayer, args->GetIntArg(1, 0),
                             (float)args->GetIntArg(2, 0), 1);
            }
        }
    }
}

// FUNCTION: 0x416cf0
void __stdcall CmdScrollSpeed(void* args)
{
    unsigned char result = ((CommandArgs*)args)->GetIntArg(1, 0);
    g_game->scrollSpeed = result;
    SaveSettings();
}

// FUNCTION: 0x416d20
void __stdcall CmdIFace(void* args)
{
    int result = ((CommandArgs*)args)->GetIntArg(1, 0);
    g_game->interfaceType = result;
    SaveSettings();
}

// FUNCTION: 0x416d50
void __stdcall CmdLOS(int unused)
{
    g_game->bits_14281.bit1_14281 = !g_game->bits_14281.bit1_14281;
    SaveSettings();
    g_seams.los_radar->RecalculateLineOfSight(0);
}

// FUNCTION: 0x416d80
void __stdcall CmdMapping(int unused)
{
    g_game->bits_14281.bit0_14281 = !g_game->bits_14281.bit0_14281;
    SaveSettings();
    g_seams.los_radar->RecalculateLineOfSight(1);
}

// FUNCTION: 0x416db0
void __stdcall CmdContour(CommandArgs* args)
{
    g_contourSpacing = (int)(args->GetFloatArg(1, 0.0f) * 256.0f);
    g_contourOffset = (int)(args->GetFloatArg(2, 0.75f) * 256.0f);
}

// FUNCTION: 0x416e00
void __stdcall CmdSelBoxes(int unused)
{
    g_game->uiOptionBits.b2 = !g_game->uiOptionBits.b2;
}

// FUNCTION: 0x416e30
void __stdcall CmdTreeDeath(int unused)
{
    g_game->uiOptionBits.b3 = !g_game->uiOptionBits.b3;
}

// FUNCTION: 0x416e60
void __stdcall CmdNoShake(int unused)
{
    g_game->uiOptionBits.b4 = !g_game->uiOptionBits.b4;
}

// FUNCTION: 0x416e90
void __stdcall CmdNow(CommandArgs* args)
{
    if (args->count==6 && !strcmp(args->GetArg(1,DAT_005119b8),"Film") &&
        !strcmp(args->GetArg(2,DAT_005119b8),"Chris") &&
        !strcmp(args->GetArg(3,DAT_005119b8),"Include") &&
        !strcmp(args->GetArg(4,DAT_005119b8),"Reload") &&
        !strcmp(args->GetArg(5,DAT_005119b8),"Assert"))
        g_game->uiOptionFlags|=2;
    else g_game->uiOptionFlags&=~2;
}

// FUNCTION: 0x417030
void __stdcall CmdDoubleShot(int unused)
{
    unsigned short* ptr = &g_game->uiOptionFlags;
    unsigned short ax = *ptr;
    unsigned short edx = ax;
    edx = (unsigned short)(~edx);
    edx ^= ax;
    edx &= 0x80;
    edx ^= ax;
    *ptr = edx;
}

// FUNCTION: 0x417060
void __stdcall CmdHalfShot(int unused)
{
    g_game->uiOptionBits.b8 = !g_game->uiOptionBits.b8;
}

// FUNCTION: 0x417090
void __stdcall CmdRadar(int unused)
{
    g_game->uiOptionBits.b9 = !g_game->uiOptionBits.b9;
}

// FUNCTION: 0x4170c0
void __stdcall CmdATM(CommandArgs* args)
{
    g_game->players[g_game->playerIndex].energy += 1000.0f;
    g_game->players[g_game->playerIndex].metal += 1000.0f;
}

// FUNCTION: 0x417130
void __stdcall CmdScreenChat(int unused)
{
    g_game->screenchat ^= 1;
    SaveSettings();
}

// FUNCTION: 0x417150
void __stdcall CmdNoMetal(CommandArgs* args)
{
    unsigned char i;
    if (args->count == 1)
        i = g_game->localPlayer;
    else
        i = args->GetIntArg(1, 0);
    if (i < 10) {
        Player* p = &g_game->players[i];
        if (p->active != 0 && (p->type == 1 || p->type == 2 || p->type == 3)
            && p->index != 10) {
            g_game->players[i].metal = args->GetIntArg(2, 0);
        }
    }
}

// FUNCTION: 0x4171f0
void __stdcall CmdNoEnergy(CommandArgs* args)
{
    unsigned char i;
    if (args->count == 1)
        i = g_game->localPlayer;
    else
        i = args->GetIntArg(1, 0);
    if (i < 10) {
        Player* p = &g_game->players[i];
        if (p->active != 0 && (p->type == 1 || p->type == 2 || p->type == 3)
            && p->index != 10) {
            g_game->players[i].energy = args->GetIntArg(2, 0);
        }
    }
}

// FUNCTION: 0x417290
void __stdcall CmdGamma(CommandArgs* args)
{
    SetBrightness(args->GetIntArg(1, 0) * 0.1f);
    g_game->brightness = args->GetIntArg(1, 0);
    SaveSettings();
}

// FUNCTION: 0x4172e0
void __stdcall CmdSing(int unused)
{
    g_cmdSingEnabled = !g_cmdSingEnabled;
}

// FUNCTION: 0x417300
void __stdcall CmdClock(int unused)
{
    g_game->uiOptionBits.b6 = !g_game->uiOptionBits.b6;
    SaveSettings();
}

// FUNCTION: 0x417330
void __stdcall CmdFilm(CommandArgs* args)
{
    if (args->count > 1) {
        strcpy(g_game->path, args->GetArg(1, DAT_005119b8));
        if (g_game->path[strlen(g_game->path) - 1] == '\\' ||
            g_game->path[strlen(g_game->path) - 1] == '/')
            g_game->path[strlen(g_game->path) - 1] = 0;
        g_game->valueSet = 1;
        SaveSettings();
    }
}

// FUNCTION: 0x4173e0
void __stdcall CmdFilmSpeed(CommandArgs* args)
{
    if (args->count > 1 && args->GetIntArg(1, 0) > 0) {
        g_game->movieOutputRate = args->GetIntArg(1, 0);
        g_game->valueSet_38c63 = 1;
        SaveSettings();
    }
}

// FUNCTION: 0x417430
void __stdcall CmdSave(CommandArgs* args)
{
    char path[256];
    if (args->count > 1) {
        sprintf(path, "savegame\\%s.sav", args->GetArg(1, DAT_005119b8));
        MakeDirectoryPath("savegame");
        SaveGameFile(path, "Generic Game Description", 0x29a);
    }
}

// FUNCTION: 0x417490
void __stdcall CmdReload(CommandArgs* args)
{
    if (args->count > 1) {
        short id = g_seams.units->FindUnitTypeId(args->GetArg(1, DAT_005119b8));
        if (id) {
            g_seams.units->KillUnitsOfType(id);
            g_seams.units->ReloadUnitType(id);
        }
    }
}

// FUNCTION: 0x4174d0
void __stdcall CmdReloadAIProfiles(int arg1)
{
    g_seams.ai->ResetAIPlayers();
}

// FUNCTION: 0x4174e0
void __stdcall CmdBigBrother(int unused)
{
    if (g_game->autoFollow) {
        g_game->autoFollow = 0;
        ClearCameraFollowState();
    } else {
        g_game->autoFollow = 1;
        g_game->autoFollowTicks = 1;
    }
}

// FUNCTION: 0x417520
void __stdcall CmdProfile(void* args)
{
    g_game->profileBarsEnabled = g_game->profileBarsEnabled == 0 ? 1 : 0;
}

// FUNCTION: 0x417540
void __stdcall CmdNowISee(void*)
{
    void* p_game = g_game;
    unsigned short* p = (unsigned short*)((char*)p_game + 0x14281);
    *p = *p & 0xfffe;

    p_game = g_game;
    p = (unsigned short*)((char*)p_game + 0x14281);
    *p = *p & 0xfffd;

    g_seams.los_radar->RecalculateLineOfSight(1);
}

// FUNCTION: 0x417570
void __stdcall CmdNetStats(int)
{
    SumMessageStats();
    DAT_00511c20 = g_game->ticks;
    g_messageBytesReceived = 0;
    g_messageBytesSent = 0;
    Pair_00419560* p = (Pair_00419560*)&g_messageBytesByType[1];
    for (int i = 0; i < 44; i++) {
        g_messageCountByType[i + 1][0] = 0;
        g_messageCountByType[i + 1][1] = 0;
        g_messageBytesByType[i + 1][0] = 0;
        p->b = 0;
        p++;
    }
    g_packetsSent = 0;
    g_packetBytesSent = 0;
    g_packetsReceived = 0;
    g_packetBytesReceived = 0;
}

// FUNCTION: 0x4175e0
void __stdcall CmdMusicMode(void* args)
{
    CommandArgs* obj = (CommandArgs*)args;
    int result = obj->GetIntArg(1, 0);
    ((Sound*)g_game->sound)->SetTrackCategory(result);
}

// FUNCTION: 0x417600
void __stdcall CmdMakePoster(CommandArgs* args)
{
    int w = 0xc80;
    int h = 0x960;
    if (args->count > 1)
        w = args->GetIntArg(1, 0);
    if (args->count > 2)
        h = args->GetIntArg(2, 0);
    if (_strcmpi(args->GetArg(1, DAT_005119b8), "all") == 0) {
        w = g_game->mapWidth;
        h = g_game->mapHeight;
    }
    int sx = g_game->screenTilesX;
    w = max(w, sx * 16);
    w = min(w, g_game->mapWidth);
    int sy = g_game->screenTilesY;
    h = max(h, sy * 16);
    h = min(h, g_game->mapHeight);
    int x = g_game->scrollX - w / 2 + sx * 8;
    int y = g_game->scrollY - h / 2 + sy * 8;
    x = max(x, 0);
    x = min(x, g_game->mapWidth - w);
    y = max(y, 0);
    y = min(y, g_game->mapHeight - h);
    char buf[256];
    sprintf(buf, "%s\\screenshots", g_game->path);
    MakeDirectoryPath(buf);
    WriteScreenshot(buf, "BIGSHOT", x, y, w, h);
    g_game->lastTick = GetTicks();
}

// FUNCTION: 0x417760
void __stdcall CmdMeteor(CommandArgs* args)
{
    if (args->count > 1) {
        if (args->GetIntArg(1, 0))
            EnableMeteors();
        else
            DisableMeteors();
    } else {
        StartMeteorShower();
    }
}

// FUNCTION: 0x4177a0
void __stdcall CmdDrop(CommandArgs* args)
{
    int value = args->GetIntArg(1, 0);
    g_game->uiOptionBits.b0 = value == 0;
}

// FUNCTION: 0x4177e0
void __stdcall CmdInclude(CommandArgs* args)
{
    Vec3 pos;
    int info;
    char path[60];

    sprintf(path, "debugdat\\%s.txt", args->GetArg(0, DAT_005119b8));
    FileHandle* file = HAPI_OpenFileRead(path);
    if (file != 0) {
        pos = g_game->pos;
        void* data = HAPI_LoadOpenFile(path, file, (unsigned int*)&info);
        if (data != 0) {
            ExecuteCommandText((char*)data, info, args, 0xffffffff);
            GameFreeThunk(data);
        }
        HAPI_CloseFile(file);
        g_game->pos = pos;
    }
}

// FUNCTION: 0x417890
void __stdcall DefaultCommandHandler(CommandArgs* args)
{
    Vec3 pos=g_game->pos;
    int count=0;
    for (unsigned short i=1;i<g_game->unitDefCount;++i) {
        UnitDef* def=&g_game->defs[i];
        if (MatchWildcard(def->unitname,args->GetArg(0,DAT_005119b8))) {
            if (count) pos.x-=def->modelMinX;
            g_seams.map->SnapWorldPosToFootprint(def,&pos);
            g_seams.units->CreateUnit(args->GetIntArg(1,0),i,pos,1,1,0);
            pos.x+=def->modelMaxX+0x200000;
            if (pos.x >= (g_game->mapWidth<<16)) { pos.x=0xa00000; pos.z+=0xa00000; }
            ++count;
        }
    }
    if (!count) {
        Vec3 saved;
        int size;
        char path[60];
        sprintf(path,"debugdat\\%s.txt",args->GetArg(0,DAT_005119b8));
        FileHandle* file=HAPI_OpenFileRead(path);
        if (file) {
            saved=g_game->pos;
            void* data=HAPI_LoadOpenFile(path,file,(unsigned int*)&size);
            if (data) {
                ExecuteCommandText((char*)data,size,args,0xffffffff);
                GameFreeThunk(data);
            }
            HAPI_CloseFile(file);
            g_game->pos=saved;
        }
    }
}

// FUNCTION: 0x417a60
void __stdcall CmdDebugBreak(CommandArgs* args)
{
    if ((g_game->uiOptionFlags & 2) && (g_game->endGameFlags & 2)) {
        if (args->count > 0) {
            if (args->GetIntArg(1, 0) == 1) {
                for (;;)
                    operator new(0x2000000);
            }
            if (args->GetIntArg(1, 0) == 2) {
                for (;;)
                    GameAllocIgnoreTag("FORCE OUT-OF-MEMORY", 0x2000000);
            }
            if (args->GetIntArg(1, 0) == 3) {
                volatile int one = 1;
                exit(one / (one >> 1));
                return;
            }
        }
        if (args->count == 0 || args->GetIntArg(1, 0) == 0) {
            if (GetDisplay()->windowed) {
                ToggleFullScreen();
                host_sleep_ms(500);
            }
            host_debug_break();
        }
    }
}

// FUNCTION: 0x417b50
void __stdcall ExecuteCommandLine(char* commandLine, int mask)
{
    CommandArgs buf;

    if (commandLine == 0)
        commandLine = g_commandLineBuf;
    else
        strncpy(g_commandLineBuf, commandLine, 0x4f);

    buf.InitArgs();
    buf.Tokenize(commandLine, 0);
    ExecuteCommand(&buf, mask);
}

int __stdcall GetGroundHeight(Pos_00417bb0* pos);

// FUNCTION: 0x417bb0
void __stdcall WorldToScreenWithHeight(Pos_00417bb0* pos, int* screen_x, int* screen_y)
{
    int height = GetGroundHeight(pos);
    *screen_x = pos->x - g_game->scrollX + 0x80;
    *screen_y = pos->z - g_game->scrollY - (height >> 1) + 0x20;
}

// FUNCTION: 0x417c00
void __stdcall WorldToScreenTileOffset(int x, int z, int* screen_x, int* screen_y)
{
    Pos_00417bb0 pos;
    *(int*)&pos.x_frac = x << 16;
    *(int*)&pos.z_frac = z << 16;
    WorldToScreenWithHeight(&pos, screen_x, screen_y);
}

// FUNCTION: 0x417c70
void __stdcall DrawWorldDeltaLine(void* surface, Pos_00417bb0* p, short dx, short dz, int color)
{
    Pos_00417bb0 pos;
    *(int*)&pos.x_frac = (dx << 16) + *(int*)&p->x_frac;
    *(int*)&pos.z_frac = (dz << 16) + *(int*)&p->z_frac;

    int h1 = GetGroundHeight(p);
    int x1 = p->x - g_game->scrollX + 0x80;
    int y1 = p->z - g_game->scrollY - (h1 >> 1) + 0x20;
    int h2 = GetGroundHeight(&pos);
    int x2 = pos.x - g_game->scrollX + 0x80;
    int y2 = pos.z - g_game->scrollY - (h2 >> 1) + 0x20;

    DrawLine(surface, x1, y1, x2, y2, color & 0xff);
}

// FUNCTION: 0x417d30
void __stdcall DrawWorldSegmentLine(void* surface, Point16 from, short dx, short dz, int color)
{
    Pos_00417bb0 pos1, pos2;
    int x1 = from.x << 16;
    int z1 = from.y << 16;
    *(int*)&pos1.x_frac = x1;
    *(int*)&pos1.z_frac = z1;
    *(int*)&pos2.x_frac = x1 + (dx << 16);
    *(int*)&pos2.z_frac = z1 + (dz << 16);
    int sx1, sy1, sx2, sy2;
    WorldToScreenWithHeight(&pos1, &sx1, &sy1);
    WorldToScreenWithHeight(&pos2, &sx2, &sy2);
    DrawLine(surface, sx1, sy1, sx2, sy2, color & 0xff);
}

// FUNCTION: 0x417e00
void AiSearchGoal::DrawOnSurface(void* surface)
{
    DrawMapTileSelectionOutline(surface, (short*)&this->owner->cell, (*(int*)&this->owner->footprint), 0xf);
    unsigned char color = g_game->colors[(this->active & 1) ? 9 : 12];
    for (int i = 0; i < this->count - 1; i++) {
        Pos_00417bb0 p1;
        *(int*)&p1.x_frac = this->points[i].x << 16;
        *(int*)&p1.z_frac = this->points[i].y << 16;
        int h = GetGroundHeight(&p1);
        int x1 = p1.x - g_game->scrollX + 0x80;
        int y1 = p1.z - g_game->scrollY - (h >> 1) + 0x20;

        Pos_00417bb0 p2;
        *(int*)&p2.x_frac = this->points[i + 1].x << 16;
        *(int*)&p2.z_frac = this->points[i + 1].y << 16;
        h = GetGroundHeight(&p2);
        int x2 = p2.x - g_game->scrollX + 0x80;
        int y2 = p2.z - g_game->scrollY - (h >> 1) + 0x20;

        DrawLine(surface, x1, y1, x2, y2, color);
    }
}

// FUNCTION: 0x417f30
void __stdcall DrawSelectedGoal(int ctx, Iface_00417f30*** unit)
{
    if (unit && *unit && **unit) {
        (**unit)->Method_28(ctx);
    }
}

// FUNCTION: 0x417f60
void __stdcall DrawTriangleContours(void* surface, Point_00417f60 p1, int z1,
                            Point_00417f60 p2, int z2,
                            Point_00417f60 p3, int z3)
{
    int tz;
    Point_00417f60 tp;
    if (z2 > z3) {
        tz = z2; z2 = z3; z3 = tz;
        tp = p2; p2 = p3; p3 = tp;
    }
    if (z1 > z2) {
        tz = z1; z1 = z2; z2 = tz;
        tp = p1; p1 = p2; p2 = tp;
    }
    if (z2 > z3) {
        tz = z2; z2 = z3; z3 = tz;
        tp = p2; p2 = p3; p3 = tp;
    }
    int level = z3 / g_contourSpacing * g_contourSpacing + g_contourOffset;
    while (level > z3) {
        level -= g_contourSpacing;
    }
    if (level > z2) {
        int dz13 = z3 - z1;
        int dz23 = z3 - z2;
        do {
            int h13 = level - z1;
            int h23 = level - z2;
            DrawLine(surface,
                         (p1.x * (dz13 - h13) + h13 * p3.x) / dz13,
                         (p1.y * (dz13 - h13) + h13 * p3.y) / dz13,
                         (p2.x * (dz23 - h23) + h23 * p3.x) / dz23,
                         (p2.y * (dz23 - h23) + h23 * p3.y) / dz23,
                         g_contourColors[((level >> 8) - g_game->seaLevel + 0x100) >> 4]);
            level -= g_contourSpacing;
        } while (level > z2);
    }
    if (level > z1) {
        int dz13 = z3 - z1;
        int dz12 = z2 - z1;
        do {
            int h13 = level - z1;
            int h12 = level - z1;
            DrawLine(surface,
                         (p1.x * (dz13 - h13) + h13 * p3.x) / dz13,
                         (p1.y * (dz13 - h13) + h13 * p3.y) / dz13,
                         (p1.x * (dz12 - h12) + h12 * p2.x) / dz12,
                         (p1.y * (dz12 - h12) + h12 * p2.y) / dz12,
                         g_contourColors[((level >> 8) - g_game->seaLevel + 0x100) >> 4]);
            level -= g_contourSpacing;
        } while (level > z1);
    }
}

// FUNCTION: 0x4181d0
void __stdcall DrawCellContours(void* surface, Point_00417f60* corners, unsigned char* heights)
{
    Point_00417f60 mid;
    mid.x = (corners[0].x + corners[1].x + corners[2].x + corners[3].x + 2) / 4;
    mid.y = (corners[0].y + corners[1].y + corners[2].y + corners[3].y + 2) / 4;
    int midHeight = (heights[0] + heights[1] + heights[2] + heights[3]) * 64;
    DrawTriangleContours(surface, corners[0], heights[0] * 256, corners[1], heights[1] * 256, mid, midHeight);
    DrawTriangleContours(surface, corners[1], heights[1] * 256, corners[2], heights[2] * 256, mid, midHeight);
    DrawTriangleContours(surface, corners[2], heights[2] * 256, corners[3], heights[3] * 256, mid, midHeight);
    DrawTriangleContours(surface, corners[3], heights[3] * 256, corners[0], heights[0] * 256, mid, midHeight);
}

struct TdfRecord;
struct Mission;
struct MissionUnit;
struct MissionRule;
struct MissionFeature;
struct HapiFile;
struct StringPool;
#include "../harness/harness.h"

// FUNCTION: 0x418310
void __stdcall DrawMapDebugOverlay(void* surface)
{
    if (!g_game->debugMode && !g_contourSpacing) return;
    Movement* movement = 0;
    Player* player = &g_game->players[g_game->playerIndex];
    if (g_game->debugMode == 1) {
        Unit* unit = FindNextSelectedUnit(0, 0);
        if (unit) movement = (Movement*)unit->def->movementclass.get();
    }
    int firstY = g_game->scrollY / 16;
    int firstX = g_game->scrollX / 16;
    int lastX = __min(g_game->screenTilesX + firstX + 1, g_game->width - 1);
    int lastY = g_game->height - 1;
    unsigned char* colors = g_game->colors;
    unsigned char arrowColor;
    for (int y = firstY; y < lastY; ++y) {
        int offscreen = 1;
        Point_00417f60 p[4];
        unsigned char heights[4];
        for (int x = firstX; x < lastX; ++x) {
            Tile* tile = &((Tile*)g_game->cells.get())[y * g_game->width + x];
            heights[0] = tile->height;
            p[0].x = (x + 8) * 16 - g_game->scrollX;
            p[0].y = (y + 2) * 16 - (heights[0] >> 1) - g_game->scrollY;
            ++tile; ++x;
            heights[1] = tile->height;
            p[1].x = (x + 8) * 16 - g_game->scrollX;
            p[1].y = (y + 2) * 16 - (heights[1] >> 1) - g_game->scrollY;
            tile += g_game->width; ++y;
            heights[2] = tile->height;
            p[2].x = (x + 8) * 16 - g_game->scrollX;
            p[2].y = (y + 2) * 16 - (heights[2] >> 1) - g_game->scrollY;
            --tile; --x;
            heights[3] = tile->height;
            p[3].x = (x + 8) * 16 - g_game->scrollX;
            p[3].y = (y + 2) * 16 - (heights[3] >> 1) - g_game->scrollY;
            tile -= g_game->width; --y;
            if (p[0].y < g_game->screenHeight) offscreen = 0;
            if (g_game->debugMode == 1) {
                if (movement) {
                    unsigned int state = (movement->states[movement->width * (y >> 4) + x] >> ((y & 15) * 2)) & 3;
                    if (state < 3) {
                        unsigned char color = colors[g_movementClassColors[state]];
                        DrawLine(surface, p[0].x, p[0].y, p[2].x, p[2].y, color);
                        DrawLine(surface, p[1].x, p[1].y, p[3].x, p[3].y, color);
                    }
                }
                PathCell* cell = g_game->pathfinder->grid.At(x, y);
                if (cell->flags & 4) {
                    SetFont((int)g_game->fontSmlFont);
                    SetTextColors(HarnessViewRand() & 255, GetTextKeyColor());
                    DrawString(surface, "G", p[0].x, p[0].y, -1);
                }
                unsigned char kind = cell->flags & ~4;
                if (kind != 0 && kind != 3) {
                    int cx = p[0].x + 8, cy = p[0].y + 8;
                    switch (cell->flags) {
                    case 1: arrowColor = colors[15]; break;
                    case 2: arrowColor = colors[4]; break;
                    }
                    DrawLine(surface, cx - g_dirDeltaX[(signed char)cell->dir] * 14,
                                 cy - g_dirDeltaZ[(signed char)cell->dir] * 14, cx, cy, arrowColor);
                    int direction = ((signed char)cell->dir + 1) & 7;
                    DrawLine(surface, cx - g_dirDeltaX[direction] * 4,
                                 cy - g_dirDeltaZ[direction] * 4, cx, cy, arrowColor);
                    direction = ((signed char)cell->dir - 1) & 7;
                    DrawLine(surface, cx - g_dirDeltaX[direction] * 4,
                                 cy - g_dirDeltaZ[direction] * 4, cx, cy, arrowColor);
                }
            } else if (g_game->debugMode == 2) {
                if (tile->height > g_game->seaLevel) {
                    DrawLine(surface, p[0].x, p[0].y, p[1].x, p[1].y, colors[15]);
                    DrawLine(surface, p[0].x, p[0].y, p[3].x, p[3].y, colors[15]);
                } else {
                    DrawLine(surface, p[0].x, p[0].y, p[1].x, p[1].y, colors[13]);
                    DrawLine(surface, p[0].x, p[0].y, p[3].x, p[3].y, colors[13]);
                }
                if (tile->unit) FillPolygon(surface, p, 4, tile->unit & 0xff);
                else if (tile->object != 0xffff) FillPolygon(surface, p, 4, (tile->object - 56) & 0xff);
                if (tile->feature) {
                    DrawLine(surface, p[0].x, p[0].y, p[2].x, p[2].y, tile->feature & 0xff);
                    DrawLine(surface, p[1].x, p[1].y, p[3].x, p[3].y, tile->feature & 0xff);
                }
                if (tile->flags & 2) {
                    DrawLine(surface, (p[0].x + p[1].x) / 2, (p[0].y + p[1].y) / 2 + 2,
                                 (p[1].x + p[2].x) / 2 - 2, (p[1].y + p[2].y) / 2, colors[15]);
                    DrawLine(surface, (p[1].x + p[2].x) / 2 - 2, (p[1].y + p[2].y) / 2,
                                 (p[2].x + p[3].x) / 2, (p[2].y + p[3].y) / 2 - 2, colors[15]);
                    DrawLine(surface, (p[2].x + p[3].x) / 2, (p[2].y + p[3].y) / 2 - 2,
                                 (p[3].x + p[0].x) / 2 + 2, (p[3].y + p[0].y) / 2, colors[15]);
                    DrawLine(surface, (p[3].x + p[0].x) / 2 + 2, (p[3].y + p[0].y) / 2,
                                 (p[0].x + p[1].x) / 2, (p[0].y + p[1].y) / 2 + 2, colors[15]);
                }
            } else if (g_game->debugMode == 3) {
                if (tile->height > g_game->seaLevel) {
                    DrawLine(surface, p[0].x, p[0].y, p[1].x, p[1].y, colors[15]);
                    DrawLine(surface, p[0].x, p[0].y, p[3].x, p[3].y, colors[15]);
                } else {
                    DrawLine(surface, p[0].x, p[0].y, p[1].x, p[1].y, colors[13]);
                    DrawLine(surface, p[0].x, p[0].y, p[3].x, p[3].y, colors[13]);
                }
                SetFont((int)g_game->fontSmlFont);
                SetTextColors(colors[15], GetTextKeyColor());
                char buffer[20];
                DrawString(surface, _itoa(tile->metal, buffer, 10), p[0].x + 2, p[0].y + 2, -1);
            } else if (g_game->debugMode == 4) {
                DrawLine(surface, p[0].x, p[0].y, p[1].x, p[1].y, colors[0]);
                DrawLine(surface, p[0].x, p[0].y, p[3].x, p[3].y, colors[0]);
                if (player->explored[(y / 2) * player->exploredWidth + x / 2]) {
                    Rect r;
                    r.left = p[0].x - 5;
                    r.right = p[0].x + 5;
                    r.top = p[0].y - 5;
                    r.bottom = p[0].y + 5;
                    FillRectangle(surface, &r, colors[15]);
                }
            }
            if (g_contourSpacing) DrawCellContours(surface, p, heights);
        }
        if (offscreen) break;
    }
}

// FUNCTION: 0x418bb0
void __stdcall CmdPrintWeights(CommandArgs* args)
{
    if (args->count == 3) {
        unsigned char i = args->GetIntArg(1, 0);
        if (i < 10) {
            Player* p = &g_game->players[i];
            if (p->active != 0 && (p->type == 1 || p->type == 2 || p->type == 3)
                && p->index != 10) {
                void* f = host_file_create(args->GetArg(2, DAT_005119b8), 0);
                if (f != 0) {
                    g_seams.ai->DumpPlayerAI(args->GetIntArg(1, 0), (_iobuf*)f);
                    host_file_close(f);
                }
            }
        }
    }
}

// FUNCTION: 0x418c70
void __stdcall CmdSenderror(CommandArgs* args)
{
    if (args->count == 2) {
        int value = args->GetIntArg(1, 0);
        if (value < 0 || value > 100)
            value = 0;
        g_game->netSendDropPct = value;
    }
}

// FUNCTION: 0x418ca0
void __stdcall CmdShootAll(int unused)
{
    g_game->uiOptionBits.flag10 = !g_game->uiOptionBits.flag10;
}

// GLOBAL: 0x501d38
ConsoleCommand g_consoleCommands[44] = {
    {"NoShake", (void (__stdcall *)(CommandArgs *))CmdNoShake, 1},
    {"Contour", CmdContour, 1},
    {"ScrollSpeed", (void (__stdcall *)(CommandArgs *))CmdScrollSpeed, 1},
    {"IFace", (void (__stdcall *)(CommandArgs *))CmdIFace, 1},
    {"Give", CmdGive, 1},
    {"CDPlay", CmdCDPlay, 1},
    {"CDStop", (void (__stdcall *)(CommandArgs *))CmdCDStop, 1},
    {"Sound3D", (void (__stdcall *)(CommandArgs *))CmdSound3D, 1},
    {"Shading", (void (__stdcall *)(CommandArgs *))CmdShading, 1},
    {"AntiAlias", (void (__stdcall *)(CommandArgs *))CmdAntiAlias, 1},
    {"Shadow", (void (__stdcall *)(CommandArgs *))CmdShadow, 1},
    {"Dither", (void (__stdcall *)(CommandArgs *))CmdDither, 1},
    {"SwitchAlt", CmdSwitchAlt, 1},
    {"TShadow", (void (__stdcall *)(CommandArgs *))CmdTShadow, 1},
    {"FShadow", (void (__stdcall *)(CommandArgs *))CmdFShadow, 1},
    {"LOSType", (void (__stdcall *)(CommandArgs *))CmdLOSType, 1},
    {"Light", CmdLight, 1},
    {"RCache", (void (__stdcall *)(CommandArgs *))CmdRCache, 1},
    {"Selectable", (void (__stdcall *)(CommandArgs *))CmdSelectable, 1},
    {"MusicMode", (void (__stdcall *)(CommandArgs *))CmdMusicMode, 1},
    {"Logo", CmdLogo, 1},
    {"ScreenChat", (void (__stdcall *)(CommandArgs *))CmdScreenChat, 1},
    {"Gamma", CmdGamma, 1},
    {"Clock", (void (__stdcall *)(CommandArgs *))CmdClock, 1},
    {"NetStats", (void (__stdcall *)(CommandArgs *))CmdNetStats, 1},
    {"Sing", (void (__stdcall *)(CommandArgs *))CmdSing, 1},
    {"NoMetal", CmdNoMetal, 1},
    {"NoEnergy", CmdNoEnergy, 1},
    {"BigBrother", (void (__stdcall *)(CommandArgs *))CmdBigBrother, 1},
    {"Now", CmdNow, 1},
    {"Drop", CmdDrop, 1},
    {"ShootAll", (void (__stdcall *)(CommandArgs *))CmdShootAll, 1},
    {"ShareMetal", (void (__stdcall *)(CommandArgs *))CmdShareMetal, 1},
    {"ShareEnergy", (void (__stdcall *)(CommandArgs *))CmdShareEnergy, 1},
    {"ShareMapping", (void (__stdcall *)(CommandArgs *))CmdShareMapping, 1},
    {"ShareRadar", (void (__stdcall *)(CommandArgs *))CmdShareRadar, 1},
    {"ShareAll", (void (__stdcall *)(CommandArgs *))CmdShareAll, 1},
    {"ShowRanges", (void (__stdcall *)(CommandArgs *))CmdShowRanges, 1},
    {"SetShareMetal", CmdSetShareMetal, 1},
    {"SetShareEnergy", CmdSetShareEnergy, 1},
    {"Compression", (void (__stdcall *)(CommandArgs *))CmdCompression, 1},
    {"BPS", (void (__stdcall *)(CommandArgs *))CmdBPS, 1},
    {"SFX", (void (__stdcall *)(CommandArgs *))CmdSFX, 1},
    {0, 0, 0},
};

// GLOBAL: 0x501f48
ConsoleCommand g_cheatCommands[11] = {
    {"Radar", (void (__stdcall *)(CommandArgs *))CmdRadar, 2},
    {"ATM", CmdATM, 2},
    {"View", CmdView, 2},
    {"LOS", (void (__stdcall *)(CommandArgs *))CmdLOS, 2},
    {"Mapping", (void (__stdcall *)(CommandArgs *))CmdMapping, 2},
    {"DoubleShot", (void (__stdcall *)(CommandArgs *))CmdDoubleShot, 2},
    {"HalfShot", (void (__stdcall *)(CommandArgs *))CmdHalfShot, 2},
    {"NowISee", (void (__stdcall *)(CommandArgs *))CmdNowISee, 2},
    {"Meteor", CmdMeteor, 2},
    {"MakePoster", CmdMakePoster, 2},
    {0, 0, 0},
};

// GLOBAL: 0x501fcc
int DAT_00501fcc = 0;

// GLOBAL: 0x501fd0
ConsoleCommand g_debugCommands[31] = {
    {"AI", CmdAI, 4},
    {"Control", CmdControl, 4},
    {"Kill", CmdKill, 4},
    {"IWin", CmdIWin, 4},
    {"ILose", CmdILose, 4},
    {"Film", CmdFilm, 4},
    {"FilmSpeed", CmdFilmSpeed, 4},
    {"Assert", (void (__stdcall *)(CommandArgs *))CmdAssert, 4},
    {"Assign", CmdAssign, 4},
    {"BurnAll", (void (__stdcall *)(CommandArgs *))CmdBurnAll, 4},
    {"BurnOne", (void (__stdcall *)(CommandArgs *))CmdBurnOne, 4},
    {"DebugBreak", CmdDebugBreak, 4},
    {"DPrint", (void (__stdcall *)(CommandArgs *))CmdDPrint, 4},
    {"Edge", CmdEdge, 4},
    {"Include", CmdInclude, 4},
    {"Mem", (void (__stdcall *)(CommandArgs *))CmdMem, 4},
    {"MemDump", (void (__stdcall *)(CommandArgs *))CmdMemDump, 4},
    {"Move", CmdMove, 4},
    {"PrintWeights", CmdPrintWeights, 4},
    {"Profile", (void (__stdcall *)(CommandArgs *))CmdProfile, 4},
    {"Reload", CmdReload, 4},
    {"ReloadAIProfiles", (void (__stdcall *)(CommandArgs *))CmdReloadAIProfiles, 4},
    {"Save", CmdSave, 4},
    {"SeaLevel", CmdSeaLevel, 4},
    {"Search", CmdSearch, 4},
    {"SelBoxes", (void (__stdcall *)(CommandArgs *))CmdSelBoxes, 4},
    {"Senderror", CmdSenderror, 4},
    {"TreeDeath", (void (__stdcall *)(CommandArgs *))CmdTreeDeath, 4},
    {"Feature", CmdFeature, 4},
    {"ZBuffer", (void (__stdcall *)(CommandArgs *))CmdZBuffer, 4},
    {0, 0, 0},
};
