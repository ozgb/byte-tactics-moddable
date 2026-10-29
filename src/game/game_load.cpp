#include <excpt.h>
#include <memory.h>
#include "win_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <time.h>
#include "../../interfaces/host.h"
#include "../state/game_rand.h"
#include "../files/files_api.h"

#include "../../interfaces/ref.h"
#pragma pack(push, 1)

#include "game.h"
#include "mission_option_flags.h"
#include "main2_menu.h"

#include "create_unit_pos.h"

struct PlayerRecordFlags {
    unsigned short started : 1;
    unsigned short : 15;
};

struct LoadingScreenDisplay {
    char unknown_0[0x40];
    HWND hwnd;
};

struct LoadGuiLayerParent {
    char unknown_0[0x10];
};

struct LoadedGuiLayer {
    char unknown_0[0x8];
    void (__stdcall* handler)(LoadedGuiLayer*);
    char* owner;
};


#include "../network/player_info.h"

#include "../graphics/display.h"
#include "../network/net_game.h"
int AimCobStub(int, int, int, int);
void DrawLine(int, int, int, int, int, int);

struct LoadingPlayerRecord {
    union {
        int present;
        int active;
    };
    unsigned int id;
    char unknown_8[0x20 - 0x8];
    unsigned char percent;
    unsigned char keepaliveFlags;
    char unknown_22[0x27 - 0x22];
    Ref<PlayerInfo> data;
    char name[0x73 - 0x2b];
    union {
        unsigned char team;
        unsigned char control;
    };
    char unknown_74[0xdc - 0x74];
    float size1;
    float size2;
    char unknown_e4[0x146 - 0xe4];
    unsigned char kind;
    unsigned char startPos;
    char unknown_148[0x149 - 0x148];
    PlayerRecordFlags flags_149;
};

typedef LoadingPlayerRecord LoadingPlayerInfo;
typedef LoadingPlayerRecord LoadingPlayerEntry;

struct WorldViewOrigin {
    char unknown_0[0x2c];
    int x;
    int y;
};

struct MinimapWorldPos {
    Fixed x;
    Fixed y;
    Fixed z;
};

struct CursorWorldPos {
    unsigned int x;
    unsigned int y;
    unsigned int z;
};

#include "../graphics/rect.h"
#include "../ingame/view_mouse_event.h"

struct CursorMoveEvent {
    int x;
    int y;
    char unknown_8[0x18 - 0x8];
};

#include "../map/cell.h"
#include "../util/vec3.h"

class Sound;


#pragma pack(pop)

extern Game* g_game;

extern unsigned char g_scorePanelKillFlash[10];
extern unsigned char g_scorePanelLossFlash[10];
extern "C" unsigned char g_loadingBarFlashAlpha[8];
extern "C" int g_loadingBarFlashDecayTick;
extern int g_usePacketManager;
extern "C" unsigned char g_loadingBarPrevPercent[8];
extern int g_nonCampaignGame;

#include "../map/mission.h"

#include "../network/packet_manager.h"


#include "../sound/sound.h"
int GetHostDpid();
void RegisterUnitOrders(void);
#include "../ingame/control_panel.h"
#include "../ingame/camera.h"

#include "../util/math.h"
#include "../harness/harness.h"
unsigned char __stdcall FindHostSlot();
#include "settings.h"
#include "../frontend/skirmish_menu.h"
#include "game_state.h"
#include "settings_campaign.h"
#include "../units/unit_commands.h"
void __stdcall LoadPictureCached(const char* name, int a, int b, int c);
#include "../network/unit_sync.h"
#include "main_loop.h"
void __stdcall CreateUnit(unsigned char team, unsigned short id, CreateUnitPos pos, int a,
    int b, int c);
LoadedGuiLayer* __stdcall LoadGuiLayer(LoadGuiLayerParent* sub, const char* name, int flags);
#include "../ingame/game_dialogs.h"
void __cdecl operator delete(void* p);

#include "../gui/gui.h"
void __stdcall BlitMenuLayers(Main2Menu* menu, int a, int b);
char* __stdcall Translate(char* s);

#include "../graphics/pcx.h"
#include "../ingame/build_placement.h"
#include "../frontend/options.h"
#include "../frontend/frontend.h"
#include "../network/online.h"
#include "../ingame/info_panel.h"
void __cdecl ShowSoftwareCursor();
void __stdcall UnlockScreen(void*);
#include "../graphics/surface.h"
void __cdecl FlipScreen();
int __cdecl IsCdPlaying();
#include "../graphics/object_pool.h"
#include "data_files.h"
#include "game_load.h"
int __stdcall GetTextPixelWidth(char*);
void __stdcall SetCursorAnimation(void*, void*);
void __stdcall RemapPaletteToClosestIndices(void*, void*, void*);
void __stdcall SetCloseHandler(void (__cdecl *)(int), int);
void* __stdcall GetGafFrame(void*, int);
void __stdcall DrawFrame(void*, void*, int, int);
void* __stdcall FindGafEntry(int, char*);
#include "../graphics/palette.h"
void __stdcall SetTextColors(int, int);
void __stdcall SetFont(int);
int __stdcall LockScreen(void*);
#include "../graphics/surface.h"
void __stdcall SetOffscreenSurface(void*);
void* __stdcall AllocSurface(char*, int, int);
void __stdcall DrawSurface(void*, void*, int, int);
#include "../graphics/draw.h"
unsigned int __cdecl GetTicks();
#include "main.h"

#include "../../interfaces/seams.h"
#include "../state/sim_globals.h"
int __stdcall PointInRect(Rect* r, int x, int y);

inline void __stdcall ApplyMissionOptionFlags(MissionOptionFlags* s)
{
    g_game->commanderDeath = s->value;
    g_game->mapFlags_14281.bit2 = s->flag_c;
    g_game->mapFlags_14281.bit0 = s->flag_4;
    g_game->mapFlags_14281.bit1 = s->flag_8;
}

// FUNCTION: 0x497180
void __cdecl LoadMatch(void*)
{
    unsigned long long perfCount;
    CreateUnitPos pos;
    CreateUnitPos start;
    int order[10];

    perfCount = host_time_counter();
    SeedRandom(HarnessGameSeed((unsigned int)perfCount + (unsigned int)(perfCount >> 32)));
    GameSrand(RAND_STREAM_LOAD, HarnessCrtSeed((unsigned)time(NULL)));
    g_game->ticks = 0;

    switch (g_seams.map->GetGameType(((Mission*)g_game->net))) {
    case 1:
        g_nonCampaignGame = 0;
        ApplyMissionOptionFlags(&g_game->singleSettings);
        ApplyUseOnlyUnits();
        break;
    case 2:
        g_game->maxUnitsSetting = g_game->maxUnits_37eec;
        g_nonCampaignGame = 1;
        ApplyMissionOptionFlags(&((SkirmishOptionsTable*)g_game->options)->missionFlags);
        break;
    case 3: {
        g_game->maxUnitsSetting = g_game->maxUnits_37eec;
        g_nonCampaignGame = 1;
        g_game->otherFlags &= 0xfffe;

        int sel = FindHostSlot();
        unsigned char cur = g_game->localPlayer;
        if ((*(LoadingPlayerRecord(*)[10])&g_game->players)[cur].keepaliveFlags & 2) {
            do {
                PlayerInfo* p = (*(LoadingPlayerRecord(*)[10])&g_game->players)[g_game->localPlayer].data;
                if (g_usePacketManager)
                    g_packetManager.SendAllQueued(1);
                HandleNetPackets();
                sel = FindHostSlot();
                host_sleep_ms(0x32);
                if (sel == 10)
                    continue;
                if (p->color == 0xff)
                    continue;
                if (p->unknown_8f == 0)
                    continue;
                break;
            } while (1);
            host_sleep_ms(0x32);
        }

        g_seams.map->LoadMissionByName(((Mission*)g_game->net), 0, (char*)(*(LoadingPlayerRecord(*)[10])&g_game->players)[sel].data.get());
        if (FindHostSlot() == 10)
            break;

        int sel2 = FindHostSlot();
        PlayerInfo* p2 = (*(LoadingPlayerRecord(*)[10])&g_game->players)[sel2].data;
        g_nonCampaignGame = p2->cheating;
        g_game->commanderDeath = p2->commander;
        g_game->mapFlags_14281.bit1 = p2->los;
        g_game->mapFlags_14281.bit2 = p2->losType;
        g_game->mapFlags_14281.bit0 = p2->mapping;
        g_game->maxUnitsSetting = p2->maxUnits;
        break;
    }
    default:
        break;
    }

    if (((HapiBank*)g_game->pendingSaveStore) != 0) {
        ((HapiBank*)g_game->pendingSaveStore)->OpenAccount("summary");
        if (((HapiBank*)g_game->pendingSaveStore)->HasItem("BetweenMissions") == 0) {
            g_seams.players->LoadPlayerControllers(((HapiBank*)g_game->pendingSaveStore));
            if (g_seams.map->GetGameType(((Mission*)g_game->net)) == 2) {
                int count = 0;
                SkirmishPlayer* def = ((SkirmishOptionsTable*)g_game->options)->players;
                int i = 0;
                while (i < 10) {
                    if (def->controller == 1 || def->controller == 2)
                        count = i + 1;
                    i++;
                    def++;
                }
                int cur = g_game->playerCount;
                if (count > cur)
                    cur = count;
                g_game->playerCount = cur;
                ApplySlotsToGamePlayers();
            }
        }
    }

    LoadBattleAssets();

    if (g_seams.map->GetGameType(g_game->net) != 1) {
        if (g_seams.map->GetGameType(g_game->net) == 3) {
            (*(volatile union LoadingFlags*)&g_game->loadingFlags).value |= 4;
            while (((*(volatile union LoadingFlags*)&g_game->loadingFlags).value & 8) == 0)
                host_sleep_ms(0x32);

            int sel = FindHostSlot();
            PlayerInfo* pl = (*(LoadingPlayerRecord(*)[10])&g_game->players)[sel].data;
            g_game->mapFlags_14281.bit0 = pl->mapping;
            g_game->mapFlags_14281.bit1 = pl->los;
            g_game->mapFlags_14281.bit2 = pl->losType;
            g_game->commanderDeath = pl->commander;

            for (int i = 0; i < 10; i++) {
                LoadingPlayerRecord* rec = &(*(LoadingPlayerRecord(*)[10])&g_game->players)[i];
                if (rec->active == 0)
                    continue;
                unsigned char st = rec->team;
                if (st != 1 && st != 2)
                    continue;
                pos.x.i = (RandomInt(g_game->baseX - 0xa0) + 0x50) << 16;
                pos.y.i = 0;
                pos.z.i = (RandomInt(g_game->baseY - 0xa0) + 0x50) << 16;
                if (rec->active != 0 && (rec->data->flags_9b & 0x40))
                    continue;
                PlayerInfo* pl2 = rec->data;
                int side = pl2->side;
                int which = rec->startPos;
                g_seams.map->GetStartPosition(((Mission*)g_game->net), 0, (Vec3*)&pos, which);
                if (rec->active != 0 && rec->team == 1)
                    start = pos;
                unsigned short id = g_seams.units->FindUnitTypeId(g_game->sideDefs[side].commander);
                g_seams.units->CreateUnit(rec->kind, id, *(Vec3*)&pos, 1, 1, 0);
                int s1 = pl->energy * 100;
                int s2 = pl->metal * 100;
                rec->flags_149.started = 1;
                rec->size1 = (float)(s1 >= 200 ? s1 : 200);
                rec->size2 = (float)(s2 >= 200 ? s2 : 200);
            }

            unsigned char li = g_game->localPlayer;
            PlayerInfo* lp = (*(LoadingPlayerRecord(*)[10])&g_game->players)[li].data;
            int cx;
            int cz;
            if (lp->bit6) {
                g_game->mapFlags &= 0xfffe;
                g_game->mapFlags &= 0xfffd;
                cx = g_game->viewWidth / 2;
                cz = g_game->viewHeight / 2;
            } else {
                cx = start.x.h.whole - g_game->viewWidth / 2;
                cz = start.z.h.whole - g_game->viewHeight / 2;
            }
            SetCameraPosition(cx, cz, 0);
            ReportGameEvent(6);
        } else if (g_seams.map->GetGameType(((Mission*)g_game->net)) == 2 &&
            ((HapiBank*)g_game->pendingSaveStore) == 0) {
            if (((SkirmishOptionsTable*)g_game->options)->fixedLocations != 0) {
                for (int i1 = 0; i1 < 10; i1++) {
                    if ((unsigned char)i1 < 10) {
                        LoadingPlayerRecord* rec = &(*(LoadingPlayerRecord(*)[10])&g_game->players)[(unsigned char)i1];
                        if (rec->active != 0) {
                            unsigned char st = rec->team;
                            if ((st == 1 || st == 2 || st == 3) &&
                                rec->kind != 10)
                                SpawnCommanderAtStartPos(i1, i1);
                        }
                    }
                }
            } else {
                for (int i2 = 0; i2 < 10; i2++)
                    order[i2] = -1;
                int n = 0;
                for (int i3 = 0; i3 < 10; i3++) {
                    if ((unsigned char)i3 < 10) {
                        LoadingPlayerRecord* rec = &(*(LoadingPlayerRecord(*)[10])&g_game->players)[(unsigned char)i3];
                        if (rec->active != 0) {
                            unsigned char st = rec->team;
                            if ((st == 1 || st == 2 || st == 3) &&
                                rec->kind != 10)
                                order[n++] = i3;
                        }
                    }
                }
                if (n > 2 || (int)(((__int64)GameRand(RAND_STREAM_LOAD) * 2) / 0x8000) != 0) {
                    GameShuffleDraw draw(RAND_STREAM_LOAD);
                    std::random_shuffle(order, order + n, draw);
                }
                int k = 0;
                for (int i4 = 0; i4 < 10; i4++) {
                    if ((unsigned char)i4 < 10) {
                        LoadingPlayerRecord* rec = &(*(LoadingPlayerRecord(*)[10])&g_game->players)[(unsigned char)i4];
                        if (rec->active != 0) {
                            unsigned char st = rec->team;
                            if ((st == 1 || st == 2 || st == 3) &&
                                rec->kind != 10)
                                SpawnCommanderAtStartPos(i4, order[k++]);
                        }
                    }
                }
            }
            g_seams.players->InitPlayerResources();
        }
    }

    g_seams.los_radar->RecalculateLineOfSight(1);

    if (((HapiBank*)g_game->pendingSaveStore) != 0) {
        ((HapiBank*)g_game->pendingSaveStore)->OpenAccount("summary");
        if (((HapiBank*)g_game->pendingSaveStore)->HasItem("BetweenMissions") == 0) {
            LoadSavedGameState(((HapiBank*)g_game->pendingSaveStore));
            goto tail;
        }
    } else if (g_seams.map->GetGameType(((Mission*)g_game->net)) != 1) {
        goto tail;
    }
    CreateMissionUnits();
    CenterCameraOnStartPosition();

tail:
    {
        int pnum = g_game->playerIndex;
        LoadingPlayerRecord* rec = &(*(LoadingPlayerRecord(*)[10])&g_game->players)[pnum];
        LoadPictureCached(0, 0, 0, 0);
        PlayerInfo* pl = rec->data;
        int side = pl->side;
        sprintf(g_game->name, "%sMAIN2.GUI", g_game->sideDefs[side].nameprefix);
    }
    LoadedGuiLayer* gadget =
        LoadGuiLayer((LoadGuiLayerParent*)&g_game->menu_519_18, g_game->name, 0x20);
    gadget->handler = (void (__stdcall *)(LoadedGuiLayer *))HandleMain2LayoutEvent;
    gadget->owner = (char*)g_game;

    LoadingPlayerRecord* currec = &(*(LoadingPlayerRecord(*)[10])&g_game->players)[g_game->localPlayer];
    currec->data->flags_9b |= 0x10;
    BroadcastPlayerInfo();
    UpdateNetGameInfo();
    g_seams.players->UpdatePlayersOnStream(RAND_STREAM_LOAD);
    g_seams.players->InitPlayerResources();

    HapiBank* mission = ((HapiBank*)g_game->pendingSaveStore);
    if (mission != 0) {
        mission->CloseBank();
        operator delete(mission);
        g_game->pendingSaveStore = 0;
    }
    g_seams.players->RebuildAIFeatureCells();

    (*(volatile union LoadingFlags*)&g_game->loadingFlags).value |= 2;
}
static inline char* PlayerRecordAt(Game* game, int off) { return (char*)(*(LoadingPlayerRecord(*)[10])&game->players) + off; }

// FUNCTION: 0x497c70
void __cdecl LoadThreadMain(void* param)
{
    __try {
        LoadMatch(param);
    } __except (ExceptionFilter(GetExceptionInformation(), "Load Thread")) {
    }
}

// FUNCTION: 0x497ce0
void __stdcall DrawSyncStatus(void* surface)
{
    int off;
    extern Game* g_game;

    RenderLayer((Gui *)&g_game->menu_519_18, 0x40);
    MarkLayerChanged((Dialog *)&g_game->menu_519_18);
    BlitMenuLayers(&g_game->menu_519_18, 0, 0);

    const char* text;
    if (g_game->netBits.synced) {
        text = Translate("Synchronization complete");
    } else {
        int countA = 0;
        int countB = 0;
        for (int i = 0; i < 10; i++) {
            LoadingPlayerEntry* p = &(*(LoadingPlayerRecord(*)[10])&g_game->players)[i];
            if (p->present && (p->team == 1 || p->team == 2 || p->team == 3) && p->kind != 10)
                countA++;
            if (p->present && (p->team == 1 || p->team == 2 || p->team == 3) && p->kind != 10 &&
                p->percent == 100 && g_game->loaded[i] != 0)
                countB++;
        }

        int x;
        int slot = 620 / countA;
        Rect r;
        r.left = 10;
        r.top = 420;
        r.bottom = 435;
        off = 0;
        x = 11;
        for (; off < 10 * (int)sizeof(LoadingPlayerEntry); off += sizeof(LoadingPlayerEntry)) {
            Game* game = g_game;
            LoadingPlayerEntry* q = (LoadingPlayerEntry*)PlayerRecordAt(game, off);
            if (!(q->present && (q->team == 1 || q->team == 2 || q->team == 3) && q->kind != 10))
                continue;
            {
                r.left = x;
                r.right = slot + x - 2;
                FillRectangle((Surface*)surface, &r, g_game->colors[4]);
                int pc = q->percent;
                r.right = r.left + (pc * (slot - 2)) / 100;
                FillRectangle((Surface*)surface, &r, g_game->colors[10]);
                DrawTextClipped(surface, q->name, r.left, 420, slot - 2, 0);
                x += slot;
            }
        }

        const char* pr;
        int ready = countB == 1;
        if (ready)
            pr = Translate("player ready");
        else
            pr = Translate("players ready");
        char buf[128];
        sprintf(buf, "%s.  %i %s", Translate("Waiting for other players"), countB, pr);
        text = buf;
    }

    DrawTextClipped(surface, (char *)text, 10, 400, -1, 0);
}
static inline void DrawLoadingBar(void* gadget, void* lightbar, int index, unsigned char* prev, unsigned char* alphas, int slot, char* label, int y, int* rect)
{
    unsigned int color;
    int flash;
    color = g_game->colors[(*(volatile unsigned char(*)[6])&g_game->progress_38d6f)[index] < 100 ? 12 : 10];
    SetTextColors(color, GetTextKeyColor());
    if ((*(volatile unsigned char(*)[6])&g_game->progress_38d6f)[index] == 100 && *prev != 100) {
        alphas[slot] = 0x1e;
    }
    flash = alphas[slot];
    *prev = (*(volatile unsigned char(*)[6])&g_game->progress_38d6f)[index];
    DrawTextClipped(gadget, (char*)Translate(label), 0x5a, y, -1, flash);
    rect[0] = 0xcd;
    rect[2] = ((int)(*(volatile unsigned char(*)[6])&g_game->progress_38d6f)[index] * 7) / 2 + 0xcd;
    rect[1] = y;
    rect[3] = y + 0x14;
    FillRectangle((Surface*)gadget, (Rect*)rect, color);
    DrawFrame(gadget, lightbar, rect[0], rect[1]);
}
// FUNCTION: 0x497f40
void LoadingScreenFrame(void)
{
    Surface gadget;
    char buf[128];
    char aux[256];
    void* surfaceHandle;
    int i;
    unsigned int color;
    int textWidth;
    unsigned int stamp;
    int rect[4];
    LoadingPlayerInfo* pi;
    char namebuf[100];

    if (!(*(volatile union LoadingFlags*)&g_game->loadingFlags).bits.started) {
        while (((int)g_game->layer) != 0) {
            CloseTopScreen((Gui *)&g_game->gui);
        }
        DisableKeyCommands((Dialog *)&g_game->gui);
        if (((unsigned char)g_game->cursorMode) != 0x14) {
            g_game->cursorMode = (char)(0x14);
            SetCursorAnimation(&g_game->gui, (void*)g_game->cursorHourglass);
        }
        SetFont(g_game->fontComix);
        SetPaletteColors((unsigned char *)g_game->paletteRgba, 0, 0x100);
        if (g_seams.map->GetGameType(g_game->net) != 2) {
            SaveSettings();
        }
        while (((int)g_game->layer) != 0) {
            CloseTopScreen((Gui *)&g_game->gui);
        }
        BlankScreen();
        g_game->width_37e1f = 0x280;
        g_game->screenHeight = 0x1e0;
        if (GetScreenWidth() != 0x280 || GetScreenHeight() != 0x1e0) {
            GameFreeThunk((void*)g_game->screen);
            g_game->screen = 0;
            SetRestoreSurface(0);
            RestoreScreen();
            host_window_place(0, 0, 0x280, 0x1e0);
            SetResolution(0x280, 0x1e0);
            g_game->screen = (void*)AllocSurface("OFFSCREEN", g_game->width_37e1f, g_game->screenHeight);
            SetRestoreSurface((int)g_game->screen);
            SetOffscreenSurface((void*)g_game->screen);
        }
        BuildDataPath(aux, "palettes", "guipal", "PAL");
        surfaceHandle = HAPI_LoadFile(aux, 0);
        RemapPaletteToClosestIndices(&g_game->gui, g_game->paletteRgba, surfaceHandle);
        GameFreeThunk(surfaceHandle);
        g_game->lastTick = GetTicks();
        g_game->frames_38a3b = 0;
        g_game->ticks = 0;
        g_game->streak = 0;
        g_game->endGameCountdown = (short)0xffff;
        g_game->width_37e1f = g_game->width_37f1b;
        g_game->screenHeight = g_game->height_37f1f;
        g_game->viewCullMinX = 0x80;
        g_game->viewCullMinY = 0x20;
        g_game->viewCullMaxX = g_game->width_37e1f - 1;
        g_game->viewCullMaxY = g_game->screenHeight - 0x21;
        g_game->viewWidth = g_game->viewCullMaxX - g_game->viewCullMinX + 1;
        g_game->viewHeight = g_game->viewCullMaxY - g_game->viewCullMinY + 1;
        LoadPictureCached("loadgame2bg", 0, 0, 0);
        memset(&g_game->slots_29a4, 0, sizeof(g_game->slots_29a4));
        for (i = 0; i < 10; i++) {
            pi = &(*(LoadingPlayerRecord(*)[10])&g_game->players)[i];
            if (pi->active == 0 || (pi->control != 1 && pi->control != 2)) {
                g_game->slots_29a4.inGame[i] = 0;
            } else {
                g_game->slots_29a4.inGame[i] = 1;
            }
            g_game->slots_29a4.flag40[i] = (pi->active != 0 && (pi->data->flags_9b & 0x40) != 0) ? 1 : 0;
        }
        if (!host_thread_start(LoadThreadMain, 0, 0)) {
            FatalError("Unable to start the loading thread!");
        }
        memset(g_scorePanelKillFlash, 0, 10);
        memset(g_scorePanelLossFlash, 0, 10);
        memset(g_loadingBarFlashAlpha, 0, 6);
        memset(g_loadingBarPrevPercent, 0, 6);
        (*(volatile union LoadingFlags*)&g_game->loadingFlags).bits.started = 1;
        OnlineUnload();
    }
    if ((*(volatile union LoadingFlags*)&g_game->loadingFlags).bits.loaded) {
        StopAllSounds();
        BlankScreen();
        FreePictureCache();
        if (GetScreenWidth() != g_game->width_37f1b || GetScreenHeight() != g_game->height_37f1f) {
            GameFreeThunk((void*)g_game->screen);
            g_game->screen = 0;
            SetRestoreSurface(0);
            RestoreScreen();
            host_window_place(0, 0, g_game->width_37f1b, g_game->height_37f1f);
            SetResolution(g_game->width_37f1b, g_game->height_37f1f);
            g_game->screen = (void*)AllocSurface("OFFSCREEN", g_game->width_37e1f, g_game->screenHeight);
            SetRestoreSurface((int)g_game->screen);
        }
        DrawLightBars();
        MainLoopTick();
        ShowSoftwareCursor();
        g_game->mode_391f1 = 6;
        g_game->handler = BattleFrame;
        SetCloseHandler(HandleBattleQuitPrompt, 0);
        g_game->field_589 = 0;
        memset((void*)(*(volatile unsigned char(*)[6])&g_game->progress_38d6f), 0, 8);
        ((Sound*)g_game->sound)->SetTrackCategory(0);
        if (!((Sound*)g_game->sound)->IsCdPlaying()) {
            ((Sound*)g_game->sound)->PlayNextTrack();
        }
        for (i = 0; i < 10; i++) {
            if ((*(LoadingPlayerRecord(*)[10])&g_game->players)[i].active != 0)
                ((unsigned char*)rect)[19] = (*(LoadingPlayerRecord(*)[10])&g_game->players)[i].control;
        }
        return;
    }
    for (i = 0; i < 10; i++) {
        if ((*(LoadingPlayerRecord(*)[10])&g_game->players)[i].active != 0
            && ((*(LoadingPlayerRecord(*)[10])&g_game->players)[i].control == 1 || (*(LoadingPlayerRecord(*)[10])&g_game->players)[i].control == 2)) {
            SendProbe((*(LoadingPlayerRecord(*)[10])&g_game->players)[i].id, 0);
            if (g_usePacketManager != 0)
                (&g_packetManager)->SendAllQueued(1);
        }
    }
    HandleNetPackets();
    if ((*(volatile union LoadingFlags*)&g_game->loadingFlags).bits.b2) {
        if (AssignStartPositions() != 0) {
            (*(volatile union LoadingFlags*)&g_game->loadingFlags).bits.b2 = 0;
            (*(volatile union LoadingFlags*)&g_game->loadingFlags).bits.b3 = 1;
            host_net_guarantee_packets(0);
        }
    }
    if (g_usePacketManager != 0) {
        (&g_packetManager)->SendAllQueued(1);
    }
    SetOffscreenSurface((void*)g_game->screen);
    int ok = LockScreen(&gadget);
    if (ok != 0) {
        color = g_game->colors[15];
        stamp = GetTicks();
        if (g_loadingBarFlashDecayTick < (int)stamp) {
            g_loadingBarFlashDecayTick = GetTicks();
            for (i = 0; i < 6; i++) {
                if (g_loadingBarFlashAlpha[i] != 0) {
                    g_loadingBarFlashAlpha[i] -= 2;
                }
            }
        }
        SetFont(g_game->fontComix);
        DrawSurface(&gadget, (void*)((int)g_game->surface), 0, 0);
        if (g_seams.map->GetGameType(g_game->net) != 1) {
            SetTextColors(color, 0xfe);
            char* name = g_seams.map->GetMissionName(g_game->net);
            strncpy(namebuf, name, 100);
            namebuf[99] = 0;
            if (GetPreferredLanguage() != 0 && _strcmpi((const char*)GetPreferredLanguage(), "english") != 0) {
                _strlwr(namebuf);
            }
            sprintf(buf, "%s: %s", (char*)Translate("Map"), (char*)Translate(namebuf));
            textWidth = GetTextPixelWidth(buf);
            {
                int x = gadget.width / 2 - textWidth / 2;
                DrawTextClipped(&gadget, buf, x,
                             (int)((double)gadget.height - (double)GetFontHeight() * 1.5), -1, 0);
            }
        }
        {
            void* light = FindGafEntry(((int)g_game->gaf), "LIGHTBAR");
            void* lightbar = GetGafFrame(light, 0);
            *((short*)lightbar + 3) = 0;
            *((short*)lightbar + 2) = 0;
            DrawLoadingBar(&gadget, lightbar, 0, &g_loadingBarPrevPercent[0], g_loadingBarFlashAlpha, 0, "Textures", 0x87, rect);
            DrawLoadingBar(&gadget, lightbar, 1, &g_loadingBarPrevPercent[1], g_loadingBarFlashAlpha, 1, "Terrain", 0xb1, rect);
            DrawLoadingBar(&gadget, lightbar, 2, &g_loadingBarPrevPercent[2], g_loadingBarFlashAlpha, 2, "Units", 0xda, rect);
            DrawLoadingBar(&gadget, lightbar, 3, &g_loadingBarPrevPercent[3], g_loadingBarFlashAlpha, 3, "Animation", 0x106, rect);
            DrawLoadingBar(&gadget, lightbar, 4, &g_loadingBarPrevPercent[4], g_loadingBarFlashAlpha + 4, 0, "3D Data", 0x130, rect);
            DrawFrame(&gadget, lightbar, rect[0] + *((short*)lightbar + 2),
                         rect[1] + *((short*)lightbar + 3));
            DrawLoadingBar(&gadget, lightbar, 5, &g_loadingBarPrevPercent[5], g_loadingBarFlashAlpha + 4, 1, "Explosions", 0x15b, rect);
        }
        if (g_seams.map->GetGameType(g_game->net) == 3) {
            DrawSyncStatus(&gadget);
            SendLoadProgress();
        }
        UnlockScreen(&gadget);
        FlipScreen();
    }
    host_sleep_ms(200);
}
// FUNCTION: 0x498cd0
void __stdcall OffsetWorldPosFromView(Point* p, WorldViewOrigin* view, int param_3)
{
    g_seams.los_radar->ClampWorldPosToTerrain((view->x - 0x80) + p->x, (view->y - 0x20) + p->y, (Vec3*)param_3);
}
#include <string.h>
// FUNCTION: 0x498d00
void __stdcall MinimapCursorToWorldPos(MinimapWorldPos* out)
{
    ViewMouseEvent r = g_game->view;
    int dx = r.x - g_game->posX;
    int dy = r.y - g_game->posY;
    memset(out, 0, sizeof(MinimapWorldPos));
    out->x.whole = g_game->mapWidth * dx / g_game->width_142eb;
    out->z.whole = g_game->mapHeight * dy / g_game->screen_h;
    out->y.whole = g_seams.units->GetGroundHeight((Vec3*)out);
}

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

// FUNCTION: 0x498da0
void __stdcall UpdateCursorWorldPos(CursorMoveEvent* r)
{
    int mx, my;

    if (PointInRect(&g_game->viewLimit, r->x, r->y) && !((*(union CursorInputFlags*)&g_game->inputFlags_2cc6).value & 8)) {
        mx = (r->x - g_game->posX) * g_game->mapWidth / g_game->width_142eb;
        my = (r->y - g_game->posY) * g_game->mapHeight / g_game->screen_h;
        (*(union CursorInputFlags*)&g_game->inputFlags_2cc6).value |= 1;
        (*(union CursorInputFlags*)&g_game->inputFlags_2cc6).value &= ~2;
    } else {
        Rect* lim = &g_game->rect;
        mx = g_game->scrollX + MIN(MAX(r->x, lim->left), lim->right) - lim->left;
        my = g_game->scrollY + MIN(MAX(r->y, lim->top), lim->bottom) - lim->top;
        (*(union CursorInputFlags*)&g_game->inputFlags_2cc6).value &= ~1;
        (*(union CursorInputFlags*)&g_game->inputFlags_2cc6).bits.b1 = PointInRect(lim, r->x, r->y);
    }
    (*(union CursorInputFlags*)&g_game->inputFlags_2cc6).bits.b2 = (*(union CursorInputFlags*)&g_game->inputFlags_2cc6).bits.b0 || (*(union CursorInputFlags*)&g_game->inputFlags_2cc6).bits.b1;
    g_seams.los_radar->ClampWorldPosToTerrain(mx, my, (Vec3*)&(*(struct CursorWorldPos*)&g_game->pos));
    {
        CursorWorldPos* p = &(*(struct CursorWorldPos*)&g_game->pos);
        Point16 pt;
        CursorWorldPos v = *p;
        pt.x = (short)(v.x >> 20);
        pt.y = (short)(v.z >> 20);
        (*(struct Point16*)&g_game->point) = pt;
    }
    g_game->cellFeature = g_seams.features->GetCellFeature(g_seams.los_radar->GetMapCell((*(struct Point16*)&g_game->point).x, (*(struct Point16*)&g_game->point).y));
}
