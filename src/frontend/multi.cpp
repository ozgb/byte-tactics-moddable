#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "win_types.h"
#include "../../interfaces/ref.h"
#include <math.h>
#include "../../interfaces/host.h"
#include "../units/unit_def.h"
#include "../../interfaces/seams.h"

struct Gadget;
struct Layer;
struct Gui;
struct Game;
struct UnitRestrictEntry;
struct PlayerInfo;
struct LogoTable;

#include "../gui/gadget.h"

#pragma pack(push, 1)

struct GameAdvertGroup {
    unsigned int a;
    unsigned int b;
    unsigned int c;
    unsigned int d;
};

struct GameSelectionMsg {
    char pad_0[0x99];
    GameAdvertGroup group;
    char pad_1[0xb9 - 0xa9];
};

struct SelectGameMsg {
    char pad_0[0x99];
    GameAdvertGroup group;
    char pad_1[0x13];
};

struct NetLobbyBlockView {
    void* dp;
    void* dp3;
    char unknown_8[0x431 - 8];
    GUID* guids;
    char unknown_435[0x4b9 - 0x435];
    void* lobby;
    char unknown_4bd[0x4e5 - 0x4bd];
    int count;
    int gameCount;
    char unknown_4ed[0x505 - 0x4ed];
};

struct GameDescEntry {
    char unknown_0[0x4c];
    int size;
    int offset;
};

struct ModemAccount {
    char name[0x81];
    char number[0x81];
};

struct SerialPortConfig {
    int unknown_0;
    char unknown_4[4];
    unsigned long unknown_8;
    unsigned long unknown_c;
    unsigned long unknown_10;
};

struct CompoundAddressElement {
    GUID guid;
    unsigned long size;
    void* data;
};

struct GameSessionSettings {
    unsigned short memory;
    unsigned short players : 4;
    unsigned short playing : 1;
    unsigned short pad5 : 3;
    unsigned short black : 1;
    unsigned short nocmd : 1;
    unsigned short pad10 : 1;
    unsigned short mode : 2;
    unsigned short pad13 : 2;
    unsigned short lock : 1;
    unsigned short allyWinFlags;
    unsigned short pingLimit;
    unsigned short energy;
    unsigned short metal;
    unsigned short maxUnits;
    unsigned short version;
};

struct GameSessionRecord {
    GameSessionSettings settings;
    int maxPlayers;
    char name[0x20];
    char name2[0x20];
};

#include "../network/link_info.h"

#include "../network/player_info.h"

#include "../network/player.h"

#include "../map/mission.h"

#include "../gui/layer.h"
typedef Layer SelectDialogLayer;
typedef Layer ListDialogLayer;

#include "../gui/gui.h"

#include "../game/game.h"

void __stdcall SetGadgetText(void* gui, int index, char* text);
#include "multi.h"
// GLOBAL: 0x511de8
extern Game* g_game;
extern char g_onlineLobbyPassword;
extern int DAT_00512c80;
extern GUID g_dpspGuidModem;
extern GUID g_dpspGuidTcpip;
extern GUID g_dpspGuidIpx;
extern GUID g_dpspGuidSerial;
int __stdcall FindGadgetIndex(void* entries, const char* name, int flag);
void __stdcall BlitMenuLayers(void* menu, int a, int b);
void __stdcall SetGadgetActiveByName(void* menu, const char* name, int value);
#include "../gui/dialogs.h"
char* __stdcall Translate(const char* text);
void __stdcall SetOffscreenSurface(int a);
void FlipScreen();
#include "../gui/keyboard.h"
#include "../../include/v4i.h"
#include "../graphics/display.h"
#include "../debug/debug_lib.h"
#include "../network/online.h"
#include "../game/main.h"
Gadget* __stdcall FindGadgetChecked(void* entries, const char* name);
Gadget* __stdcall FindGadgetOrNull(void* entries, const char* name);
Gadget* __stdcall FindGadgetChecked_B(Gadget* entries, const char* name);
Gadget* __stdcall FindGadgetChecked_B(void* entries, const char* name);
Gadget* __stdcall FindGadgetChecked_C(void* entries, const char* name);
Gadget* __stdcall FindGadgetChecked_D(void* entries, char* name);
Gadget* __stdcall FindGadgetChecked_E(void* entries, char* name);

static inline void SetGadgetEnabledBitInUpdateGameSelection(unsigned short* p, unsigned short r, int ge, Gadget* entry)
{
    unsigned short on = (unsigned short)(((entry->u.list.rowCount == 0) | (ge == 0)) & 1);
    *p = (unsigned short)((r | on) | (*p & 0xfffe));
}

static int HOST_CALL PollGetGames(void)
{
    DispatchHostEvents();
    if (PeekKey()) {
        if (PopKey() == 0x1b)
            return 1;
    }
    return 0;
}

// FUNCTION: 0x441220
void __stdcall UpdateGameSelection(Gui* menu, Gadget* entry)
{
    void* gadgets = menu->layer->entries;
    int gv; int& ge = gv;
    unsigned short* p;
    Gadget* gd;
    unsigned short on;
    unsigned short r;
    int idx = entry->u.list.selected;
    GameAdvertGroup* rec = (GameAdvertGroup*)((char*)entry->u.list.records + idx * 0x54 + 4);
    GameSelectionMsg msg;
    msg.group = *rec;
    memcpy(g_game->buffer_2ab1, &msg, 185);

    unsigned short flags = *(unsigned short*)((char*)&msg.group + 2);
    int value = *(int*)((char*)&msg.group + 0xe);

    int index = FindGadgetIndex(gadgets, "WATCH", 1);
    if (index != -1) {
        gd = (Gadget*)((char*)gadgets + index * 0x15b);
        gv = ((value & 0xff) >= g_game->version) ? 1 : 0;
        p = (unsigned short*)((char*)gd + 0x13c);
        r = (unsigned short)((~flags & 0x80) | (flags >> 8));
        r >>= 3;
        r |= flags & 0x10;
        r >>= 4;
        SetGadgetEnabledBitInUpdateGameSelection(p, r, ge, entry);
    }
    index = FindGadgetIndex(gadgets, "JOINGAME", 1);
    if (index != -1) {
        gd = (Gadget*)((char*)gadgets + index * 0x15b);
        gv = ((value & 0xff) >= g_game->version) ? 1 : 0;
        p = (unsigned short*)((char*)gd + 0x13c);
        r = (unsigned short)((flags >> 11) | (flags & 0x10));
        r >>= 4;
        SetGadgetEnabledBitInUpdateGameSelection(p, r, ge, entry);
    }

    int password = msg.group.b & 1;
    SetGadgetActiveByName(&g_game->gui, "PASSWORDTEXT", password);
    SetGadgetActiveByName(&g_game->gui, "PASSWORD", password);
    MarkChanged((Dialog *)menu);
}

// FUNCTION: 0x441460
int __stdcall ConnectToGame(Layer* gadget) {
    int count;
    int left;
    int i;
    char* p[21];
    char names[0x20];
    char buf[0x80];
    struct { char pre[0x99]; GameSessionSettings s; char pad[0x13]; } sb;
    const char* msg;
    char* lang;
#define temp (buf)

#define PE(g) (memcmp(&g_game->info.guid, &(g), 0x10) == 0)
    if (!PE(g_dpspGuidModem) && !PE(g_dpspGuidTcpip)) {
        if (PE(g_dpspGuidIpx))
            goto upd;
        if (PE(g_dpspGuidSerial))
            ;
        else
            *(int*)&p[20] = memcmp(&g_game->info.guid, &g_dpspGuidSerial, 0x10) != 0;
    }
    if (PE(g_dpspGuidModem))
        goto conn;
    if (!PE(g_dpspGuidTcpip)) {
        if (!PE(g_dpspGuidIpx)) {
            if (PE(g_dpspGuidSerial))
                ;
            else
                *(int*)&p[20] = memcmp(&g_game->info.guid, &g_dpspGuidSerial, 0x10) != 0;
        }
        goto conn;
    }
upd:
    msg = "Updating...";
    goto shown;
conn:
    msg = "Connecting  (ESC to abort)";
shown:
    OpenMessageBox(&g_game->gui, Translate(msg), 0x96, 0, 1);
    BlitMenuLayers(&g_game->gui, (int)g_game->screen, 0);
    SetOffscreenSurface((int)g_game->screen);
    FlipScreen();
    FlipScreen();

    count = host_net_get_games((host_net*)((char*)g_game + 0x14), g_game->desc, PollGetGames);
    CloseTopScreen(&g_game->gui);
    if (count < 0)
        return 0;

    i = 0;
    do {
        i++;
        p[i] = (char*)g_game->data16[i];
        memset(p[i], 0, 0xa00);
    } while (i < 15);

    char* dsc = (char*)g_game->desc;
    if (count > 0) {
        p[0] = dsc + 0x18;
        left = count;
        do {
            char* e;
            sb.s = *(GameSessionSettings*)(p[0] - 0x14);
            memcpy(names, p[0], 0x20);

            strncpy(p[1], names, 0x10);
            p[1][0x10] = 0;
            p[1] += strlen(p[1]) + 1;
            sprintf(p[2], "%d/%d", sb.s.players, ((GameSessionRecord*)(p[0] - 0x14))->maxPlayers);
            p[2] += strlen(p[2]) + 1;

            memset(temp, 0, 0x80);
            strncpy(temp, names + 0x10, 0xf);
            e = temp + strlen(temp);
            while (e != temp) {
                e--;
                if (*e != ' ')
                    break;
                *e = 0;
            }
            if (GetPreferredLanguage() != 0) {
                if (_strcmpi((const char *)GetPreferredLanguage(), "english") != 0) {
                    _strlwr(temp);
                    lang = Translate(temp);
                    strncpy(temp, lang, 0x80);
                    temp[0x7f] = 0;
                }
            }
            strcpy(p[3], temp);
            p[3] += strlen(p[3]) + 1;

            if ((sb.s.version & 0xff) >= g_game->version) {
                if (sb.s.lock)
                    msg = "Lock";
                else if (sb.s.playing)
                    msg = "Play";
                else
                    msg = "Open";
                sprintf(p[4], "%s", Translate(msg));
            } else {
                sprintf(p[4], "%s", Translate("VER!"));
            }
            p[4] += strlen(p[4]) + 1;
            sprintf(p[5], "%d", sb.s.memory);
            p[5] += strlen(p[5]) + 1;
            sprintf(p[6], "%d", sb.s.metal * 100);
            p[6] += strlen(p[6]) + 1;
            sprintf(p[7], "%d", sb.s.energy * 100);
            p[7] += strlen(p[7]) + 1;
            sprintf(p[8], "%d", sb.s.pingLimit);
            p[8] += strlen(p[8]) + 1;

            if (sb.s.mode != 0) {
                if (sb.s.mode == 1)
                    msg = "Yes";
                else
                    msg = "DM";
            } else {
                msg = "No";
            }
            sprintf(p[9], "%s", Translate(msg));
            p[9] += strlen(p[9]) + 1;

            sprintf(p[10], "%s", Translate(sb.s.black ? "Blk" : "Gray"));
            p[10] += strlen(p[10]) + 1;
            sprintf(p[11], "%s", Translate(sb.s.nocmd ? "No" : "Yes"));
            p[11] += strlen(p[11]) + 1;

            p[0] += 0x54;
        } while (--left);
    }

    ConfigureListBoxByName(&g_game->gui, "GAMENAME", (char*)g_game->data16[1], (*(NetLobbyBlockView*)((char*)g_game + 0x14)).gameCount, 0);
    ConfigureListBoxByName(&g_game->gui, "PLAYERS", (char*)g_game->data16[2], (*(NetLobbyBlockView*)((char*)g_game + 0x14)).gameCount, 0);
    ConfigureListBoxByName(&g_game->gui, "MAPNAME", (char*)g_game->data16[3], (*(NetLobbyBlockView*)((char*)g_game + 0x14)).gameCount, 0);
    ConfigureListBoxByName(&g_game->gui, "STATUS", (char*)g_game->data16[4], (*(NetLobbyBlockView*)((char*)g_game + 0x14)).gameCount, 0);
    ConfigureListBoxByName(&g_game->gui, "METAL", (char*)g_game->data16[6], (*(NetLobbyBlockView*)((char*)g_game + 0x14)).gameCount, 0);
    ConfigureListBoxByName(&g_game->gui, "ENERGY", (char*)g_game->data16[7], (*(NetLobbyBlockView*)((char*)g_game + 0x14)).gameCount, 0);
    ConfigureListBoxByName(&g_game->gui, "COMMANDER", (char*)g_game->data16[9], (*(NetLobbyBlockView*)((char*)g_game + 0x14)).gameCount, 0);
    ConfigureListBoxByName(&g_game->gui, "LOS", (char*)g_game->data16[11], (*(NetLobbyBlockView*)((char*)g_game + 0x14)).gameCount, 0);
    ConfigureListBoxByName(&g_game->gui, "PING", (char*)g_game->data16[8], (*(NetLobbyBlockView*)((char*)g_game + 0x14)).gameCount, 0);
    ConfigureListBoxByName(&g_game->gui, "FULLMAP", (char*)g_game->data16[10], (*(NetLobbyBlockView*)((char*)g_game + 0x14)).gameCount, 0);

    i = FindGadgetIndex(gadget->entries, "GAMENAME", 2);
    if (i != -1)
        UpdateGameSelection(&g_game->gui, gadget->entries + i);
    return 1;
}
#undef PE
#undef temp

struct LogoFrameEntry {
    void* ptr;
    int unknown_4;
};

struct LogoTable {
    unsigned short count;
    char unknown_2[0x28 - 0x2];
    LogoFrameEntry entries[1];
};

struct MapSelectData {
    char unknown_0[0x14];
    char* items;
};

struct MapSelectClickLayout {
    char unknown_0[0x14];
    void* items;
};

struct LogoAnimSeq {
    char unknown_0[0x28];
    void* frame;
    char unknown_2c[0x30 - 0x2c];
};

struct LogoSelectLayout {
    char selected[0x18];
    void** ptrList;
    LogoAnimSeq* seqs;
};

#include "../../include/display_mode.h"

struct ModeList {
    int count;
    DisplayMode* modes;
    char unknown_8[0x14 - 0x8];
    char* available;
    char unknown_18[0x20 - 0x18];
};

struct PlayerSlotGadgetCopy {
    unsigned char state;
    char team;
    char name[0x13];
    short y;
    char unknown_17[2];
    short height;
    int attribs;
    char unknown_1f[0x29 - 0x1f];
    unsigned char active;
    char unknown_2a[0xb6 - 0x2a];
    char text[0x13e - 0xb6];
};

struct UnitRestrictEntry {
    char name[0x52];
    int unitIndex;
    int previousMax;
    int max;
    int peerEnabled;
};

struct UnitRestrictRecord {
    unsigned short a;
    unsigned short b;
    unsigned short e;
    unsigned short f;
    unsigned char flag8;
    unsigned char flag9;
    unsigned char flaga;
    unsigned char flagb;
    int reserved;
    int d;
    int scratch;
};

#pragma pack(pop)

#include "../orders/order_fx.h"

#include "../network/packet_manager.h"
#include "../state/sim_globals.h"
#include "../quirks/quirks.h"

struct Dialog;
struct UnitRestrictPicView;

void RegisterUnitOrders();

typedef void (__stdcall* BattleRoomSliderCallback)(Gui* gui, int index);
typedef void (__stdcall* UnitRestrictSliderCallback)(Gui* gui, int index);

// GLOBAL: 0x512c84
extern int g_cmdlineHostMode;
// GLOBAL: 0x512d90
extern char g_cmdlineTcpJoinAddress[];
extern int g_unitRestrictPicLoadIndex;
extern int g_unitRestrictPicCursor;
extern int g_unitRestrictRecordCursor;
extern int g_startCountdownNextTick;
extern unsigned int g_heartbeatNextTick;
extern char* g_saveListFileNames;
extern char* g_saveListDisplayNames;
extern int* g_unitRestrictPics;
extern int DAT_005129c0;
extern int* g_unitRestrictOldCounts;
// GLOBAL: 0x5129c8
extern int g_unitRestrictNextPicTick;
extern unsigned int g_lastPlayerCount;
extern char* g_savegameDir;
extern char g_star[];
extern char g_lstExtension[];
extern char g_gamesGadgetName[];
extern char g_savegameNamesName[];
extern char g_savegameDescsName[];
extern char* g_hostOnlyGadgets[];
extern char* g_battleRoomGadgetNames[];
extern char g_lobbyMapName[];
extern int g_lobbyLock;
extern int g_lobbyMaxUnits;
extern int g_lobbyEnergy;
extern int g_lobbyMetal;
extern int g_lobbyCommander;
extern int g_lobbyLineOfSight;
extern int g_lobbyCheating;
extern int g_lobbyFixedLoc;
extern int g_lobbyMapping;
extern int g_lobbyWatching;
extern int g_usePacketManager;
// GLOBAL: 0x5129b4
extern UnitRestrictEntry* g_unitRestrictEntries;
extern char g_onlineLobbyPlayerName;
extern char* g_modemInfo;
extern int g_modemCount;
extern ModemAccount* g_modemAccounts;
extern char* g_modemAccountNames;
extern GUID DPAID_Modem;
extern GUID DPAID_ServiceProvider;
extern GUID DPAID_Phone;
extern GUID DPAID_INet;
extern GUID DPAID_ComPort;
extern char DAT_004fcfb8[];
extern SerialPortConfig g_serialPortNumber;
extern int g_serialBaudRate;
extern int g_serialBaudRateTable[];
extern unsigned int g_reporterCount;
extern unsigned int g_reporterNames[16];
extern LinkInfo g_linkInfo[];
extern char g_okGadgetName[];
extern char g_multiSoundName[];
extern char* g_oldMapName;
extern int DAT_00512760;
extern int g_battleRoomBaseGadgetCount;
extern int DAT_0051276c;
extern int g_battleRoomSlotsBuilt;
extern int g_rejectPlayer;

int __stdcall TrySetFocus(void* menu, int index);
void __stdcall SetKeyboardInput(void* menu, int value);
void __stdcall SetDescListCleanupFlag(void* gui, int flag);
void __stdcall SetTranslatedTextByName(void* menu, const char* name, char* text, int param_4);
void __stdcall SetGrayedOutByName(void* menu, const char* name, int value);
void __stdcall SetGrayedOutByName(void* gui, char* name, int value);
void __stdcall SetTranslatedText(void* menu, int index, int param_3, int param_4);
void __stdcall SetGadgetGrayedOutByName(void* menu, char* name, int value);
char* __stdcall GetGadgetText(void* menu, const char* key, char* out);
int __stdcall GetGadgetStatus(void* menu, int handle);
char __stdcall GetGadgetActiveByName(void* menu, char* name);
int __stdcall GetButtonStage(void* gadget, int index);
int __stdcall GetButtonStageByName(void* gadget, char* name);
void __stdcall SetButtonStageByName(void* gui, char* name, int value);
int __stdcall SetButtonStageByName(Gui* obj, char* name, int value);
void __stdcall SetGadgetStatusByName(void* gui, char* name, int value);
void __stdcall SetGadgetRows(void* table, char* name, int* pics, int count);
void __stdcall DrawButton(void* gadget, int value);
void __stdcall LoadPictureCached(const char* name, int a, int b, int c);
Layer* __stdcall LoadGuiLayer(void* menu, const char* name, int flags);
void __stdcall PlaySoundByName(const char* name, int flag);
void __stdcall PlaySoundByName(char* str, int flag);
#include "frontend.h"
char* __stdcall Translate(char* text);
void __stdcall SetFrontendState(int state, int line, const char* file);
void __stdcall SetFrontendState(int a, int line, char* file);
#include "frontend.h"
#include "../game/settings.h"
void __stdcall SetCursorMode(int value);
#include "../game/statistics.h"
#include "../graphics/object_pool.h"
int __stdcall LoadReporterDll(unsigned int* a, unsigned int* b);
unsigned char FindHostSlot();
#include "../network/net_game.h"
#include "../graphics/palette.h"
void __stdcall RemapPaletteToClosestIndices(void* menu, void* palette, void* param_3);
#include "../network/net_game.h"

#include "../game/data_files.h"
#include "../map/radar.h"
#include "../util/registry.h"
#include "../network/unit_sync.h"
#include "../network/unit_sync_state.h"
#include "../map/map_list.h"
#include "../gui/file_list.h"
void __stdcall SetGadgetItems(void* gui, const char* name, void** items, int count);
#include "options.h"
int __stdcall IsGadgetNamed(int param1, int param2, char* name);
void __stdcall RejectPlayer(int param_1, int param_2);
int __stdcall GetDisplayModes(ModeList* obj);
void __stdcall SortDisplayModes(ModeList* obj);
void __stdcall SetAlliance(int, int, int, int);
void __stdcall SetAlliance(int a, int b, unsigned char allied, int d);

#include "../game/game_state.h"
void* __stdcall LoadBitmapByName(char* name, unsigned char* palette);
void __stdcall DrawSurface(void* dest, void* image, int x, int y);
#include "../graphics/surface.h"
void ShowSoftwareCursor();
#include "../files/files_api.h"
int __stdcall ScanDirectory(char* path, void* buffer, char* p3, int p4, int p5, int p6);
extern "C" __declspec(dllimport) char* __stdcall lstrcpynA(char*, const char*, int);
void* __stdcall LoadPcx(char* path, int param_2);
void __stdcall FrameFromSurface(void* dst, void* src);
void* __stdcall FindGafEntry(void* gaf, const char* name);
unsigned int GetTicks();
void __stdcall AddMessage(char* text, int a, int b, int c);
void __stdcall SendChatMessage(void* from, char* text, int a, int b);
int __stdcall GetTextPixelWidth(char* text);

// FUNCTION: 0x440d70
void __stdcall HandleNewMultiClick(Gui* gadget)
{
    Gadget* entries = gadget->layer->entries;
    if (gadget->hotGadgetIndex == -1)
        return;
    if (IsCurrentGadgetNamed((Dialog *)gadget, "GAMENAME")) {
        SelectGadgetByIndex(gadget, FindGadgetIndex(entries, "NICKNAME", 3));
        TrySetFocus(gadget, FindGadgetIndex(entries, "NICKNAME", 3));
        MarkChanged((Dialog *)gadget);
        ClearSelectedGadget(gadget);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gadget, "NICKNAME")) {
        SelectGadgetByIndex(gadget, FindGadgetIndex(entries, "PASSWORD", 3));
        TrySetFocus(gadget, FindGadgetIndex(entries, "PASSWORD", 3));
        MarkChanged((Dialog *)gadget);
        ClearSelectedGadget(gadget);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gadget, "PASSWORD")) {
        SelectGadgetByIndex(gadget, FindGadgetIndex(entries, "OK", 1));
        MarkChanged((Dialog *)gadget);
        ClearSelectedGadget(gadget);
        return;
    }
    if (FindGadgetIndex(entries, "OK", 0xe) == gadget->hotGadgetIndex) {
        char nickbuf[100];
        char namebuf[100];
        char* dst;
        int gi;
        int ni;
        PlaySoundByName("BigButton", 0);
        char* pw = (char*)FindGadgetChecked_B(entries, "PASSWORD");
        g_game->password[0] = 0;
        strncat(g_game->password, pw + 0xb6, 0xa);
        gi = FindGadgetIndex(entries, "GAMENAME", 3);
        dst = namebuf;
        strcpy(dst, entries[gi].u.text);
        if (strlen(namebuf) == 0) {
            BeginTextEdit(gadget, gi);
            ClearSelectedGadget(gadget);
            OpenMessageBox((Gui *)(char*)gadget, Translate("You must enter a game name"), 0x140, 1, 1);
            return;
        }
        ni = FindGadgetIndex(entries, "NICKNAME", 3);
        strcpy(nickbuf, entries[ni].u.text);
        if (strlen(nickbuf) == 0) {
            BeginTextEdit(gadget, ni);
            ClearSelectedGadget(gadget);
            OpenMessageBox((Gui *)(char*)gadget, Translate("You must enter your name"), 0x140, 1, 1);
            return;
        }
        strcpy(g_game->gameName_2bc1, namebuf);
        strcpy(g_game->nickname, nickbuf);
        if (!InitScoreReporting()) {
            g_game->frontendNextSubState = 0x11;
            return;
        }
        ClearSelectedGadget(gadget);
        return;
    }
    if (FindGadgetIndex(entries, "CANCEL", 0xe) == gadget->hotGadgetIndex) {
        PlaySoundByName("Previous", 0);
        CloseTopScreen(gadget);
        OpenSelectGameDialog();
        return;
    }
    ClearSelectedGadget(gadget);
}

// FUNCTION: 0x441080
void OpenNewMultiDialog()
{
    Layer* layer = LoadGuiLayer(&g_game->gui, "NEWMULTI.GUI", 0x80);
    layer->handler = HandleNewMultiClick;
    layer->data = g_game;
    LoadPictureCached("createnew", 0, 0, 0);
    Gadget* entries = layer->entries;
    if (IsOnlineConfigLoaded() && g_onlineLobbyPlayerName != 0) {
        g_game->nickname[0] = 0;
        strncat(g_game->nickname, &g_onlineLobbyPlayerName, 0x10);
    }
    if (strlen(g_game->nickname) == 0) {
        host_user_name(g_game->nickname, 0x11);
    }
    Gadget* gname = FindGadgetChecked_B(entries, "GAMENAME");
    SetTranslatedTextByName(&g_game->gui, "GAMENAME", g_game->gameName_2bc1, 0);
    gname->status = 0x10;
    Gadget* nname = FindGadgetChecked_B(entries, "NICKNAME");
    SetTranslatedTextByName(&g_game->gui, "NICKNAME", g_game->nickname, 0);
    nname->status = 0x10;
    char* pw = g_game->players[g_game->localPlayer].info->password;
    if (strlen(pw) == 0)
        pw = g_game->password;
    SetTranslatedTextByName(&g_game->gui, "PASSWORD", pw, 0xa);
    OrLabelAttribs();
    SetKeyboardInput(&g_game->gui, 1);
    RenderLayer(&g_game->gui, 0x40);
}

// FUNCTION: 0x441430
int __cdecl GetLocalPlayerPassword()
{
    int ecx = (int)g_game;
    unsigned int edx = *(unsigned char*)(ecx + 0x2a42);
    int eax = edx;
    ecx = ecx + edx;
    eax = eax << 5;
    eax = eax + edx;
    eax = eax + eax * 4;
    eax = (int)(*(Ref<char>*)(ecx + eax * 2 + 0x1b8a)).get();
    return eax + 0x80;
}

// FUNCTION: 0x441bc0
int GetServiceProviderIndex()
{
    GUID* guid = (GUID*)&g_game->info.guid;
    if (memcmp(guid, &g_dpspGuidModem, sizeof(GUID)) == 0) {
        return 0;
    }
    if (memcmp(guid, &g_dpspGuidTcpip, sizeof(GUID)) == 0) {
        return 1;
    }
    if (memcmp(guid, &g_dpspGuidIpx, sizeof(GUID)) == 0) {
        return 2;
    }
    if (memcmp(guid, &g_dpspGuidSerial, sizeof(GUID)) == 0) {
        return 3;
    }
    return 4;
}

// FUNCTION: 0x441c30
int __stdcall BuildCompoundAddress(int* addressOut, int* sizeOut)
{
    void* block = 0;
    unsigned long size = 0;
    CompoundAddressElement elements[3];
    GUID guid;
    char buf1[200];
    char buf2[200];
    char buf3[200];
    unsigned long count;
    int result;

    guid = (*(GUID*)&g_game->info.guid);

    if (memcmp(&guid, &g_dpspGuidModem, sizeof(GUID)) == 0) {
        elements[0].guid = DPAID_ServiceProvider;
        elements[0].size = 0x10;
        elements[0].data = &g_dpspGuidModem;
        memset(buf1, 0, sizeof(buf1));
        char* s = g_modemInfo;
        if (s == 0) {
            s = DAT_005119b8;
        }
        strcpy(buf1, s);
        elements[1].guid = DPAID_Modem;
        elements[1].size = strlen(buf1) + 1;
        elements[1].data = buf1;
        strcpy(buf2, GetGadgetText(&g_game->gui, "NUMBER", 0));
        elements[2].guid = DPAID_Phone;
        elements[2].size = strlen(buf2) + 1;
        elements[2].data = buf2;
        count = 3;
    } else if (memcmp(&guid, &g_dpspGuidTcpip, sizeof(GUID)) == 0) {
        elements[0].guid = DPAID_ServiceProvider;
        elements[0].size = 0x10;
        elements[0].data = &g_dpspGuidTcpip;
        char* t = GetGadgetText(&g_game->gui, "ADDRESS", 0);
        if (t == 0) {
            t = DAT_005119b8;
        }
        strcpy(buf3, t);
        elements[1].guid = DPAID_INet;
        elements[1].size = strlen(buf3) + 1;
        elements[1].data = buf3;
        count = 2;
    } else if (memcmp(&guid, &g_dpspGuidIpx, sizeof(GUID)) == 0) {
        elements[0].guid = DPAID_ServiceProvider;
        elements[0].size = 0x10;
        elements[0].data = &g_dpspGuidIpx;
        count = 1;
    } else if (memcmp(&guid, &g_dpspGuidSerial, sizeof(GUID)) == 0) {
        elements[0].guid = DPAID_ServiceProvider;
        elements[0].size = 0x10;
        elements[0].data = &g_dpspGuidSerial;
        g_serialPortNumber.unknown_8 = 0;
        g_serialPortNumber.unknown_c = 0;
        g_serialPortNumber.unknown_10 = 3;
        elements[1].guid = DPAID_ComPort;
        elements[1].size = 0x14;
        elements[1].data = &g_serialPortNumber;
        count = 2;
    } else {
        elements[0].guid = DPAID_ServiceProvider;
        elements[0].size = 0x10;
        elements[0].data = &guid;
        count = 1;
    }

    result = host_net_create_compound_address((host_net*)((char*)g_game + 0x14), elements, count, 0, &size);
    if (result != 0x8877001e) goto cleanup;
    block = GameAllocIgnoreTag("COMPOUND ADDR", size);
    if (block == 0) {
        result = 0x8007000e;
        goto cleanup;
    }
    result = host_net_create_compound_address((host_net*)((char*)g_game + 0x14), elements, count, block, &size);
    if (result < 0) {
cleanup:
        if (block != 0) {
            GameFreeThunk(block);
        }
        return result;
    }
    *addressOut = (int)block;
    *sizeOut = size;
    return 0;
}

// FUNCTION: 0x442000
int __stdcall TryConnect(int unused)
{
    int a = 0;
    int b = 0;
    int result = BuildCompoundAddress(&a, &b);
    if (result >= 0) {
        g_game->info.conn.data = (void*)a;
        result = 0;
        g_game->info.conn.size = b;
    }
    return result;
}

static int BuildAddressForTcpDialog()
{
    int a = 0;
    int b = 0;
    int result = BuildCompoundAddress(&a, &b);
    if (result >= 0) {
        g_game->info.conn.data = (void*)a;
        g_game->info.conn.size = b;
    }
    return result;
}

// FUNCTION: 0x442050
void __stdcall HandleTcpDialogClick(Gui* gadget)
{
    Gadget* entries = gadget->layer->entries;
    if (g_cmdlineTcpJoinAddress[0] != 0) {
        if (g_cmdlineHostMode == 0)
            g_cmdlineTcpJoinAddress[0] = 0;
    }
    else if (gadget->hotGadgetIndex == -1) {
        return;
    }
    else if (FindGadgetIndex(entries, "OK", 0xe) == gadget->hotGadgetIndex) {
        goto connect;
    }
    else if (FindGadgetIndex(entries, "JOIN", 0xe) == gadget->hotGadgetIndex) {
        g_game->bit1 = 1;
        g_game->bit0 = 0;
        BuildAddressForTcpDialog();
        PlaySoundByName("Smlbutton", 0);
        goto tcpaddr;
    }
    else if (IsCurrentGadgetNamed((Dialog *)gadget, "ADDRESS") != 0) {
    }
    else {
        if (IsCurrentGadgetNamed((Dialog *)gadget, "PREV") != 0) {
            SetFrontendState(0xf, 0x305, "c:\\cavedog\\wargame\\multi.cpp");
            PlaySoundByName("Previous", 0);
            return;
        }
        ClearSelectedGadget(gadget);
        return;
    }
connect:
    g_game->bit0 = 1;
    g_game->bit1 = 1;
    BuildAddressForTcpDialog();
    PlaySoundByName("Smlbutton", 0);
    BlankScreen();
tcpaddr:
    WriteGameRegistryValue((void*)"TCPADDR", GetGadgetText(gadget, "ADDRESS", 0), 0x80);
}

// FUNCTION: 0x4421f0
void OpenTcpDialog()
{
    Layer* dialog = LoadGuiLayer(&g_game->gui, "TCP.GUI", 0x800);
    dialog->handler = HandleTcpDialogClick;
    dialog->data = g_game;
    dialog->cb1c = 0;
    LoadPictureCached(0, 0, 0, 0);
    host_net_init_lobbied_connection((host_net*)((char*)g_game + 0x14));
    FindGadgetIndex(dialog->entries, "ADDRESS", 3);
    char* address = GetGadgetText(&g_game->gui, "ADDRESS", 0);
    int direct = g_cmdlineTcpJoinAddress[0];
    unsigned int len = 0x80;
    if (direct) {
        address[0] = 0;
        strncat(address, g_cmdlineTcpJoinAddress, len - 1);
    } else {
        if (!ReadGameRegistryValue("TCPADDR", address, &len)) {
            address[0] = 0;
        }
    }
    SetTranslatedTextByName(&g_game->gui, "ADDRESS", address, 0);
    if (direct) {
        HandleTcpDialogClick(&g_game->gui);
        g_game->dplayAddressDialogFlags_2aaf = g_game->dplayAddressDialogFlags_2aaf ^ ((g_cmdlineHostMode != 0) ^ g_game->dplayAddressDialogFlags_2aaf) & 1;
    } else {
        SelectGadgetByIndex(&g_game->gui, FindGadgetIndex(dialog->entries, "ADDRESS", 3));
        TrySetFocus(&g_game->gui, FindGadgetIndex(dialog->entries, "ADDRESS", 3));
        SetKeyboardInput(&g_game->gui, 1);
        RenderLayer(&g_game->gui, 0x40);
    }
}

#pragma auto_inline(off)
// FUNCTION: 0x442380
void __stdcall SetSerialBaudFromGadget(Gui* menu, Gadget* entry)
{
    int value = entry->u.list.selected;
    if (value >= 0) {
        g_serialBaudRate = g_serialBaudRateTable[value];
    }
}

// FUNCTION: 0x4423a0
void __stdcall SetSerialPortFromGadget(Gui* menu, Gadget* entry)
{
    int val = entry->u.list.selected;
    if (val >= 0) {
        g_serialPortNumber.unknown_0 = val + 1;
    }
}
#pragma auto_inline(on)

// FUNCTION: 0x4423c0
void __stdcall HandleSerialDialogClick(Gui* gadget)
{
    Gadget* entries = gadget->layer->entries;
    int a;
    int r;
    if (gadget->hotGadgetIndex == -1)
        return;
    if (FindGadgetIndex(entries, "HOST", 0xe) == gadget->hotGadgetIndex) {
        g_game->bit0 = 1;
        g_game->bit1 = 1;
        a = 0;
        int b = 0;
        r = BuildCompoundAddress(&a, &b);
        if (r >= 0) {
            g_game->info.conn.data = (void*)a;
            g_game->info.conn.size = b;
        }
        PlaySoundByName("SMLBUTTON", 0);
        BlankScreen();
    } else if (FindGadgetIndex(entries, "JOIN", 0xe) == gadget->hotGadgetIndex) {
        g_game->bit1 = 1;
        g_game->bit0 = 0;
        int a = 0, b = 0;
        r = BuildCompoundAddress(&a, &b);
        if (r >= 0) {
            g_game->info.conn.data = (void*)a;
            g_game->info.conn.size = b;
        }
        PlaySoundByName("SMLBUTTON", 0);
    } else if (IsCurrentGadgetNamed((Dialog *)gadget, "PREV")) {
        SetFrontendState(0xf, 0x395, "c:\\cavedog\\wargame\\multi.cpp");
        PlaySoundByName("Previous", 0);
        return;
    } else {
        ClearSelectedGadget(gadget);
        return;
    }
    a = FindGadgetChecked(entries, "SPEEDS")->u.list.selected;
    WriteGameRegistryValue((void*)"SERBAUD", &a, 4);
    a = FindGadgetChecked(entries, "PORTS")->u.list.selected;
    WriteGameRegistryValue((void*)"SERPORT", &a, 4);
}

// FUNCTION: 0x442560
void OpenSerialDialog()
{
    Layer* gadget = LoadGuiLayer(&g_game->gui, "SERIAL.GUI", 0x800);
    gadget->handler = HandleSerialDialogClick;
    gadget->data = g_game;
    gadget->cb1c = 0;
    LoadPictureCached(0, 0, 0, 0);
    host_net_init_lobbied_connection((host_net*)((char*)g_game + 0x14));
    ConfigureListBoxByName(&g_game->gui, "PORTS", "COM1\0COM2\0COM3\0COM4", 4, 0);
    ConfigureListBoxByName(&g_game->gui, "SPEEDS", "115200\0" "57600\0" "38400\0" "19200\0" "14400\0" "9600", 6, 0);

    int value;
    unsigned int size = 4;

    if (ReadGameRegistryValue("SERBAUD", &value, &size)) {
        SetListBoxScrollByName((SetListBoxScrollByNameDialog *)&g_game->gui, "SPEEDS", value);
    }
    if (ReadGameRegistryValue("SERPORT", &value, &size)) {
        SetListBoxScrollByName((SetListBoxScrollByNameDialog *)&g_game->gui, "PORTS", value);
    }
    Gadget* entry = FindGadgetChecked(gadget->entries, "PORTS");
    entry->u.list.callback = SetSerialPortFromGadget;
    SetSerialPortFromGadget(&g_game->gui, entry);
    Gadget* speeds = FindGadgetChecked(gadget->entries, "SPEEDS");
    speeds->u.list.callback = SetSerialBaudFromGadget;
    SetSerialBaudFromGadget(&g_game->gui, speeds);
    SetKeyboardInput(&g_game->gui, 1);
    RenderLayer(&g_game->gui, 0x40);
}

// FUNCTION: 0x4426e0
void FillAccountList(void)
{
    char* buffer = g_modemAccountNames;
    *buffer = 0;
    for (int i = 0; i < 20; i++) {
        strcpy(buffer, g_modemAccounts[i].name);
        buffer += strlen(g_modemAccounts[i].name) + 1;
    }
    Gadget* entry = FindGadgetChecked(g_game->gui.layer->entries, "ACCOUNTS");
    int player = entry->u.list.selected;
    ConfigureListBoxByName(&g_game->gui, "ACCOUNTS", g_modemAccountNames, 20, 0);
    SetListBoxScrollByName((SetListBoxScrollByNameDialog *)&g_game->gui, "ACCOUNTS", player);
}

// FUNCTION: 0x4427a0
void RefreshAccountList(void)
{
    Gadget* entry = FindGadgetChecked(g_game->gui.layer->entries, "ACCOUNTS");
    if (entry != 0 && g_modemAccounts != 0) {
        GetGadgetText(&g_game->gui, "NAME",
                     g_modemAccounts[entry->u.list.selected].name);
        GetGadgetText(&g_game->gui, "NUMBER",
                     g_modemAccounts[entry->u.list.selected].number);
        char* buffer = g_modemAccountNames;
        *buffer = 0;
        for (int i = 0; i < 20; i++) {
            strcpy(buffer, g_modemAccounts[i].name);
            buffer += strlen(g_modemAccounts[i].name) + 1;
        }
        int player = FindGadgetChecked(g_game->gui.layer->entries, "ACCOUNTS")->u.list.selected;
        ConfigureListBoxByName(&g_game->gui, "ACCOUNTS", g_modemAccountNames, 20, 0);
        SetListBoxScrollByName((SetListBoxScrollByNameDialog *)&g_game->gui, "ACCOUNTS", player);
    }
}

#pragma auto_inline(off)
// FUNCTION: 0x4428f0
void __stdcall ShowSelectedAccount(Gui* menu, Gadget* player)
{
    int entry = player->u.list.selected;
    if (entry >= 0) {
        SetTranslatedTextByName(menu, "NAME", g_modemAccounts[entry].name, 0);
        SetTranslatedTextByName(menu, "NUMBER", g_modemAccounts[entry].number, 0);
        int index = FindGadgetIndex(menu->layer->entries, "NAME", 3);
        SelectGadgetByIndex(menu, index);
        TrySetFocus(menu, index);
        MarkChanged((Dialog *)menu);
    }
}
#pragma auto_inline(on)

// FUNCTION: 0x442970
void SaveModemNumbers(void)
{
    if (g_modemAccounts != 0) {
        short count = FindGadgetChecked(g_game->gui.layer->entries, "ACCOUNTS")->u.list.selected;
        if (count > 0) {
            ModemAccount temp;
            memcpy(&temp, &g_modemAccounts[count], 0x102);
            for (int i = count; i > 0; i--)
                memcpy(&g_modemAccounts[i], &g_modemAccounts[i - 1], 0x102);
            memcpy(&g_modemAccounts[0], &temp, 0x102);
        }
        WriteGameRegistryValue((void*)"MODEMNUMBERS", g_modemAccounts, 0x1428);
    }
}

static inline int BuildAddressForModemDialog()
{
    int a = 0;
    int b = 0;
    int result = BuildCompoundAddress(&a, &b);
    if (result >= 0) {
        g_game->info.conn.data = (void*)a;
        g_game->info.conn.size = b;
    }
    return result;
}

// FUNCTION: 0x442a30
void __stdcall HandleModemDialogClick(Gui* gadget)
{
    Gadget* entries = gadget->layer->entries;
    if (gadget->hotGadgetIndex == -1) {
        if (g_modemInfo != 0) {
            GameFreeThunk(g_modemInfo);
            g_modemInfo = 0;
        }
        if (g_modemAccountNames != 0) {
            GameFreeThunk(g_modemAccountNames);
            g_modemAccountNames = 0;
        }
        if (g_modemAccounts != 0) {
            GameFreeThunk(g_modemAccounts);
            g_modemAccounts = 0;
        }
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gadget, "NAME")) {
        RefreshAccountList();
        TrySetFocus(gadget, FindGadgetIndex(entries, "NUMBER", 3));
        MarkChanged((Dialog *)gadget);
        ClearSelectedGadget(gadget);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gadget, "NUMBER")) {
        RefreshAccountList();
        SelectGadgetByIndex(gadget, FindGadgetIndex(entries, "JOIN", 1));
        TrySetFocus(gadget, FindGadgetIndex(entries, "JOIN", 1));
        strcpy((char*)entries + 0xcc, "JOIN");
        MarkChanged((Dialog *)gadget);
        ClearSelectedGadget(gadget);
        return;
    }
    if (FindGadgetIndex(entries, "HOST", 0xe) == gadget->hotGadgetIndex) {
        RefreshAccountList();
        SaveModemNumbers();
        g_game->bit0 = 1;
        g_game->bit1 = 1;
        BuildAddressForModemDialog();
        PlaySoundByName("SMLBUTTON", 0);
        BlankScreen();
        return;
    }
    if (FindGadgetIndex(entries, "JOIN", 0xe) != gadget->hotGadgetIndex) {
        if (IsCurrentGadgetNamed((Dialog *)gadget, "ACCOUNTS") == 0) {
            if (IsCurrentGadgetNamed((Dialog *)gadget, "PREV")) {
                SetFrontendState(0xf, 0x47f, "c:\\cavedog\\wargame\\multi.cpp");
                PlaySoundByName("Previous", 0);
                return;
            }
            ClearSelectedGadget(gadget);
            return;
        }
    }
    RefreshAccountList();
    SaveModemNumbers();
    g_game->bit1 = 1;
    g_game->bit0 = 0;
    BuildAddressForModemDialog();
    PlaySoundByName("SMLBUTTON", 0);
    OpenMessageBox(gadget, Translate("Connecting... press ESC to abort"),
                 0xfa, 1, 1);
}

static inline char* NameSlot(char* buffer)
{
    if (strlen(buffer) != 0)
        return buffer + strlen(buffer) + 1;
    return buffer;
}

// FUNCTION: 0x443070
BOOL __stdcall EnumModemAddressCallback(const GUID& guidDataType, DWORD dataSize, LPCVOID data, LPVOID context)
{
    char* name = (char*)data;
    if (memcmp(&guidDataType, &DPAID_Modem, sizeof(GUID)) == 0) {
        while (strlen(name) != 0) {
            strcpy(NameSlot(g_modemInfo), name);
            g_modemCount++;
            name += strlen(name) + 1;
        }
    }
    return TRUE;
}

#define DPERR_BUFFERTOOSMALL_00443100 0x8877001e

struct Len { unsigned int v; };

// FUNCTION: 0x443100
void __stdcall OpenModemDialog()
{
    char* addr = 0;
    unsigned long size = 0;
    Len len;
    int r;
    Layer* gadget;
    struct { void* dp; void* dp3; } net;
    GUID iid = g_dpspGuidModem;

    gadget = LoadGuiLayer(&g_game->gui, "MODEM.GUI", 0x800);
    gadget->handler = HandleModemDialogClick;
    gadget->data = g_game;
    LoadPictureCached(0, 0, 0, 0);
    host_net_init_lobbied_connection((host_net*)((char*)g_game + 0x14));
    r = host_net_create_play_interface((const host_guid*)&iid, (host_net_pair*)&net);
    if (r >= 0) {
        r = host_net_get_player_address((host_net_pair*)&net, 0, 0, &size);
        if (r == DPERR_BUFFERTOOSMALL_00443100) {
            addr = (char*)GameAllocIgnoreTag("MODEMADDR", size);
            if (addr != 0) {
                r = host_net_get_player_address((host_net_pair*)&net, 0, addr, &size);
                if (r >= 0) {
                    g_modemInfo = (char*)GameAllocIgnoreTag("MODEMINFO", 0xc8);
                    memset(g_modemInfo, 0, 0xc8);
                    g_modemCount = 0;
                    r = host_net_enum_address((host_net*)((char*)g_game + 0x14), (void*)EnumModemAddressCallback, addr, size, 0);
                    if (g_modemCount == 0) {
                        CloseTopScreen(&g_game->gui);
                        SetFrontendErrorText("Unable to find any modems");
                        SetFrontendState(0xf, 0x4e4, "c:\\cavedog\\wargame\\multi.cpp");
                        SetFrontendSubState(0, 0x4e5, "c:\\cavedog\\wargame\\multi.cpp");
                        GameFreeThunk(g_modemInfo);
                        GameFreeThunk(addr);
                        host_net_release_play_interface((host_net_pair*)&net);
                        return;
                    }
                    if (r >= 0) {
                        int i;
                        ConfigureListBoxByName(&g_game->gui, "MODEMS", g_modemInfo, g_modemCount, 0);
                        g_modemAccounts = (ModemAccount*)GameAllocIgnoreTag("MODEMACCOUNTS", 0x1428);
                        len.v = 0x1428;
                        r = ReadGameRegistryValue("MODEMNUMBERS", g_modemAccounts, &len.v);
                        if (r == 0) {
                            for (i = 0; i < 20; i++) {
                                strcpy(g_modemAccounts[i].name, "UNUSED");
                                g_modemAccounts[i].number[0] = 0;
                            }
                        }
                        char* buffer = (char*)GameAllocIgnoreTag("ACCOUNTNAMES", 0xa00);
                        g_modemAccountNames = buffer;
                        *buffer = 0;
                        char* p = buffer;
                        for (i = 0; i < 20; i++) {
                            strcpy(p, g_modemAccounts[i].name);
                            p += strlen(g_modemAccounts[i].name) + 1;
                        }
                        Gadget* entry = FindGadgetChecked(g_game->gui.layer->entries, "ACCOUNTS");
                        int player = entry->u.list.selected;
                        ConfigureListBoxByName(&g_game->gui, "ACCOUNTS", g_modemAccountNames, 20, 0);
                        SetListBoxScrollByName((SetListBoxScrollByNameDialog *)&g_game->gui, "ACCOUNTS", player);
                        entry = FindGadgetChecked(gadget->entries, "ACCOUNTS");
                        entry->u.list.callback = ShowSelectedAccount;
                        ShowSelectedAccount(&g_game->gui, entry);
                    }
                }
            }
        }
    }
    SetKeyboardInput(&g_game->gui, 1);
    RenderLayer(&g_game->gui, 0x40);
    host_net_release_play_interface((host_net_pair*)&net);
    GameFreeThunk(addr);
}

// FUNCTION: 0x443480
void __stdcall HandleReportClick(Gui* obj)
{
    int i;
    Gadget* entries;
    int value;
    char buf[16];
    int handle;
    int acc;

    entries = obj->layer->entries;
    if (obj->hotGadgetIndex == -1)
        return;
    if (IsCurrentGadgetNamed((Dialog *)obj, "OK")) {
        acc = 0;
        for (i = 0; i < 16u; i++) {
            sprintf(buf, "CHK%d", i);
            handle = FindGadgetIndex(entries, buf, 1);
            if (!GetGadgetActiveByName(obj, buf))
                break;
            value = GetGadgetStatus(obj, handle);
            acc = (int)(pow(2.0, i) * value + acc);
        }

        SetCursorMode(0x14);
        EnableReporter(acc);
        if ((g_game->lobbyUiDirtyFlags & 0x10) || g_game->frontendSubState == 0x14) {
            g_game->frontendNextSubState = 0x15;
        } else {
            g_game->frontendNextSubState = 0x11;
        }
    } else {
        ClearSelectedGadget(obj);
    }
}

// FUNCTION: 0x443590
void ReportDialogFrame(void)
{
    SendNetHeartbeat();
}

// FUNCTION: 0x4435a0
void __stdcall OpenReportDialog(unsigned int* count, char** names)
{
    char name[16];
    Layer* gadget = LoadGuiLayer(&g_game->gui, "REPORT.GUI", 0x800);
    gadget->handler = HandleReportClick;
    gadget->data = g_game;
    gadget->cb1c = ReportDialogFrame;
    LoadPictureCached("scorebg", 0, 1, 0);
    for (unsigned int i = 0; i < *count; i++) {
        sprintf(name, "CHK%d", i);
        SetGadgetActiveByName(&g_game->gui, name, 1);
        sprintf(name, "SERVICE%d", i);
        SetGadgetActiveByName(&g_game->gui, name, 1);
        SetTranslatedTextByName(&g_game->gui, name, names[i], 0x80);
    }
    SetKeyboardInput(&g_game->gui, 1);
    RenderLayer(&g_game->gui, 0x141);
    SetCursorMode(0x13);
    MarkChanged((Dialog *)&g_game->gui);
    MarkLayerChanged((Dialog *)&g_game->gui);
}

// FUNCTION: 0x4436e0
int InitScoreReporting(void)
{
    if (g_game->net->GetGameType() != 3)
        return 0;
    int saved = (signed char)g_game->cursorMode;
    SetCursorMode(0x14);
    int r = LoadReporterDll(&g_reporterCount, g_reporterNames);
    if (r == 0) {
        if (g_reporterCount > 0) {
            SetCursorMode(0x13);
            OpenReportDialog(&g_reporterCount, (char**)g_reporterNames);
            SetCursorMode(saved);
            return 1;
        }
    } else if (r != 4) {
        SetOffscreenSurface((int)g_game->screen);
        OpenMessageBox(&g_game->gui, Translate("Unable to initialize scores reporting."), 0x190, 1, 0);
        LoadPictureCached("ReportError", 0, 1, 0);
        RunWhileScreenNamed((Dialog *)&g_game->gui, "MSGBOX.GUI");
    }
    SetCursorMode(saved);
    return 0;
}

// FUNCTION: 0x4437c0
void __stdcall HandleSelectGameClick(Gui* param_1)
{
    Gadget* entries = param_1->layer->entries;
    int i;
    int id;
    int cur;

    if (g_cmdlineTcpJoinAddress[0] != 0) {
        g_cmdlineTcpJoinAddress[0] = 0;
        if (g_cmdlineHostMode != 0)
            goto startnew;
    }

    if (param_1->hotGadgetIndex == -1) {
        for (i = 0; i < 0xf; i++) {
            GameFreeThunk(g_game->data[i]);
            g_game->data[i] = 0;
        }
        GameFreeThunk(g_game->shared);
        GameFreeThunk(g_game->desc);
        g_game->shared = 0;
        g_game->desc = 0;
        return;
    }

    if (FindGadgetIndex(entries, "UPDATE", 0xe) == param_1->hotGadgetIndex) {
        char* pass = (char*)FindGadgetChecked_B(entries, "PASSWORD");
        if (pass != 0) {
            cur = g_game->localPlayer;
            strcpy(g_game->players[cur].info->password, pass + 0xb6);
        }
        PlaySoundByName("Multi", 0);
        ConnectToGame(param_1->layer);
        MarkChanged((Dialog *)param_1);
        ClearSelectedGadget(param_1);
        return;
    }

    if (FindGadgetIndex(entries, "PREVMENU", 0xe) == param_1->hotGadgetIndex) {
        g_game->frontendNextSubState = 3;
        PlaySoundByName("Previous", 0);
        return;
    }

    if (FindGadgetIndex(entries, "WATCH", 0xe) == param_1->hotGadgetIndex ||
        FindGadgetIndex(entries, "JOINGAME", 0xe) == param_1->hotGadgetIndex ||
        entries[param_1->hotGadgetIndex].type == 2) {
        Gadget* e = FindGadgetChecked(entries, "GAMENAME");
        SelectGameMsg msg;
        unsigned int flags;
        int ver;
        char* pass;
        unsigned int b;

        *(GameDescEntry*)g_game->game_info = g_game->desc[e->u.list.selected];
        msg.group = *(GameAdvertGroup*)((char*)g_game + 0x2b6e);
        flags = *(unsigned int*)((char*)&msg.group + 2);
        ver = *(unsigned int*)((char*)&msg.group + 0xe) & 0xff;

        if (ver <= g_game->version && ver >= g_game->version) {
            if ((flags & 0x8000) != 0 || (flags & 0x10) != 0) {
                PlaySoundByName("Previous", 0);
                ClearSelectedGadget(param_1);
                return;
            }
            GetGadgetText(param_1, "NICKNAME", g_game->nickname);
            if (strlen(g_game->nickname) == 0) {
                BeginTextEdit(param_1, FindGadgetIndex(entries, "NICKNAME", 3));
                ClearSelectedGadget(param_1);
                OpenMessageBox(param_1, Translate("You must enter your name"), 0xc8, 1, 1);
                return;
            }
            pass = (char*)FindGadgetChecked_B(entries, "PASSWORD");
            g_game->password[0] = 0;
            strncat(g_game->password, pass + 0xb6, 0xa);
            cur = g_game->localPlayer;
            g_game->players[cur].info->password[0] = 0;
            strncat(g_game->players[cur].info->password, pass + 0xb6, 0xa);
            b = FindHostSlot();
            if (b != 0xa &&
                (g_game->players[b].info->flags & 0x10) == 0x10)
                g_game->netGameFlags |= 4;
            if (IsCurrentGadgetNamed((Dialog *)param_1, "WATCH")) {
                cur = g_game->localPlayer;
                g_game->players[cur].info->flags |= 0x40;
                PlaySoundByName("Multi", 0);
                g_game->frontendNextSubState = 0x13;
                return;
            }
            cur = g_game->localPlayer;
            g_game->players[cur].info->flags &= 0xffbf;
            PlaySoundByName("BigButton", 0);
            g_game->frontendNextSubState = 0x12;
            return;
        }
        SetFrontendErrorText("You do not have a compatible version for this game.");
    } else if (FindGadgetIndex(entries, "STARTNEW", 0xe) == param_1->hotGadgetIndex) {
startnew:
        cur = g_game->localPlayer;
        g_game->players[cur].info->flags &= 0xffbf;
        PlaySoundByName("BigButton", 0);
        GetGadgetText(param_1, "NICKNAME", g_game->nickname);
        BlankScreen();
        CloseTopScreen(param_1);
        OpenNewMultiDialog();
        ClearSelectedGadget(param_1);
        return;
    }
    ClearSelectedGadget(param_1);
}

// FUNCTION: 0x443cb0
void OpenSelectGameDialog()
{
    char name[0x14];
    int i;
    int j;

    BlankScreen();
    Layer* gadget = LoadGuiLayer(&g_game->gui, "SELGAME.GUI", 0x80);
    gadget->handler = HandleSelectGameClick;
    gadget->data = g_game;
    LoadPictureCached("selectgame2x", 0, 0, 0);
    g_game->desc = (GameDescEntry*)GameAllocIgnoreTag("GAME DESCRIPTIONS", 0x690);
    g_game->shared = (char*)GameAllocIgnoreTag("PLAYER SHARED", 0xe74);
    for (i = 0; i < 0xf; i++) {
        sprintf(name, "DATA%d", i);
        g_game->data[i] = GameAllocIgnoreTag(name, 0xa00);
    }
    for (j = 0; j < 20; j++) {
        g_game->desc[j].size = 0xb9;
        g_game->desc[j].offset = (int)(g_game->shared + j * 0xb9);
    }
    SetTranslatedTextByName(&g_game->gui, "PASSWORD", g_game->password, 10);
    SetTranslatedTextByName(&g_game->gui, "NICKNAME", g_game->nickname, 10);
    SetGrayedOutByName(&g_game->gui, "JOIN", 1);
    SetGrayedOutByName(&g_game->gui, "WATCH", 1);
    for (i = 1; i < gadget->entries->u.count; i++) {
        if (gadget->entries[i].type == 2) {
            Gadget& e = gadget->entries[i];
            e.u.list.callback = UpdateGameSelection;
            e.u.list.records = g_game->desc;
        }
    }
    RenderLayer(&g_game->gui, 0x40);
    if (!ConnectToGame(gadget)) {
        CloseTopScreen(&g_game->gui);
        OpenMessageBox(&g_game->gui, Translate("Invalid TCP/IP Address"), 0xc8, 1, 1);
        g_game->frontendNextSubState = 3;
        return;
    }
    SelectGadgetByIndex(&g_game->gui, FindGadgetIndex(gadget->entries, "GAMENAME", 2));
    OrLabelAttribs();
    SetKeyboardInput(&g_game->gui, 1);
    RenderLayer(&g_game->gui, 0x40);
    Player* conn = &g_game->players[g_game->localPlayer];
    if (conn->rejectReason != 0 && conn->rejectReason != 2) {
        RenderLayer(&g_game->gui, 0x40);
        char* msg = GetRejectReasonText(conn->rejectReason);
        OpenMessageBox(&g_game->gui, Translate(msg), 0x140, 1, 1);
        MarkChanged((Dialog *)&g_game->gui);
        MarkLayerChanged((Dialog *)&g_game->gui);
        conn->rejectReason = 0;
    }
    if (g_cmdlineTcpJoinAddress[0] != 0 && g_cmdlineHostMode != 0) {
        if (strlen(g_game->nickname) != 0)
            HandleSelectGameClick(&g_game->gui);
    }
}

// FUNCTION: 0x4444d0
int __stdcall CloneServiceSlot(Gadget* entries, int param_2, short param_3, int param_4, char* param_5)
{
    int index = ++entries[0].u.count;
    Gadget* d = &entries[index];
    Gadget* s = &entries[param_2];
    *d = *s;
    SetTranslatedText(&g_game->gui, index, param_4, 0);
    strcpy(d->name, param_5);
    d->y = param_3;
    d->active = 1;
    d->colours = 0;
    return index;
}

// FUNCTION: 0x444910
void __stdcall CacheLogosGadgetIndex(Gui* param1, Gadget* param2)
{
    param1->hotGadgetIndex = FindGadgetIndex(param1->layer->entries, "LOGOS", 2);
}

// FUNCTION: 0x444930
void __stdcall HandleLogoSelectClick(Gui* param_1)
{
    Gadget* entries = param_1->layer->entries;
    LogoSelectLayout* layout = (LogoSelectLayout*)param_1->layer->data;

    if (param_1->hotGadgetIndex == -1) {
        GameFreeThunk(layout->seqs);
        GameFreeThunk(layout->ptrList);
        GameFreeThunk(layout);
        PlaySoundByName("Multi", 0);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)param_1, "LOGOS") || IsCurrentGadgetNamed((Dialog *)param_1, "SELECT")) {
        Player* player = &g_game->players[g_game->localPlayer];
        Gadget* entry = FindGadgetChecked(entries, "LOGOS");
        player->info->color = layout->selected[(unsigned char)entry->u.list.selected];
        g_game->bits_2bee_1.dirty = 1;
        RequestPlayerColor(player->info->color);
        return;
    }
    if (!IsCurrentGadgetNamed((Dialog *)param_1, "Cancel"))
        ClearSelectedGadget(param_1);
}

// FUNCTION: 0x444a20
void ShowSelectedMapInfo()
{
    int outX;
    int outY;
    char buffer[100];

    if (FindGadgetIndex(g_game->gui.layer->entries, "MAPNAME", 5) != -1) {
        SetTranslatedTextByName(&g_game->gui, "MAPNAME",
                     (char*)g_game->net->GetTranslatedName(), 0);
    }

    sprintf(buffer, "%s  %s: %s",
            (char*)g_game->net.get() + 0xdc4,
            Translate("Players"),
            (char*)g_game->net.get() + 0xe44);
    SetTranslatedTextByName(&g_game->gui, "SIZE", (char*)buffer, 0);

    Gadget* entry = FindGadgetChecked_E(g_game->gui.layer->entries, "MAPPIC");
    if (entry->u.hotspot.frame != 0) {
        GameFreeThunk(entry->u.hotspot.frame);
        entry->u.hotspot.frame = 0;
    }
    void* bmp = LoadRadarPic(
        (char*)g_game->net->GetNameSlot(1), &outX, &outY);
    entry->u.hotspot.frame = (GafFrame*)bmp;
    if (bmp != 0) {
        ResizeRadarPicture((GafFrame *)bmp, entry->width, entry->height, outX << 4, outY << 4);
    }

    SetTranslatedTextByName(&g_game->gui, "DESCRIPTION",
                 (char*)g_game->net->GetDescription(), 0);
    MarkChanged((Dialog *)&g_game->gui);
}

// FUNCTION: 0x444ba0
void __stdcall HandleViewMapClick(Gui* param_1)
{
    if (param_1->hotGadgetIndex != -1) {
        if (IsCurrentGadgetNamed((Dialog *)param_1, g_okGadgetName)) {
            PlaySoundByName(g_multiSoundName, 0);
        } else {
            ClearSelectedGadget(param_1);
        }
    }
}

// FUNCTION: 0x444be0
void OpenViewMapDialog()
{
    LoadGuiLayer(&g_game->gui, "VIEWMAP.GUI", 0x900)->handler = HandleViewMapClick;
    LoadPictureCached("DVIEWMAP", 0, 0, 0);
    ShowSelectedMapInfo();
    SetKeyboardInput(&g_game->gui, 1);
    RenderLayer(&g_game->gui, 0x40);
}

// FUNCTION: 0x444c40
void __stdcall UpdateMapSelection(Gui* menu, Gadget* unused)
{
    Gadget* g = FindGadgetChecked(menu->layer->entries, "MAPNAMES");
    if (g_game->net->LoadMissionByName(SkipTextLines(g->u.list.lines, g->u.list.selected)) == 0) {
        SetGadgetActiveByName(menu, "MAPPIC", 0);
    } else {
        SetGadgetActiveByName(menu, "MAPPIC", 1);
        ShowSelectedMapInfo();
    }
}

// FUNCTION: 0x444cb0
void __stdcall HandleMapSelectClick(Gui* param_1)
{
    void* entries = param_1->layer->entries;
    MapSelectClickLayout* layout = (MapSelectClickLayout*)param_1->layer->data;

    if (param_1->hotGadgetIndex == -1) {
        Gadget* entry = FindGadgetChecked_E(g_game->gui.layer->entries, "MAPPIC");
        if (entry->u.list.lines != 0) {
            GameFreeThunk(entry->u.list.lines);
            entry->u.list.lines = 0;
        }
        GameFreeThunk(layout->items);
        GameFreeThunk(layout);
        GameFreeThunk(g_oldMapName);
        g_oldMapName = 0;
        return;
    }

    if (IsCurrentGadgetNamed((Dialog *)param_1, "MAPNAMES") || IsCurrentGadgetNamed((Dialog *)param_1, "LOAD")) {
        PlaySoundByName("Multi", 0);
        Gadget* g = FindGadgetChecked(entries, "MAPNAMES");
        g_game->net->LoadMissionByName(
            SkipTextLines(g->u.list.lines, g->u.list.selected));

        Player* player = &g_game->players[g_game->localPlayer];
        strcpy(player->info->map,
               g_game->net->GetMissionName());
        player->info->mapCrc =
            g_game->net->ComputeMapChecksum();

        BroadcastPlayerInfo();
        ReportGameEvent(5);
        UpdateNetGameInfo();

        for (int i = 0; i < 10; i++) {
            if (g_game->players[i].active == 0 ||
                (g_game->players[i].type != 1 && g_game->players[i].type != 2)) {
                g_game->players[i].info->flags &= 0xffdf;
            }
        }
        return;
    }

    if (IsCurrentGadgetNamed((Dialog *)param_1, "PREVMENU")) {
        PlaySoundByName("Previous", 0);
        g_game->net->LoadMissionByName(g_oldMapName);
        BroadcastPlayerInfo();
        return;
    }

    ClearSelectedGadget(param_1);
}

// FUNCTION: 0x444ea0
void OpenMultiMapSelector()
{
    g_oldMapName = (char*)GameAllocIgnoreTag("OLDMAPNAME", 0xc8);

    if (!g_game->net->HasMissionName()) {
        OpenMessageBox(&g_game->gui,
                     Translate("There are no multiplayer maps to choose from"),
                     0x140, 1, 1);
        return;
    }

    strcpy(g_oldMapName,
           g_game->net->GetMissionName());
    g_game->net->RefreshMapList(0);

    int n = LoadMapList(0, 0, 0);
    if (n == 0) {
        OpenMessageBox(&g_game->gui,
                     Translate("There are no multiplayer maps to choose from"),
                     0x140, 1, 1);
        return;
    }

    SelectDialogLayer* layer = LoadGuiLayer(&g_game->gui, "SELMAP.GUI", 0x980);
    layer->handler = HandleMapSelectClick;
    MapSelectData* data = (MapSelectData*)GameAllocIgnoreTag("SELECT MAP DATA", 0x20);
    layer->data = data;
    LoadPictureCached("DSELECTMAP2", 0, 0, 0);
    LoadMapList((void **)&data->items, 0, 0);
    SortFileList(data->items, 0, 0, n);
    ConfigureListBoxByName(&g_game->gui, "MAPNAMES", data->items, n, 0);
    FindGadgetChecked(layer->entries, "MAPNAMES")->u.list.callback = UpdateMapSelection;

    for (int i = 0; i < n; i++) {
        if (strcmp(g_oldMapName, SkipTextLines(data->items, i)) == 0) {
            SetListBoxScrollByName((SetListBoxScrollByNameDialog *)&g_game->gui, "MAPNAMES", i);
            break;
        }
    }

    Gui* menu = &g_game->gui;
    Gadget* g = FindGadgetChecked(menu->layer->entries, "MAPNAMES");
    if (g_game->net->LoadMissionByName(
            SkipTextLines(g->u.list.lines, g->u.list.selected)) == 0) {
        SetGadgetActiveByName(menu, "MAPPIC", 0);
    } else {
        SetGadgetActiveByName(menu, "MAPPIC", 1);
        ShowSelectedMapInfo();
    }
    SetKeyboardInput(&g_game->gui, 1);
    RenderLayer(&g_game->gui, 0x40);
}

// FUNCTION: 0x445110
void OpenLogoSelectDialog()
{
    SelectDialogLayer* gui = LoadGuiLayer(&g_game->gui, "LOGOSEL.GUI", 0x800);
    gui->handler = HandleLogoSelectClick;
    LogoSelectLayout* layout = (LogoSelectLayout*)GameAllocIgnoreTag("SELECT TEAM LOGO", 0x20);
    gui->data = layout;
    int count = ((LogoTable *)g_game->logos32)->count;
    layout->ptrList = (void**)GameAllocIgnoreTag("ANIMSEQ PTR LIST", count * 4);
    layout->seqs = (LogoAnimSeq*)GameAllocIgnoreTag("ACTUAL ANIMSEQS", count * 0x30);
    void** cursor = layout->ptrList;
    int n = 0;
    for (int j = 0; j < count; j++) {
        int k;
        for (k = 0; k < 10; k++) {
            if (g_game->players[k].type != 0 && g_game->players[k].type != 4
                && g_game->players[k].info->color >= j)
                break;
        }
        layout->seqs[j] = *(LogoAnimSeq*)(LogoTable *)g_game->logos32;
        layout->seqs[j].frame = ((LogoTable *)g_game->logos32)->entries[j].ptr;
        if (k == 10 && (!g_quirks.boundLogoSelect || n < (int)sizeof(layout->selected))) {
            *cursor = &layout->seqs[j];
            layout->selected[n] = (char)j;
            n++;
            cursor++;
        }
    }
    Gadget* logo = FindGadgetChecked(gui->entries, "LOGOS");
    if (logo != 0) {
        logo->u.list.callback = CacheLogosGadgetIndex;
    }
    int index = FindGadgetIndex(gui->entries, "LOGOS", 2);
    if (index != -1) {
        ((Gadget*)((char*)gui->entries + index * 0x15b))->attribs |= 0x40;
    }
    SetGadgetItems(gui, "LOGOS", layout->ptrList, n);
    SetKeyboardInput(&g_game->gui, 1);
    RenderLayer(&g_game->gui, 0x40);
}

// FUNCTION: 0x445300
void __stdcall ExpandGadgetTextToType5(Gadget* param_1)
{
    if (param_1->type == 1) {
        PlayerSlotGadgetCopy tmp = *(PlayerSlotGadgetCopy*)param_1;
        int index = FindGadgetIndex(g_game->gui.layer->entries, param_1->name, 0xe);
        TruncateGadgetText(&g_game->gui, index);
        param_1->y += 2;
        param_1->type = 5;
        strcpy(param_1->u.text, tmp.text);
        param_1->attribs |= 0x10;
    }
}

// FUNCTION: 0x4453a0
void __stdcall SwapPlayerSlots(Player* param_1, Player* param_2)
{
    Player tmp = *param_2;
    *param_2 = *param_1;
    *param_1 = tmp;
    param_1->SetType(0);
    param_1->active = 0;
    for (int i = 0; i <= 10; i++) {
        Player* p = &g_game->players[i];
        if (p->active != 0
            && (p->type == 1 || p->type == 2 || p->type == 3)
            && p->index != 10) {
            p->index = i;
        } else {
            g_game->players[i].index = 10;
        }
    }
}

// FUNCTION: 0x445450
void CompactActivePlayerSlots()
{
    Player* p = g_game->players;
    Player* q = g_game->players + 1;
    Player* end = g_game->players + 10;
    while (1) {
        if (q >= end && p >= end)
            break;
        while ((p->active != 0
                    && (p->type == 1 || p->type == 2 || p->type == 3)
                    && p->index != 10)
               || p->type == 4) {
            if (p >= end)
                break;
            p++;
        }
        q = p + 1;
        for (; q->active == 0
               || (q->type != 1 && q->type != 2 && q->type != 3)
               || q->index == 10;
             q++) {
            if (q >= end)
                break;
        }
        if (q >= end)
            break;
        if (p >= end)
            break;
        Player tmp = *p;
        *p = *q;
        *q = tmp;
        q->SetType(0);
        q->active = 0;
        for (int i = 0; i <= 10; i++) {
            Game* g = g_game;
            unsigned char* f = &g->players[i].index;
            if (g->players[i].active != 0
                && (g->players[i].type == 1 || g->players[i].type == 2
                    || g->players[i].type == 3) && *f != 10) {
                *f = (unsigned char)i;
            } else {
                *f = 10;
            }
        }
    }
}

static void RestoreSlotGadgetText(Gadget* rec)
{
    PlayerSlotGadgetCopy tmp = *(PlayerSlotGadgetCopy*)rec;
    int index = FindGadgetIndex(g_game->gui.layer->entries, rec->name, 0xe);
    TruncateGadgetText(&g_game->gui, index);
    rec->y += 2;
    rec->type = 5;
    strcpy(rec->u.text, tmp.text);
    rec->attribs |= 0x10;
}

// FUNCTION: 0x4455b0
void __cdecl BuildPlayerSlotGadgets(void)
{
    char* base = (char*)g_game->gui.layer->entries;
    int p = 0;
    int t;
    char** slot;

    *(short*)(base + 0xb6) = g_battleRoomBaseGadgetCount;
    do {
        for (t = 0, slot = g_battleRoomGadgetNames; *slot != 0; slot++, t++) {
            int index = FindGadgetIndex(base, *slot, 0xe);
            Gadget* rec = (Gadget*)(base + 0x15b * index);
            Gadget* dst;
            short count;

            DAT_00512760 = rec->y;
            if (t == 0)
                DAT_0051276c = rec->height;
            count = ++*(short*)(base + 0xb6);
            dst = (Gadget*)(base + 0x15b * count);
            *dst = *rec;
            dst->name[strlen(dst->name) - 1] = (char)('0' + p);
            dst->y += p * 20;
            dst->team = 0;
            dst->active = 1;
            if (dst->type != 5) {
                switch (t) {
                case 0:
                    if (p == g_game->localPlayer) {
                        if (dst->type == 1)
                            RestoreSlotGadgetText(dst);
                        dst->attribs = 1;
                    } else {
                        dst->attribs |= 0x8000;
                    }
                    break;
                case 1:
                    if (p != g_game->localPlayer) {
                        dst->flag_bit = 1;
                        dst->status = 0;
                    }
                    break;
                case 2:
                    {
                        Player* pl = &g_game->players[p];
                        if (pl->active == 0 || (pl->type != 1 && pl->type != 2))
                            dst->active = 0;
                    }
                    break;
                case 3:
                    {
                        Player* pl = &g_game->players[p];
                        int ok = pl->active != 0 && (pl->type == 1 || pl->type == 2);
                        SetGadgetGrayedOutByName(&g_game->gui, dst->name, !ok);
                    }
                    dst->active = 0;
                    break;
                case 6:
                    if (p != g_game->localPlayer && dst->type == 1)
                        RestoreSlotGadgetText(dst);
                    if (*(int*)&g_game->players[p] != 0 &&
                        (&g_game->players[p])->type == 2)
                        dst->active = 0;
                    break;
                case 7:
                    if (p == g_game->localPlayer)
                        dst->active = 0;
                    break;
                case 9:
                    dst->active = 0;
                    SetButtonStageByName((Gui*)&g_game->gui, dst->name, 10);
                    break;
                default:
                    if (dst->type == 1)
                        RestoreSlotGadgetText(dst);
                    break;
                }
            }
        }
        p++;
    } while (p < 10);

    {
        char name[52];
        int index;
        sprintf(name, "PLAYER%d", g_game->localPlayer);
        index = FindGadgetIndex(base, name, 0xe);
        if (index != -1) {
            Gadget* rec = (Gadget*)(base + 0x15b * index);
            if (rec->type == 1)
                RestoreSlotGadgetText(rec);
        }
        sprintf(name, "READY%d", g_game->localPlayer);
        index = FindGadgetIndex(base, name, 1);
        if (index != -1) {
            Gadget* rec = (Gadget*)(base + 0x15b * index);
            rec->quickKey = (unsigned char)tolower(name[0]);
            strcpy(base + 0xcc, name);
        }
    }
    g_battleRoomSlotsBuilt = 1;
}

// FUNCTION: 0x445b70
void __stdcall UpdateMaxUnitsText(Gui* gui, int index)
{
    char text[0x14];
    int count;
    Gadget* maxunits = (Gadget*)FindGadgetChecked_D(gui->layer->entries, "MAXUNITS");
    if (maxunits != 0) {
        int player = FindHostSlot();
        if (player == g_game->localPlayer || player == 10) {
            count = ReadSliderValue(maxunits) + 0x14;
        } else {
            count = g_game->players[player].info->maxUnits;
        }
        _itoa(count, text, 10);
        SetTranslatedTextByName(gui, "MAXUNITSTEXT", text, 0);
        g_game->players[g_game->localPlayer].info->maxUnits = count;
        PlayerInfo* data = g_game->players[g_game->localPlayer].info;
        unsigned char f = data->flags_97;
        data->maxUnits = count;
        if (f & 1) {
            BroadcastPlayerInfo();
        }
    }
}

// FUNCTION: 0x445c70
void __stdcall UpdateMetalText(Gui* sub, int unused)
{
    char text[20];
    void* value = FindGadgetChecked_D(sub->layer->entries, "METAL");

    if (value != 0) {
        int shown = ReadSliderValue((Gadget *)value) / 100 * 100;
        int hundreds;
        PlayerInfo* unit;

        _itoa(shown, text, 10);
        SetTranslatedTextByName(sub, "METALTEXT", text, 0);
        hundreds = shown / 100;
        g_game->players[g_game->localPlayer].info->metal = (unsigned short)hundreds;
        unit = g_game->players[g_game->localPlayer].info;
        unit->metal = (unsigned short)hundreds;
        if (unit->flags_97 & 1) {
            BroadcastPlayerInfo();
            UpdateNetGameInfo();
        }
    }
}

// FUNCTION: 0x445d60
void __stdcall UpdateEnergyText(Gui* sub, int unused)
{
    char text[20];
    void* value = FindGadgetChecked_D(sub->layer->entries, "ENERGY");

    if (value != 0) {
        int shown = ReadSliderValue((Gadget *)value) / 100 * 100;
        PlayerInfo* unit;

        _itoa(shown, text, 10);
        SetTranslatedTextByName(sub, "ENERGYTEXT", text, 0);
        unit = g_game->players[g_game->localPlayer].info;
        unit->energy = (unsigned short)(shown / 100);
        if (unit->flags_97 & 1) {
            BroadcastPlayerInfo();
            UpdateNetGameInfo();
        }
    }
}

// FUNCTION: 0x445e20
void __stdcall SetNamedSliderValue(Gui* menu, char* name, int value)
{
    Gadget* gadget = FindGadgetChecked_D(menu->layer->entries, name);
    SetSliderFromValue(gadget, value);
}

typedef void (__stdcall* SliderCallback)(Gui* menu, int index);

// FUNCTION: 0x445e50
void __stdcall BindNamedSliderWithCallback(char* name, int param_2, int param_3, SliderCallback callback)
{
    Gui* menu = &g_game->gui;
    void* gadgets = menu->layer->entries;
    int index = FindGadgetIndex(gadgets, name, 0xe);
    if (index != -1) {
        Gadget* gadget = FindGadgetChecked_D(gadgets, name);
        gadget->max = param_2;
        gadget->sliderCallback = callback;
        gadget->knobPos = param_3;
        SetSliderFromValue(gadget, gadget->knobPos);
        gadget->sliderUser = (int)g_game;
    }
    callback(menu, index);
    MarkChanged((Dialog *)menu);
}

// FUNCTION: 0x445ed0
void UpdateBattleRoomFlags()
{
    int i = FindHostSlot();
    if (i == 10) {
        i = g_game->localPlayer;
    }
    PlayerInfo* info = g_game->players_1b63[i].info;

    SetButtonStageByName((Gui*)&g_game->gui, "COMMANDER", info->commander);
    SetButtonStageByName((Gui*)&g_game->gui, "MAPPING", !info->mapping);
    int los;
    if (!info->los) {
        los = 2;
    } else {
        los = !info->losType;
    }
    SetButtonStageByName((Gui*)&g_game->gui, "LOSTYPE", los);
    SetButtonStageByName((Gui*)&g_game->gui, "WATCHING", info->watching);
    SetButtonStageByName((Gui*)&g_game->gui, "CHEATING", info->cheating);
    SetButtonStageByName((Gui*)&g_game->gui, "FIXEDLOC", info->fixedloc);
    SetButtonStageByName((Gui*)&g_game->gui, "GAMEOPEN", !info->closed);
}

// FUNCTION: 0x446020
void __stdcall HandleRejectChoice(Gui* gadget)
{
    int owner = (int)gadget->layer->entries;
    if (gadget->hotGadgetIndex == -1)
        return;
    if (IsGadgetNamed(owner, gadget->hotGadgetIndex, "CHOICE1")) {
        RejectPlayer(GetSlotDpid((unsigned char)g_rejectPlayer), 1);
    } else if (!IsGadgetNamed(owner, gadget->hotGadgetIndex, "CHOICE2")) {
        ClearSelectedGadget(gadget);
    }
}

// FUNCTION: 0x446080
void __stdcall OpenRejectDialog(int player)
{
    char buf[100];
    g_rejectPlayer = player;
    SelectDialogLayer* gadget = LoadGuiLayer(&g_game->gui, "YESORNO.GUI", 0x100);
    if (gadget != 0) {
        SetKeyboardInput(&g_game->gui, 1);
        void* entries = gadget->entries;
        FindGadgetIndex(entries, "CHOICE1", 1);
        FindGadgetIndex(entries, "CHOICE2", 1);
        FindGadgetIndex(entries, "TITLE", 5);
        SetTranslatedTextByName(&g_game->gui, "CHOICE1", "Yes", 0);
        SetTranslatedTextByName(&g_game->gui, "CHOICE2", "No", 0);
        sprintf(buf, "%s %s?", Translate("Reject"),
                g_game->players[g_rejectPlayer].name);
        SetTranslatedTextByName(&g_game->gui, "TITLE", buf, 0);
        gadget->handler = HandleRejectChoice;
        gadget->data = g_game;
        SetKeyboardInput(&g_game->gui, 1);
        RenderLayer(&g_game->gui, 0x40);
    }
}

// FUNCTION: 0x4461d0
void __stdcall HandleDisplayModesClick(Gui* gui)
{
    SelectDialogLayer* holder = gui->layer;
    Gadget* gadgets = holder->entries;
    ModeList* obj = (ModeList*)holder->data;
    if (gui->hotGadgetIndex == -1) {
        GameFreeThunk(obj->available);
        GameFreeThunk(obj->modes);
        GameFreeThunk(obj);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gui, "MODES") || IsCurrentGadgetNamed((Dialog *)gui, "SELECT")) {
        Player* player = &g_game->players[g_game->localPlayer];
        Gadget* entry = FindGadgetChecked(gadgets, "MODES");
        DisplayMode* mode = &obj->modes[entry->u.list.selected];
        g_game->width_37f1b = mode->width;
        g_game->height_37f1f = mode->height;
        player->info->width = (unsigned short)mode->width;
        player->info->height = (unsigned short)mode->height;
        BroadcastPlayerInfo();
        SaveSettings();
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gui, "CANCEL")) {
        PlaySoundByName("Exit", 0);
        SaveSettings();
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gui, "OK")) {
        PlaySoundByName("SMLBUTTON", 0);
        SaveSettings();
        return;
    }
    ClearSelectedGadget(gui);
}

// FUNCTION: 0x446310
void CyclePlayerDisplayMode(void)
{
    ModeList* obj = (ModeList*)GameAllocIgnoreTag("SELECT VIDEO MODE", 0x20);
    obj->available = 0;
    obj->modes = (DisplayMode*)GameAllocIgnoreTag("DISPLAY MODES", 0x4b0);

    if (GetDisplayModes(obj) != 0) {
        SortDisplayModes(obj);
        obj->available = (char*)GameAllocIgnoreTag("AVAILABLE MODES", obj->count << 8);
        obj->available[0] = 0;

        Player* player = &g_game->players[g_game->localPlayer];
        int count = obj->count;
        for (int i = 0; i < count; i++) {
            if (obj->modes[i].width == player->info->width
                && obj->modes[i].height == player->info->height) {
                if (g_game->gui.layer->clickMode == 2) {
                    i--;
                    if (i < 0)
                        i = count - 1;
                } else {
                    i++;
                    if (i >= count)
                        i = 0;
                }
                DisplayMode& mode = obj->modes[i];
                player->info->width = (unsigned short)mode.width;
                player->info->height = (unsigned short)mode.height;
                BroadcastPlayerInfo();
                g_game->width_37f1b = mode.width;
                g_game->height_37f1f = mode.height;
                break;
            }
        }
    }
    GameFreeThunk(obj->modes);
    GameFreeThunk(obj);
}

// FUNCTION: 0x446450
void UpdateWatchingGadgets()
{
    PlayerInfo* info = g_game->players_1b63[g_game->localPlayer].info;
    SetButtonStageByName((Gui*)&g_game->gui, "WATCHING", info->watching);
    SetButtonStageByName((Gui*)&g_game->gui, "GAMEOPEN", !info->closed);
    MarkChanged((Dialog*)&g_game->gui);
}

// FUNCTION: 0x4464d0
void __stdcall HandleControlDialogClick(Gui* gui)
{
    PlayerInfo* info = g_game->players_1b63[g_game->localPlayer].info;
    if (gui->hotGadgetIndex != -1) {
        char buf[100];
        for (int i = 0; i < 10; i++) {
            sprintf(buf, "LIVEPLYR%d", i);
            if (IsCurrentGadgetNamed((Dialog *)gui, buf)) {
                OpenRejectDialog(i);
                return;
            }
        }
        if (IsCurrentGadgetNamed((Dialog *)gui, "WATCHING")) {
            info->watching = !info->watching;
            PlaySoundByName("Options", 0);
            info = g_game->players_1b63[g_game->localPlayer].info;
            SetButtonStageByName((Gui*)&g_game->gui, "WATCHING", info->watching);
            SetButtonStageByName((Gui*)&g_game->gui, "GAMEOPEN", !info->closed);
            MarkChanged((Dialog *)((Gui*)&g_game->gui));
            BroadcastPlayerInfo();
        } else if (IsCurrentGadgetNamed((Dialog *)gui, "OK")) {
            UpdateNetGameInfo();
            PlaySoundByName("Options", 0);
            if (!info->watching) {
                for (int i = 0; i < 10; i++) {
                    if (g_game->players[i].active != 0) {
                        if (g_game->players[i].type == 3) {
                            if (g_game->players[i].info->bit6) {
                                RejectPlayer(g_game->players[i].id, 9);
                            }
                        }
                    }
                }
            }
            return;
        }
        ClearSelectedGadget(gui);
    }
}

// FUNCTION: 0x4466b0
void OpenControlDialog()
{
    PlayerInfo* info = g_game->players_1b63[g_game->localPlayer].info;
    if (info->bit6) {
        return;
    }
    SelectDialogLayer* gadget = LoadGuiLayer(&g_game->gui, "CONTROL.GUI", 0x800);
    gadget->handler = HandleControlDialogClick;
    gadget->data = g_game;
    RefreshAlliesScreen(1);
    info = g_game->players_1b63[g_game->localPlayer].info;
    SetButtonStageByName((Gui*)&g_game->gui, "WATCHING", info->watching);
    SetButtonStageByName((Gui*)&g_game->gui, "GAMEOPEN", !info->closed);
    MarkChanged((Dialog*)&g_game->gui);
    SetKeyboardInput((Dialog*)&g_game->gui, 1);
    RenderLayer((Gui *)((Dialog*)&g_game->gui), 0x40);
}

// FUNCTION: 0x4467c0
bool __stdcall ArePlayersAllied(Player* a, Player* b)
{
    if (a->alliance == 5)
        return false;
    return a->alliance == b->alliance;
}

static inline int IsPlaying(Player* p)
{
    return p->active != 0 && (p->type == 1 || p->type == 2 || p->type == 3)
        && p->index != 10;
}

static inline int IsCounted(Player* p)
{
    return (p->type == 1 || p->type == 2 || p->type == 3)
        && (p->unitCount != 0 || p->unitsCreated == 0);
}

// FUNCTION: 0x4467f0
int __stdcall CountPlayersInAlliance(int alliance)
{
    if (alliance == 5)
        return 0;
    int count = 0;
    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (g_game->bits_2a44_1.bit2) {
            if (p->alliance == alliance && IsPlaying(p) && IsCounted(p))
                count++;
        } else {
            if (p->alliance == alliance && IsPlaying(p))
                count++;
        }
    }
    return count;
}


// FUNCTION: 0x4468c0
char AreAllPlayersInOneAlliance()
{
    int total = CountComputerPlayers() + CountHumanPlayers();
    for (int alliance = 0; alliance < 5; alliance++) {
        if (CountPlayersInAlliance(alliance) == total)
            return 1;
    }
    return 0;
}

// FUNCTION: 0x4469c0
int __stdcall FindNextAlly(int player, int start)
{
    Player* players = g_game->players;
    if (start == 10)
        return -1;
    for (int i = start; i < 10; i++) {
        if ((players[i].alliance == players[player].alliance && players[i].type != 0
             && players[i].alliance != 5) || i == player)
            return i;
    }
    return -1;
}

// FUNCTION: 0x446a50
void RefreshTeamIcons()
{
    int i = 0;
    int teamIcon = 0;
    char buffer[0x40];

    for (; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (IsPlaying(p) && p->type != 4
            && (!(g_game->flags_2a44 & 4) || IsCounted(p))
            && (!(g_game->flags_2a44 & 4) || p->info->color != 0xff)) {
            if (g_game->bits_2a44_1.bit2) {
                sprintf(buffer, "TEAMICONS%d", teamIcon);
                teamIcon++;
            } else {
                sprintf(buffer, "TEAMICONS%d", i);
            }

            int alliance = p->alliance;
            int count = CountPlayersInAlliance(alliance);

            switch (count) {
            case 0:
                SetButtonStageByName((Gui*)&g_game->gui, buffer, 10);
                break;
            case 1:
                SetButtonStageByName((Gui*)&g_game->gui, buffer, alliance * 2 + 1);
                break;
            default:
                SetButtonStageByName((Gui*)&g_game->gui, buffer, alliance * 2);
                break;
            }
        }
    }
    g_game->bits_2bee_1.dirty = 1;
}

#pragma auto_inline(off)
// FUNCTION: 0x446c70
void SyncMutualAlliances()
{
    for (int i = 0; i < 10; i++) {
        int j;
        int k;
        Player* p = &g_game->players[i];
        if (p->active == 0)
            continue;
        unsigned char type = p->type;
        if (type != 1 && type != 2 && type != 3)
            continue;
        if (p->index == 10)
            continue;
        if (type == 4)
            continue;
        j = 0;
        while ((k = FindNextAlly(i, j)) != -1) {
            j = k + 1;
            p->alliedBy[k] = 1;
            p->allied[k] = 1;
            Player* q = &g_game->players[k];
            q->info->flags_9d_wide |= 2;
            p->info->flags_9d_wide |= 2;
        }
        if (CountPlayersInAlliance(p->alliance) < 2)
            p->info->flags_9d_wide &= 0xfffd;
    }
}

#pragma auto_inline(on)

static inline int IsSelectable(Player* p)
{
    return (p->type == 1 || p->type == 2 || p->type == 3)
        && p->index != 10;
}

#pragma auto_inline(off)
// FUNCTION: 0x446e90
void __stdcall ClearAlliances(Player* player)
{
    if (player->alliance != 5) {
        for (int i = 0; i < 10; i++) {
            Player* p = &g_game->players[i];
            if (g_game->players[i].active
                && IsSelectable(p)
                && g_game->players[i].type != 4
                && g_game->players[i].alliance == player->alliance
                && i != player->index) {
                SetAlliance(player->id, p->id, 0, 1);
                player->info->flags_9d_wide &= 0xfffd;
            }
        }
    }
}
#pragma auto_inline(on)
// FUNCTION: 0x446f50
void __stdcall CyclePlayerAlliance(int index)
{
    int colour = g_game->players[index].alliance;
    Player* player = &g_game->players[index];
    ClearAlliances(player);
    player->alliance = (colour + 1) % 6;
    BroadcastAllyTeam(player);
    SyncMutualAlliances();
    RefreshTeamIcons();
}

__inline int IsLiveTypeInRebuildAllyList(Player* p)
{
    if (p->type != 1 && p->type != 2 && p->type != 3)
        return 0;
    return 1;
}

__inline int IsAllyInRebuildAllyList(Player* p)
{
    int act = p->active;
    if (!act)
        return 0;
    if (p->info->flags_9b & 0x40)
        return 0;
    if (!p->active)
        return 0;
    if (!IsLiveTypeInRebuildAllyList(p))
        return 0;
    if (p->index == 10)
        return 0;
    if (!IsLiveTypeInRebuildAllyList(p))
        return 0;
    if (p->unitCount == 0 && p->unitsCreated != 0)
        return 0;
    return 1;
}

__inline int IsLiveInRebuildAllyList(Player* p)
{
    if (!p->active)
        return 0;
    if (!IsLiveTypeInRebuildAllyList(p))
        return 0;
    if (p->index == 10)
        return 0;
    if (p->unitCount == 0 && p->unitsCreated != 0)
        return 0;
    if (p->info->color == 0xff)
        return 0;
    return 1;
}

// FUNCTION: 0x446fb0
void RebuildAllyList()
{
    char text[52];
    unsigned char* a = &g_game->players_1b63[g_game->localPlayer].allied[0];
    unsigned char* b = &g_game->players_1b63[g_game->localPlayer].alliedBy[0];

    if (IsScreenNamed(&g_game->gui, "ALLIES.GUI") != 0) {
        int i;
        for (i = 0; i < 10; ++i, ++a, ++b) {
            if (IsAllyInRebuildAllyList(&g_game->players_1b63[i]) && i != g_game->localPlayer
                && IsLiveInRebuildAllyList(&g_game->players_1b63[i])) {
                sprintf(text, "LIVEALLY%d", i);
                SetButtonStageByName(&g_game->gui, text, (*b << 1) | *a);
            }
        }
        MarkChanged((Dialog *)&g_game->gui);
    }
}

// FUNCTION: 0x447150
void __stdcall HandleAlliesClick(Gui* gadget)
{
    void* entries = gadget->layer->entries;
    char buf[100];

    if (gadget->hotGadgetIndex == -1) {
        g_game->flags_37ebe &= 0xffdf;
        return;
    }

    int i = 0;
    Player* local = &g_game->players[g_game->localPlayer];

    for (; i < 10; i++) {
        sprintf(buf, "LIVEALLY%d", i);
        Player* p = &g_game->players[i];
        if (IsCurrentGadgetNamed((Dialog *)gadget, buf) && p->active
            && (p->type == 1 || p->type == 2 || p->type == 3)
            && p->index != 10) {
            PlaySoundByName("Options", 0);
            SetAlliance(local->id, p->id, local->allied[i] ^= 1, 0);
            char* verb = (char*)(local->allied[i] ? "allied with" : "broke alliance with");
            sprintf(buf, " %s %s", Translate(verb),
                    (char*)g_game + 0x1b8e + i * 0x14b);
            SendChatMessage(local, buf, 4, 0);
            RebuildAllyList();
            DrawButton(&g_game->gui, gadget->hotGadgetIndex);
        }
    }

    if (IsCurrentGadgetNamed((Dialog *)gadget, "VICTORY")) {
        PlaySoundByName("Options", 0);
        ClearSelectedGadget(gadget);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gadget, "OK")) {
        PlaySoundByName("Options", 0);
        int old = (local->info->flags_9d_wide >> 1) & 1;
        int index = FindGadgetIndex(entries, "VICTORY", 1);
        unsigned int value = GetButtonStage(gadget, index);
        local->info->flags_9d_wide = (local->info->flags_9d_wide & 0xfffd) | ((value & 1) << 1);
        if (old != ((local->info->flags_9d_wide >> 1) & 1))
            BroadcastPlayerInfo();
    } else {
        ClearSelectedGadget(gadget);
    }
}

static inline int IsPlayerTypeInRefreshAlliesScreen(Player* p)
{
    return p->type == 1 || p->type == 2 || p->type == 3;
}

static inline int IsCountedInRefreshAlliesScreen(Player* p)
{
    if (!IsPlayerTypeInRefreshAlliesScreen(p))
        return 0;
    if (p->unitCount == 0 && p->unitsCreated != 0)
        return 0;
    return 1;
}

static inline int IsWatchingInRefreshAlliesScreen(Player* p)
{
    return p->active != 0 && (p->info->flags_9b & 0x40);
}

static inline int IsActiveInRefreshAlliesScreen(Player* p)
{
    return p->active != 0
        && (p->type == 1 || p->type == 2 || p->type == 3)
        && p->index != 10;
}

// FUNCTION: 0x447380
void __stdcall RefreshAlliesScreen(int param_1)
{
    Gadget* entries = g_game->gui.layer->entries;
    int i;
    int n;
    Player* local = &g_game->players[g_game->localPlayer];
    char player[20];
    char teamicons[20];
    char ally[20];
    char live[20];
    char logo[20];
    char name[0x80];

    for (i = 0, n = 0; i < 10; i++) {
        sprintf(player, "PLAYER%d", i);
        sprintf(logo, "LOGO%d", i);
        sprintf(ally, "ALLY%d", i);
        sprintf(teamicons, "TEAMICONS%d", i);
        SetGadgetActiveByName(&g_game->gui, player, 0);
        SetGadgetActiveByName(&g_game->gui, logo, 0);
        SetGadgetActiveByName(&g_game->gui, ally, 0);
        SetGadgetActiveByName(&g_game->gui, teamicons, 0);

        Player* p = &g_game->players[i];
        if (!IsWatchingInRefreshAlliesScreen(p) && IsActiveInRefreshAlliesScreen(p)
            && (i != g_game->localPlayer || param_1 == 0)
            && (!(g_game->flags_2a44 & 4) || IsCountedInRefreshAlliesScreen(p))
            && p->info->color != 0xff) {
            sprintf(player, "PLAYER%d", n);
            sprintf(logo, "LOGO%d", n);
            sprintf(ally, "ALLY%d", n);
            sprintf(teamicons, "TEAMICONS%d", n);
            name[0] = 0;
            strncat(name, p->name, 0x7f);

            int idx = FindGadgetIndex(entries, player, 0xe);
            if (entries[idx].type == 1) {
                Gadget* e = FindGadgetOrNull(entries, player);
                if (e != 0 && (e->attribs & 0x4000)) {
                    strcat(name, "|");
                    strcat(name, p->name);
                }
            }

            SetTranslatedTextByName(&g_game->gui, player, name, 0x80);
            SetGadgetActiveByName(&g_game->gui, player, 1);
            sprintf(live, "LIVEPLYR%d", i);
            SetGadgetName((Dialog *)&g_game->gui, player, live);

            if (p->active != 0
                && IsPlayerTypeInRefreshAlliesScreen(p)
                && p->index != 10
                && (p->unitCount != 0 || p->unitsCreated == 0)
                && p->type != 1
                && p->type != 2
                && !(p->type == 3 && p->info->kind == 2)) {
                Player* q = &g_game->players[g_game->localPlayer];
                if (q->active != 0
                    && IsPlayerTypeInRefreshAlliesScreen(q)
                    && q->index != 10
                    && (q->unitCount != 0 || q->unitsCreated == 0)) {
                    SetGadgetActiveByName(&g_game->gui, ally, 1);
                }
            }

            sprintf(live, "LIVEALLY%d", i);
            SetGadgetName((Dialog *)&g_game->gui, ally, live);

            if (p->alliance == local->alliance && p->alliance != 5) {
                SetGadgetGrayedOutByName(&g_game->gui, live, 1);
            }

            SetGadgetActiveByName(&g_game->gui, teamicons, 1);

            int value;
            if (p->active != 0 && (p->type == 1 || p->type == 2)
                && !(g_game->flags_2a44 & 4)) {
                value = 0;
            } else {
                value = 1;
            }
            SetGadgetGrayedOutByName(&g_game->gui, teamicons, value);

            Gadget* e2 = FindGadgetChecked_E(entries, logo);
            if (e2 != 0) {
                e2->active = 1;
                e2->u.hotspot.gaf = (GafEntry*)g_game->logos32;
                e2->u.hotspot.index = p->info->color;
                e2->u.hotspot.flags &= ~1;
            }

            n++;
        }
    }
}

static inline int IsPlayingInOpenAlliesDialog(Player* p)
{
    return p->active != 0
        && (p->type == 1 || p->type == 2 || p->type == 3)
        && p->index != 10;
}

static inline int IsCountedInOpenAlliesDialog(Player* p)
{
    return (p->type == 1 || p->type == 2 || p->type == 3)
        && (p->unitCount != 0 || p->unitsCreated == 0);
}

static inline int CountAllianceInOpenAlliesDialog(int alliance)
{
    int count = 0;
    unsigned char f = (g_game->flags_2a44 >> 2) & 1;
    for (int j = 0; j < 10; j++) {
        Player* q = &g_game->players[j];
        if (f) {
            if (q->alliance == alliance && IsPlayingInOpenAlliesDialog(q)
                && IsCountedInOpenAlliesDialog(q))
                count++;
        } else {
            if (q->alliance == alliance && IsPlayingInOpenAlliesDialog(q))
                count++;
        }
    }
    return count;
}

// FUNCTION: 0x4478b0
void OpenAlliesDialog()
{
    ListDialogLayer* gadget = LoadGuiLayer(&g_game->gui, "ALLIES.GUI", 0x800);
    gadget->handler = HandleAlliesClick;
    gadget->data = g_game;
    g_game->flags_37ebe |= 0x20;
    char* entries = (char*)g_game->gui.layer->entries;
    int i, j;
    for (i = 0; (j = FindGadgetIndex(entries, "ALLYx", 0xe)) != -1; i++)
        sprintf(entries + j * 0x15b + 2, "ALLY%d", i);
    for (i = 0; (j = FindGadgetIndex(entries, "TEAMICONSx", 0xe)) != -1; i++)
        sprintf(entries + j * 0x15b + 2, "TEAMICONS%d", i);
    RefreshAlliesScreen(0);
    RebuildAllyList();
    RefreshTeamIcons();
    Player* local = &g_game->players_1b63[g_game->localPlayer];
    int old = (local->info->flags_9d_wide >> 1) & 1;
    unsigned char win = (local->info->flags_9b >> 6) & 1;
    SetButtonStageByName((Gui*)&g_game->gui, "VICTORY", old);
    int alliance = local->alliance;
    int count;
    if (alliance == 5)
        count = 0;
    else
        count = CountAllianceInOpenAlliesDialog(alliance);
    SetGadgetGrayedOutByName(&g_game->gui, "VICTORY",
                 (count > 1 || win) ? 1 : 0);
    SetKeyboardInput(&g_game->gui, 1);
    RenderLayer((Gui *)((Dialog*)&g_game->gui), 0x40);
}

static inline int IsPlayingInHandleBattleRoomClick(Player* p)
{
    return p->active != 0
        && (p->type == 1 || p->type == 2 || p->type == 3)
        && p->index != 10;
}

static inline int IsCountedInHandleBattleRoomClick(Player* p)
{
    return (p->type == 1 || p->type == 2 || p->type == 3)
        && (p->unitCount != 0 || p->unitsCreated == 0);
}

static inline int IsLocalHumanInHandleBattleRoomClick(Player* p)
{
    return p->active != 0 && p->type == 1;
}

static inline int IsRemoteHumanInHandleBattleRoomClick(Player* p)
{
    return p->active != 0 && p->type == 3 && p->info->kind == 1;
}

static inline int IsLocalInHandleBattleRoomClick(Player* p)
{
    return p->active != 0 && (p->type == 1 || p->type == 2);
}

static inline int CountAllianceInHandleBattleRoomClick(int alliance)
{
    if (alliance == 5)
        return 0;
    int count = 0;
    unsigned char f = (g_game->flags_2a44 >> 2) & 1;
    for (int j = 0; j < 10; j++) {
        Player* q = &g_game->players[j];
        if (f) {
            if (q->alliance == alliance && IsPlayingInHandleBattleRoomClick(q) && IsCountedInHandleBattleRoomClick(q))
                count++;
        } else {
            if (q->alliance == alliance && IsPlayingInHandleBattleRoomClick(q))
                count++;
        }
    }
    return count;
}

inline int FindFreeLobbyLogo()
{
    int used[10];
    memset(used, 0, sizeof(used));
    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (p->active && (p->type == 1 || p->type == 2 || p->type == 3) && p->index != 10)
            used[p->info->color < 9 ? p->info->color : 9] = 1;
    }
    int result = 0;
    for (int j = 0; j < 10; j++) {
        if (!used[j]) {
            result = j;
            break;
        }
    }
    return result;
}

int LobbyMapCrcMatches()
{
    if (!g_game->net->GetTerrainLength()) {
        return 0;
    }
    unsigned char me = FindHostSlot();
    PlayerInfo* data = 0;
    int check = 0;
    if (me != 10) {
        data = g_game->players[me].info;
        if (data->versionMajor >= 2)
            check = 1;
        else if (data->versionMajor == 1 && data->versionMinor >= 2)
            check = 1;
    }
    if (!check) {
        return 1;
    }
    if (g_game->net->ComputeMapChecksum() != data->mapCrc)
        return 0;
    return 1;
}

void __stdcall CyclePlayerAllianceInBattleRoom(int index)
{
    Player* player = &g_game->players[index];
    int colour = g_game->players[index].alliance;
    ClearAlliances(player);
    player->alliance = (colour + 1) % 6;
    BroadcastAllyTeam(player);
    SyncMutualAlliances();
    RefreshTeamIcons();
}

// FUNCTION: 0x447b10
void __stdcall HandleBattleRoomClick(Gui* gadget)
{
    char text[250];
    Gadget* entries = gadget->layer->entries;

    if (gadget->hotGadgetIndex == -1) {
        GameFreeThunk((char *)g_game->list);
        (*(char **)&g_game->list) = 0;
        g_battleRoomSlotsBuilt = 0;
        SyncMutualAlliances();
        return;
    }

    int lp = g_game->localPlayer;
    Player* me = &g_game->players[lp];
    int canAdd = IsHostLocal();
    int i = 0;
    while (1) {
        Player* p = &g_game->players[i];

        sprintf(text, "LOGO%d", i);
        if (IsCurrentGadgetNamed((Dialog *)gadget, text) && IsLocalInHandleBattleRoomClick(p)) {
            PlaySoundByName("Multi", 0);
            RequestPlayerColor(p->info->color + 1);
            g_game->bits_2bee_1.dirty = 1;
            BroadcastPlayerInfo();
        }

        sprintf(text, "PLAYER%d", i);
        if (IsCurrentGadgetNamed((Dialog *)gadget, text) && i != lp) {
            PlaySoundByName("Multi", 0);
            char type = p->type;
            if (type == 0 && canAdd) {
                                p->SetType(4);
                p->id = -1;
                g_game->field_499--;
            } else if (type != 4 && type != 0) {
                if (p->active != 0 && type == 2 && GetTicks() - p->joinTime > 30) {
                    RejectPlayer(p->id, 1);
                    p->SetType(0);
                } else if (canAdd && p->active != 0 && p->type == 3) {
                    OpenRejectDialog(i);
                }
            } else {
                if (type == 4) {
                    p->SetType(0);
                    g_game->field_499++;
                    UpdateNetGameInfo();
                }
                if (g_game->players[FindHostSlot()].info->closed) {
                    OpenMessageBox(&g_game->gui, Translate("Can't add another player when game is closed."), 500, 1, 1);
                    p->SetType(0);
                    g_game->bits_2bee_1.dirty = 1;
                    break;
                }
                if (g_game->players[FindHostSlot()].info->commander != 2 && !CountLocalComputerPlayers()) {
                    CreateLocalPlayer(i, 2);
                    p->info->color = FindFreeLobbyLogo();
                }
            }
            g_game->bits_2bee_1.dirty = 1;
            UpdateNetGameInfo();
            BroadcastPlayerInfo();
        }

        sprintf(text, "SIDE%d", i);
        if (IsCurrentGadgetNamed((Dialog *)gadget, text)) {
            PlaySoundByName("Multi", 0);
            if (p->active != 0 && p->info->bit6) {
                p->info->bit6 = 0;
                p->info->side = 0;
            } else {
                p->info->side++;
                if (p->info->side >= g_game->sides) {
                    p->info->side = 0;
                    if (g_game->players[FindHostSlot()].info->watching
                        && p->active != 0 && p->type == 1) {
                        p->info->bit6 = 1;
                    } else {
                        SetButtonStageByName(gadget, text, 0);
                        DrawButton(gadget, gadget->hotGadgetIndex);
                    }
                }
            }
            g_game->bits_2bee_1.dirty = 1;
            ReportGameEvent(4);
            BroadcastPlayerInfo();
        }

        sprintf(text, "ALLY%d", i);
        if (IsCurrentGadgetNamed((Dialog *)gadget, text)) {
            me->allied[i] ^= 1;
            SetAlliance(me->id, p->id, me->allied[i], 0);
            char same;
            if (me->alliance == 5)
                same = 0;
            else
                same = me->alliance == p->alliance;
            if (same) {
                ClearAlliances(me);
                me->alliance = 5;
                BroadcastAllyTeam(me);
            }
            if (me->alliedBy[i] << 1 == 3 | me->allied[i])
                PlaySoundByName("Ally", 0);
            else
                PlaySoundByName("Multi", 0);
            sprintf(text, " %s %s",
                    Translate(me->allied[i] ? "allied with" : "broke alliance with"),
                    g_game->players[i].name);
            SendChatMessage(me, text, 4, 0);
            g_game->bits_2bee_1.dirty = 1;
            BroadcastPlayerInfo();
        }

        sprintf(text, "TEAMICONS%d", i);
        if (IsCurrentGadgetNamed((Dialog *)gadget, text)) {
            PlaySoundByName("Ally", 0);
            CyclePlayerAllianceInBattleRoom(i);
            BroadcastAllyTeam(p);
        }

        sprintf(text, "RES%d", i);
        if (IsCurrentGadgetNamed((Dialog *)gadget, text) && IsLocalHumanInHandleBattleRoomClick(p)) {
            PlaySoundByName("Multi", 0);
            CyclePlayerDisplayMode();
            ClearSelectedGadget(gadget);
            g_game->bits_2bee_1.dirty = 1;
            return;
        }

        sprintf(text, "READY%d", i);
        if (IsCurrentGadgetNamed((Dialog *)gadget, text) && IsLocalHumanInHandleBattleRoomClick(p)) {
            PlaySoundByName("Multi", 0);
            if (LobbyMapCrcMatches()) {
                p->info->bit5 = GetGadgetStatus(&g_game->gui, FindGadgetIndex(entries, text, 1));
                if (p->info->host) {
                    strcpy(entries->u.names.choice, "START");
                    g_game->gui.layer->current = FindGadgetIndex(entries, "START", 1);
                }
                for (int j = 0; j < 10; j++) {
                    Player* q = &g_game->players[j];
                    if (IsLocalInHandleBattleRoomClick(q))
                        q->info->bit5 = g_game->players[g_game->localPlayer].info->bit5;
                }
                g_game->bits_2bee_1.dirty = 1;
                BroadcastPlayerInfo();
            } else {
                SetGadgetStatusByName(&g_game->gui, text, 0);
            }
        }
        i++;
        if (i >= 10)
            break;
    }
    if (i != g_game->numPlayers)
        g_game->bits_2bee_1.dirty = 1;

    if (IsCurrentGadgetNamed((Dialog *)gadget, "PREVMENU")) {
        PlaySoundByName("Previous", 0);
        for (int j = 0; j < 10; j++) {
            Player* q = &g_game->players[j];
            if (IsLocalInHandleBattleRoomClick(q))
                RejectPlayer(q->id, 2);
        }
        (*(char*)&g_game->frontendNextSubState) = 3;
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gadget, "MESSAGE")) {
        Gadget* box = FindGadgetChecked_B(entries, "MESSAGE");
        char* msg = box->u.text;
        if (strlen(msg) != 0) {
            if (_strcmpi(msg, "+syncerr") == 0) {
                char* s = ((UnitSync *)g_game->sync)->GetSyncStatusText();
                if (s)
                    AddMessage(s, 4, 0, 10);
            } else {
                SendChatMessage(me, msg, 4, 0);
                if (g_usePacketManager)
                    g_packetManager.SendAllQueued(1);
            }
            g_game->bits_2bee_1.dirty = 1;
            strcpy(msg, "");
        }
        BeginTextEdit(&g_game->gui, FindGadgetIndex(g_game->gui.layer->entries, "MESSAGE", 3));
    } else if (IsCurrentGadgetNamed((Dialog *)gadget, "COMMANDER")) {
        PlaySoundByName("Multi", 0);
        me->info->commander++;
        if (me->info->commander > 2)
            me->info->commander = 0;
        BroadcastPlayerInfo();
        UpdateNetGameInfo();
        g_game->bits_2bee_1.dirty = 1;
    } else if (IsCurrentGadgetNamed((Dialog *)gadget, "LOSTYPE")) {
        PlaySoundByName("Multi", 0);
        if (!me->info->los) {
            me->info->los = 1;
            me->info->losType = 1;
        } else if (me->info->losType == 1) {
            me->info->losType = 0;
        } else {
            me->info->los = 0;
        }
        BroadcastPlayerInfo();
        UpdateNetGameInfo();
        g_game->bits_2bee_1.dirty = 1;
    } else if (IsCurrentGadgetNamed((Dialog *)gadget, "WATCHING")) {
        PlaySoundByName("Multi", 0);
        me->info->watching = !me->info->watching;
        if (!me->info->watching && me->active != 0 && me->info->bit6)
            me->info->bit6 = 0;
        BroadcastPlayerInfo();
        UpdateNetGameInfo();
        g_game->bits_2bee_1.dirty = 1;
    } else if (IsCurrentGadgetNamed((Dialog *)gadget, "CHEATING")) {
        PlaySoundByName("Multi", 0);
        me->info->cheating = !me->info->cheating;
        BroadcastPlayerInfo();
        g_game->bits_2bee_1.dirty = 1;
    } else if (IsCurrentGadgetNamed((Dialog *)gadget, "FIXEDLOC")) {
        PlaySoundByName("Multi", 0);
        me->info->fixedloc = !me->info->fixedloc;
        BroadcastPlayerInfo();
        g_game->bits_2bee_1.dirty = 1;
    } else if (IsCurrentGadgetNamed((Dialog *)gadget, "MAPPING")) {
        PlaySoundByName("Multi", 0);
        me->info->mapping = GetButtonStageByName(gadget, "MAPPING") == 0;
        BroadcastPlayerInfo();
        UpdateNetGameInfo();
        g_game->bits_2bee_1.dirty = 1;
    } else if (IsCurrentGadgetNamed((Dialog *)gadget, "START")) {
        int count = 0;
        PlaySoundByName("BigButton", 0);
        for (int j = 0; j < 10; j++) {
            Player* q = &g_game->players[j];
            if ((IsLocalHumanInHandleBattleRoomClick(q) || IsRemoteHumanInHandleBattleRoomClick(q)) && q->info->f9d_2)
                count++;
        }
        if (count < 1 || (count < 2 && CountHumanPlayers() > 3) || (count < 3 && CountHumanPlayers() > 6)) {
            ClearSelectedGadget(&g_game->gui);
            OpenMessageBox(gadget, Translate("There are not enough game CDs present to play"), 200, 1, 1);
            return;
        }
        int total = CountComputerPlayers() + CountHumanPlayers();
        for (int t = 0; t < 5; t++) {
            if (CountAllianceInHandleBattleRoomClick(t) == total) {
                ClearSelectedGadget(&g_game->gui);
                OpenMessageBox(gadget, Translate("Can not start game with all players on the same team."), 200, 1, 1);
                return;
            }
        }
        if (!g_game->net->HasMissionName()) {
            PlaySoundByName("Multi", 0);
            OpenMultiMapSelector();
            goto done;
        }
        if (!me->info->watching) {
            for (int j = 0; j < 10; j++) {
                Player* q = &g_game->players[j];
                if (q->active != 0 && q->type == 3 && (q->info->flags_9b & 0x40))
                    RejectPlayer(q->id, 9);
            }
        }
        (*(char*)&g_game->frontendNextSubState) = 0x11;
        me->info->started = 1;
        UpdateNetGameInfo();
        g_game->multiLineOfSight = me->info->los;
        g_game->multiLOSType = me->info->losType;
        g_game->multiCommanderDeath = me->info->commander;
        g_game->options->fixedLocations = me->info->fixedloc;
        g_game->multiMapping = me->info->mapping;
        SaveSettings();
        g_game->difficulty = 2;
        return;
    } else if (IsCurrentGadgetNamed((Dialog *)gadget, "GAMEOPEN")) {
        PlaySoundByName("Multi", 0);
        me->info->closed = GetButtonStageByName(gadget, "GAMEOPEN") == 0;
        BroadcastPlayerInfo();
        UpdateNetGameInfo();
        g_game->bits_2bee_1.dirty = 1;
    } else if (IsCurrentGadgetNamed((Dialog *)gadget, "RESTRICTIONS")) {
        PlaySoundByName("Options", 0);
        OpenUnitRestrictions();
        ClearSelectedGadget(gadget);
    } else {
        int hit = IsCurrentGadgetNamed((Dialog *)gadget, "MAP");
        if (!hit)
            hit = IsCurrentGadgetNamed((Dialog *)gadget, "MAPNAME");
        if (hit) {
            PlaySoundByName("Multi", 0);
            if (me->info->host) {
                OpenMultiMapSelector();
            } else {
                ListDialogLayer* view = LoadGuiLayer(&g_game->gui, "VIEWMAP.GUI", 0x900);
                view->handler = HandleViewMapClick;
                LoadPictureCached("DVIEWMAP", 0, 0, 0);
                ShowSelectedMapInfo();
                SetKeyboardInput(&g_game->gui, 1);
                RenderLayer(&g_game->gui, 0x40);
            }
        }
    }
done:
    ClearSelectedGadget(gadget);
}

// FUNCTION: 0x448bf0
void __stdcall UpdateSideGadget(int side)
{
    char name[20];
    Player* p = &g_game->players[side];

    sprintf(name, "SIDE%d", side);
    SetButtonStageByName((Gui*)&g_game->gui, name,
                 (p->active != 0 && (p->info->flags_9b & 0x40)) ? 2 : p->info->side);
}

static inline int IsPlayingInRefreshBattleRoomRows(Player* p)
{
    return p->active != 0
        && (p->type == 1 || p->type == 2 || p->type == 3)
        && p->index != 10;
}

static inline int IsWatchingInRefreshBattleRoomRows(Player* p)
{
    return p->active != 0 && (p->info->flags_9b & 0x40);
}

static inline int IsLocalHumanInRefreshBattleRoomRows(Player* p)
{
    return p->active != 0 && p->type == 1;
}

static inline int IsLocalAiInRefreshBattleRoomRows(Player* p)
{
    return p->active != 0 && p->type == 2;
}

static inline int IsRemoteHumanInRefreshBattleRoomRows(Player* p)
{
    return p->active != 0 && p->type == 3 && p->info->kind == 1;
}

static inline int IsRemoteAiInRefreshBattleRoomRows(Player* p)
{
    return p->active != 0 && p->type == 3 && p->info->kind == 2;
}

static inline int IsLocalInRefreshBattleRoomRows(Player* p)
{
    return p->active != 0 && (p->type == 1 || p->type == 2);
}

// FUNCTION: 0x448c70
void RefreshBattleRoomRows()
{
    char name[20];
    char* str;
    unsigned int minPing = 0xffffffff;
    int count = 0;
    Player* me = &g_game->players[g_game->localPlayer];
    int ready = me->info->bit5;
    Gadget* output = FindGadgetChecked(g_game->gui.layer->entries, "OUTPUT");

    int end = g_game->tail;
    int start = g_game->head;
    if (end < start)
        end += 30;
    if (end - start > output->height / (GetFontLineHeight() + 2)) {
        g_game->head++;
        if (g_game->head >= 30)
            g_game->head = 0;
    }
    if (g_game->tail != g_game->head) {
        for (int i = g_game->head; g_game->tail != i; ) {
            char* line = SkipTextLines((char *)g_game->list, count);
            strcpy(line, g_game->chatHudRing_12ef[i]);
            count++;
            i++;
            if (i == 30)
                i = 0;
        }
    }
    UpdateBattleRoomFlags();
    output->u.list.rowCount = count;

    Gadget* mapname = FindGadgetChecked_C(g_game->gui.layer->entries, "MAPNAME");
    char* map = g_game->net->GetMissionName();
    if (!g_game->net->HasMissionName()) {
        mapname->colour = 0xc;
        SetTranslatedTextByName(&g_game->gui, "MAPNAME", "NOT SELECTED", 0);
    } else {
        char* cur = g_game->net->GetTranslatedName();
        str = mapname->u.text;
        int differs = strcmp(str, cur);
        if (differs) {
            if (IsScreenNamed(&g_game->gui, "viewmap.gui"))
                ShowSelectedMapInfo();
            else
                g_game->net->LoadMissionByName(map);
        }
        if (!LobbyMapCrcMatches()) {
            mapname->colour = ((int)GetTicks() / 30 & 1) ? 0xc : 0;
            if (differs) {
                SendChatMessage(me, Translate("does not have this map"), 4, 0);
                me->info->bit5 = 0;
                sprintf(name, "READY%d", g_game->localPlayer);
                SetGadgetStatusByName(&g_game->gui, name, 0);
                BroadcastPlayerInfo();
            }
            if (!g_game->players[g_game->localPlayer].info->host)
                SetGadgetGrayedOutByName(&g_game->gui, "MAP", 1);
        } else {
            mapname->colour = 0;
            SetGadgetGrayedOutByName(&g_game->gui, "MAP", 0);
        }
        strcpy(str, g_game->net->GetTranslatedName());
    }

    int i;
    for (i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (IsLocalInRefreshBattleRoomRows(p))
            p->info->bit5 = g_game->players[g_game->localPlayer].info->bit5;
    }
    for (i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (g_game->players[g_game->localPlayer].info->commander == 2
            && (IsLocalAiInRefreshBattleRoomRows(p) || IsRemoteAiInRefreshBattleRoomRows(p))) {
            RejectPlayer(p->id, 0xb);
            BroadcastPlayerInfo();
        }
        if (FindHostSlot() != 10
            && !g_game->players[FindHostSlot()].info->watching
            && p->active != 0) {
            unsigned short flags = p->info->flags;
            if (flags & 0x40) {
                p->info->flags = flags & ~0x40;
                p->info->side = 0;
                BroadcastPlayerInfo();
            }
        }
    }
    SyncMutualAlliances();
    RefreshTeamIcons();

    char* entries = (char*)g_game->gui.layer->entries;
    Player* local = &g_game->players[g_game->localPlayer];
    for (unsigned char n = 0; n < 10; n++) {
        char text[32];
        char res[52];
        char blocked[52];
        Player* p = &g_game->players[n];
        Gadget* e;
        if (!IsPlayingInRefreshBattleRoomRows(p) && !IsWatchingInRefreshBattleRoomRows(p)) {
            sprintf(name, "CD%d", n);
            SetGadgetActiveByName(&g_game->gui, name, 0);
            SetGadgetGrayedOutByName(&g_game->gui, name, 0);
            sprintf(name, "PLAYER%d", n);
            char* s = "UNUSED";
            if (p->type == 4) {
                sprintf(blocked, "[%s]", Translate("BLOCKED"));
                s = blocked;
            }
            strncpy(text, s, 0x1e);
            SetTranslatedTextByName(&g_game->gui, name, text, 0);
            TruncateGadgetText(&g_game->gui, FindGadgetIndex(entries, name, 0xe));
            SetGadgetGrayedOutByName(&g_game->gui, name, ready);
            sprintf(name, "LOGO%d", n);
            e = FindGadgetChecked_E(entries, name);
            if (e)
                e->active = 0;
            sprintf(name, "SIDE%d", n);
            e = FindGadgetOrNull(entries, name);
            if (e)
                e->active = 0;
            if (n != g_game->localPlayer) {
                sprintf(name, "ALLY%d", n);
                e = FindGadgetOrNull(entries, name);
                if (e)
                    e->active = 0;
            }
            sprintf(name, "TEAMICONS%d", n);
            e = FindGadgetOrNull(entries, name);
            if (e)
                e->active = 0;
            sprintf(name, "RES%d", n);
            e = FindGadgetChecked_C(entries, name);
            if (e)
                e->active = 0;
            sprintf(name, "PING%d", n);
            e = FindGadgetChecked_C(entries, name);
            if (e)
                e->active = 0;
            sprintf(name, "MEM%d", n);
            e = FindGadgetChecked_C(entries, name);
            if (e)
                e->active = 0;
            sprintf(name, "READY%d", n);
            e = FindGadgetOrNull(entries, name);
            if (e) {
                e->flag_bit = 1;
                e->status = 0;
                e->active = 0;
            }
        } else {
            sprintf(name, "CD%d", n);
            SetGadgetActiveByName(&g_game->gui, name,
                         ((IsLocalHumanInRefreshBattleRoomRows(p) || IsRemoteHumanInRefreshBattleRoomRows(p))
                          && p->info->f9d_2) ? 1 : 0);
            SetGadgetGrayedOutByName(&g_game->gui, name, 0);
            sprintf(name, "LOGO%d", n);
            e = FindGadgetChecked_E(entries, name);
            if (e) {
                e->active = (p->info->color == 0xff && !ready) ? 0 : 1;
                e->u.hotspot.clickable = !ready;
                e->u.hotspot.gaf = (GafEntry*)g_game->logos32;
                e->u.hotspot.index = p->info->color;
            }
            sprintf(name, "PLAYER%d", n);
            strncpy(text, p->name, 0x1e);
            SetTranslatedTextByName(&g_game->gui, name, text, 0);
            TruncateGadgetText(&g_game->gui, FindGadgetIndex(entries, name, 0xe));
            SetGadgetGrayedOutByName(&g_game->gui, name, ready);
            sprintf(name, "SIDE%d", n);
            e = FindGadgetOrNull(entries, name);
            if (e) {
                UpdateSideGadget(n);
                e->active = 1;
                SetGadgetGrayedOutByName(&g_game->gui, name, (IsLocalInRefreshBattleRoomRows(p) && !ready) ? 0 : 1);
            }
            sprintf(name, "ALLY%d", n);
            e = FindGadgetOrNull(entries, name);
            if (e) {
                SetButtonStageByName((Gui*)&g_game->gui, name,
                             me->alliedBy[n] << 1 | me->allied[n]);
                e->active = (IsLocalHumanInRefreshBattleRoomRows(p) || IsWatchingInRefreshBattleRoomRows(p)
                              || IsLocalAiInRefreshBattleRoomRows(p) || IsRemoteAiInRefreshBattleRoomRows(p)
                              || IsWatchingInRefreshBattleRoomRows(local)) ? 0 : 1;
                SetGadgetGrayedOutByName(&g_game->gui, name, ready);
            }
            sprintf(name, "TEAMICONS%d", n);
            e = FindGadgetOrNull(entries, name);
            if (e) {
                e->active = (IsWatchingInRefreshBattleRoomRows(p) || 0) ? 0 : 1;
                SetGadgetGrayedOutByName(&g_game->gui, name, (IsLocalInRefreshBattleRoomRows(p) && !ready) ? 0 : 1);
            }
            sprintf(name, "RES%d", n);
            if (!IsLocalHumanInRefreshBattleRoomRows(p) && !IsRemoteHumanInRefreshBattleRoomRows(p))
                sprintf(res, "%s", "n/a");
            else
                sprintf(res, "%dx%d", p->info->width, p->info->height);
            SetTranslatedTextByName(&g_game->gui, name, res, 0);
            if (IsLocalHumanInRefreshBattleRoomRows(p)) {
                SetGadgetGrayedOutByName(&g_game->gui, name, ready);
            } else {
                e = FindGadgetChecked_C(entries, name);
                if (e)
                    e->active = 1;
            }
            sprintf(name, "PING%d", n);
            e = FindGadgetChecked_C(entries, name);
            if (IsRemoteHumanInRefreshBattleRoomRows(p)) {
                if (e) {
                    str = e->u.text;
                    _itoa(p->ping, str, 10);
                    if (IsHostLocal())
                        strcat(str, ((UnitSync *)g_game->sync)->IsPlayerSynced(GetSlotDpid(n)) ? ":s" : "");
                    if (minPing >= p->ping)
                        minPing = p->ping;
                    e->active = 1;
                }
            } else {
                SetTranslatedTextByName(&g_game->gui, name, "n/a", 0);
                if (e)
                    e->active = 1;
            }
            sprintf(name, "MEM%d", n);
            e = FindGadgetChecked_C(entries, name);
            if (!IsLocalHumanInRefreshBattleRoomRows(p) && !IsRemoteHumanInRefreshBattleRoomRows(p)) {
                sprintf(e->u.text, "%s", "n/a");
                e->colour = p->info->memory < g_game->net->GetTerrainSizeTier() ? 0xc : 0;
                e->active = 1;
            } else {
                sprintf(e->u.text, "%d", p->info->memory);
                e->colour = p->info->memory < g_game->net->GetTerrainSizeTier() ? 0xc : 0;
                e->active = 1;
            }
            sprintf(name, "READY%d", n);
            e = FindGadgetOrNull(entries, name);
            if (e) {
                e->status = g_game->players[n].info->bit5;
                e->active = 1;
                e->flag_bit = !IsLocalHumanInRefreshBattleRoomRows(p);
            }
        }
    }
    MarkChanged((Dialog *)&g_game->gui);
    PlayerInfo* info = me->info;
    if (info->host && minPing < info->pingLimit) {
        info->pingLimit = minPing;
        UpdateNetGameInfo();
    }
}

// FUNCTION: 0x44afb0
void __stdcall HandleEndMultiClick(Gui* obj)
{
    if (obj->hotGadgetIndex == -1) {
        if (g_game->bits_2bee.flag4)
            LeaveNetGame();
    } else if (IsCurrentGadgetNamed((Dialog *)obj, "OK")) {
        PlaySoundByName("BigButton", 0);
        SetFrontendState(2, 0x1412, "c:\\cavedog\\wargame\\multi.cpp");
        SetGameMode(1);
    } else {
        ClearSelectedGadget(obj);
    }
}

// FUNCTION: 0x44b020
void OpenEndMultiScreen()
{
    unsigned char palette[0x400];
    void* image;

    BlankScreen();
    image = LoadBitmapByName("Mission02WinBW", palette);
    SetPaletteColors(palette, 0, 0x100);
    SetOffscreenSurface((int)g_game->screen);
    DrawSurface(0, image, 0, 0);
    FreeSurface((Surface *)image);
    FlipScreen();
    LoadGuiLayer(&g_game->gui, "ENDMULTI.GUI", 0x80)->handler = HandleEndMultiClick;
    SetTranslatedTextByName(&g_game->gui, "RESULT",
                 Translate(g_game->bits_3923b.flag4_3923b ? "Victory" : "Failure"), 0);
    RenderLayer(&g_game->gui, 0xc0);
    ShowSoftwareCursor();
}

// FUNCTION: 0x44b100
void FreeSaveListBuffers()
{
    if (g_saveListFileNames) {
        GameFreeThunk(g_saveListFileNames);
    }
    if (g_saveListDisplayNames) {
        GameFreeThunk(g_saveListDisplayNames);
    }
    g_saveListFileNames = g_saveListDisplayNames = 0;
}

// FUNCTION: 0x44b140
void __stdcall LoadUnitRestrictListFile(char* name)
{
    void* f = host_file_open_read(name);
    int count;
    host_file_read(f, &count, 4);
    for (int i = 0; i < count; i++) {
        int a, b;
        host_file_read(f, &a, 4);
        host_file_read(f, &b, 4);
        int n = g_game->unitDefCount;
        for (int idx = 1; idx < n; idx++) {
            if (g_game->defs.get()[idx].fbiChecksum == a) {
                for (int j = 0; j < n; j++) {
                    if (g_unitRestrictEntries[j].unitIndex == idx) {
                        g_unitRestrictEntries[j].max = b;
                        break;
                    }
                }
                break;
            }
        }
    }
    host_file_close(f);
}

// FUNCTION: 0x44b230
void __stdcall SaveUnitRestrictListFile(char* filename)
{
    void* f = host_file_create(filename, 0);

    int count = g_game->unitDefCount - 1;
    host_file_write(f, &count, 4);
    count++;

    for (int i = 1; i < count; i++) {
        for (int j = 0; j < g_game->unitDefCount; j++) {
            if (*(int*)((char*)g_unitRestrictEntries + 0x52 + j * 0x62) == i) {
                int v = g_game->defs.get()[i].fbiChecksum;
                host_file_write(f, &v, 4);
                v = *(int*)((char*)g_unitRestrictEntries + 0x5a + j * 0x62);
                host_file_write(f, &v, 4);
                break;
            }
        }
    }

    host_file_close(f);
}

// FUNCTION: 0x44b330
void CopySelectedGameName()
{
    Gui* menu = &g_game->gui;
    void* gadgets = g_game->gui.layer->entries;
    Gadget* games = FindGadgetChecked(gadgets, "GAMES");
    int index = FindGadgetIndex(gadgets, "GAMENAME", 3);
    char* name;
    if (games->u.list.selected > -1 && (name = SkipTextLines(g_saveListDisplayNames, games->u.list.selected)) != 0 && strlen(name) != 0)
        SetGadgetText(menu, index, name);
    else
        SetGadgetText(menu, index, DAT_005119b8);
    MarkChanged((Dialog *)&g_game->gui);
}

// FUNCTION: 0x44b3c0
void __stdcall HandleLoadListClick(Gui* menu)
{
    void* gadgets = menu->layer->entries;
    if (menu->hotGadgetIndex == -1)
        return;
    if (IsCurrentGadgetNamed((Dialog *)menu, "CANCEL")) {
        PlaySoundByName("Previous", 0);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)menu, "LOAD") || IsCurrentGadgetNamed((Dialog *)menu, "GAMES")) {
        PlaySoundByName("Options", 0);
        Gadget* games = FindGadgetChecked(gadgets, "GAMES");
        sprintf(g_game->saveName_38c6b, "%s\\%s", g_savegameDir,
                SkipTextLines(g_saveListFileNames, games->u.list.selected));
        LoadUnitRestrictListFile(g_game->saveName_38c6b);
        ListDialogLayer* inner = menu->layer;
        menu->layer = inner->next;
        UpdateUnitSliders(menu, 0);
        menu->layer = inner;
        if (g_saveListFileNames)
            GameFreeThunk(g_saveListFileNames);
        if (g_saveListDisplayNames)
            GameFreeThunk(g_saveListDisplayNames);
        g_saveListDisplayNames = 0;
        g_saveListFileNames = 0;
    } else if (menu->hotGadgetIndex != -1) {
        ClearSelectedGadget(menu);
    }
}

// FUNCTION: 0x44b4e0
void* __stdcall ListSaveGameFiles(int* out)
{
    char path[0x100];
    BuildDataPath(path, g_savegameDir, g_star, g_lstExtension);
    int count = CountDirectoryEntries(path, 0);
    *out = count;
    if (count == 0) {
        ConfigureListBoxByName(&g_game->gui, g_gamesGadgetName, DAT_005119b8, 0, 0);
        return 0;
    }
    g_saveListFileNames = (char*)GameAllocIgnoreTag(g_savegameNamesName, count << 8);
    g_saveListDisplayNames = (char*)GameAllocIgnoreTag(g_savegameDescsName, *out << 8);
    memset(g_saveListDisplayNames, 0, *out << 8);
    memset(g_saveListFileNames, 0, *out << 8);
    ScanDirectory(path, g_saveListFileNames, 0, 0, 0, 1);
    ConfigureListBoxByName(&g_game->gui, g_gamesGadgetName, g_saveListFileNames, *out, 0);
    return *out ? g_saveListFileNames : 0;
}

// FUNCTION: 0x44b600
void __stdcall ShowSelectedSaveGame(Gui* unused1, Gadget* unused2)
{
    Gui* menu = &g_game->gui;
    void* gadgets = g_game->gui.layer->entries;
    Gadget* games = FindGadgetChecked(gadgets, "GAMES");
    int index = FindGadgetIndex(gadgets, "GAMENAME", 3);
    char* name;
    if (games->u.list.selected > -1 && (name = SkipTextLines(g_saveListDisplayNames, games->u.list.selected)) != 0 && strlen(name) != 0)
        SetGadgetText(menu, index, name);
    else
        SetGadgetText(menu, index, DAT_005119b8);
    MarkChanged((Dialog *)&g_game->gui);
}

// FUNCTION: 0x44b690
void __stdcall HandleSaveGameClick(Gui* menu)
{
    Gadget* entries = menu->layer->entries;
    if (menu->hotGadgetIndex == -1) {
        SetDescListCleanupFlag(menu, 1);
        if (g_saveListFileNames)
            GameFreeThunk(g_saveListFileNames);
        if (g_saveListDisplayNames)
            GameFreeThunk(g_saveListDisplayNames);
        g_saveListFileNames = g_saveListDisplayNames = 0;
        g_game->bits_38a51.bit_38a51 = 0;
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)menu, "CANCEL")) {
        PlaySoundByName("Previous", 0);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)menu, "DELETE")) {
        PlaySoundByName("SMLBUTTON", 0);
        Gadget* games = FindGadgetChecked(entries, "GAMES");
        char buf[0x100];
        sprintf(buf, "%s\\%s", g_savegameDir,
                SkipTextLines(g_saveListFileNames, games->u.list.selected));
        RemoveFile(buf);
        int count;
        ListSaveGameFiles(&count);
        char* p = g_saveListDisplayNames;
        for (int i = 0; i < count; i++) {
            strcpy(p, SkipTextLines(g_saveListFileNames, i));
            p += strlen(SkipTextLines(g_saveListFileNames, i));
            while (*p != '.')
                p--;
            *p++ = 0;
        }
        ConfigureListBoxByName(&g_game->gui, "GAMES", g_saveListDisplayNames, count, 0);
        ClearSelectedGadget(menu);
        Gui* menu2 = &g_game->gui;
        Gadget* gadgets = g_game->gui.layer->entries;
        Gadget* games2 = FindGadgetChecked(gadgets, "GAMES");
        int index = FindGadgetIndex(gadgets, "GAMENAME", 3);
        char* name;
        if (games2->u.list.selected > -1 &&
            (name = SkipTextLines(g_saveListDisplayNames, games2->u.list.selected)) != 0 &&
            strlen(name) != 0)
            SetGadgetText(menu2, index, name);
        else
            SetGadgetText(menu2, index, DAT_005119b8);
        MarkChanged((Dialog *)&g_game->gui);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)menu, "GAMES") || IsCurrentGadgetNamed((Dialog *)menu, "LOAD") ||
        IsCurrentGadgetNamed((Dialog *)menu, "GAMENAME")) {
        PlaySoundByName("Options", 0);
        int idx = FindGadgetIndex(entries, "GAMENAME", 3);
        char* name = entries[idx].u.text;
        if (strlen(name) != 0) {
            BuildDataPath(g_game->saveName_38c6b, g_savegameDir, name, "LST");
            SaveUnitRestrictListFile(g_game->saveName_38c6b);
        }
    } else if (menu->hotGadgetIndex != -1) {
        ClearSelectedGadget(menu);
    }
}

static char* GetSaveDescriptions()
{
    return g_saveListDisplayNames;
}

// FUNCTION: 0x44b990
void __stdcall OpenSaveGameDialog()
{
    int count;
    ListDialogLayer* layer = LoadGuiLayer(&g_game->gui, "SAVELIST.GUI", 0x880);
    layer->handler = HandleSaveGameClick;
    layer->data = g_game;
    LoadPictureCached("DSaveList", 0, 0, 0);
    MakeDirectoryPath(g_savegameDir);
    ListSaveGameFiles(&count);
    SetTranslatedTextByName(&g_game->gui, "TITLE", "Save Game", 0);
    char* ptr = GetSaveDescriptions();
    int i = 0;
    for (; i < count; i++) {
        strcpy(ptr, SkipTextLines(g_saveListFileNames, i));
        ptr += strlen(SkipTextLines(g_saveListFileNames, i));
        while (*ptr != '.')
            ptr--;
        *ptr = 0;
        ptr++;
    }
    ConfigureListBoxByName(&g_game->gui, "GAMES", g_saveListDisplayNames, count, 0);
    if (count == 0)
        SetGadgetActiveByName(&g_game->gui, "DELETE", 0);
    Gadget* games = FindGadgetChecked(layer->entries, "GAMES");
    if (games != 0)
        games->u.list.callback = ShowSelectedSaveGame;
    int index = FindGadgetIndex(layer->entries, "GAMENAME", 3);
    layer->entries[index].attribs |= 2;

    Gui* menu = &g_game->gui;
    Gadget* entries = g_game->gui.layer->entries;
    Gadget* games2 = FindGadgetChecked(entries, "GAMES");
    int index2 = FindGadgetIndex(entries, "GAMENAME", 3);
    char* name;
    if (games2->u.list.selected > -1 && (name = SkipTextLines(g_saveListDisplayNames, games2->u.list.selected)) != 0 && strlen(name) != 0)
        SetGadgetText(menu, index2, name);
    else
        SetGadgetText(menu, index2, DAT_005119b8);
    MarkChanged((Dialog *)&g_game->gui);

    BeginTextEdit(&g_game->gui, index);
    SetKeyboardInput(&g_game->gui, 1);
    OrLabelAttribs();
    SetGadgetActiveByName(&g_game->gui, "LoadGame", 0);
    EnableKeyCommands((Dialog *)&g_game->gui);
    RenderLayer(&g_game->gui, 0x40);
}

// FUNCTION: 0x44bc10
void OpenLoadListDialog()
{
    ListDialogLayer* gadget = LoadGuiLayer(&g_game->gui, "LOADLIST.GUI", 0x981);
    gadget->handler = HandleLoadListClick;
    gadget->data = g_game;
    LoadPictureCached("DLoadList", 0, 0, 0);
    int count;
    if (ListSaveGameFiles(&count) == 0) {
        CloseTopScreen(&g_game->gui);
        OpenMessageBox(&g_game->gui,
                     Translate("There are no saved lists to choose from"),
                     0x140, 1, 1);
        return;
    }
    char* p = g_saveListDisplayNames;
    for (int i = 0; i < count; i++) {
        strcpy(p, SkipTextLines(g_saveListFileNames, i));
        p += strlen(SkipTextLines(g_saveListFileNames, i));
        char c = *p;
        while (c != '.') {
            c = *--p;
        }
        *p = 0;
        p++;
    }
    ConfigureListBoxByName(&g_game->gui, "GAMES", g_saveListDisplayNames, count, 0);
    SetGadgetActiveByName(&g_game->gui, "DELETE", 0);
    SetGadgetActiveByName(&g_game->gui, "GAMENAME", 0);
    Gadget* entry = FindGadgetChecked(gadget->entries, "GAMES");
    if (entry != 0) {
        entry->u.list.callback = ShowSelectedSaveGame;
    }
    Gui* menu = &g_game->gui;
    void* gadgets = g_game->gui.layer->entries;
    Gadget* games = FindGadgetChecked(gadgets, "GAMES");
    int index = FindGadgetIndex(gadgets, "GAMENAME", 3);
    char* name;
    if (games->u.list.selected > -1 && (name = SkipTextLines(g_saveListDisplayNames, games->u.list.selected)) != 0 && strlen(name) != 0)
        SetGadgetText(menu, index, name);
    else
        SetGadgetText(menu, index, DAT_005119b8);
    MarkChanged((Dialog *)&g_game->gui);
    SetKeyboardInput(&g_game->gui, 1);
    OrLabelAttribs();
    SetGadgetActiveByName(&g_game->gui, "SaveGame", 0);
    EnableKeyCommands((Dialog *)&g_game->gui);
    RenderLayer(&g_game->gui, 0x40);
    g_game->bits_38a51.bit_38a51 |= 1;
}

// FUNCTION: 0x44be70
void __stdcall HandleUnitCountSlider(void* obj, char* gadget)
{
    int n = atoi(gadget + 8);
    Gadget* desc = FindGadgetChecked(g_game->gui.layer->entries, "DESCLIST");
    char count[20];
    sprintf(count, "COUNT%d", n);
    int value = ReadSliderValue((Gadget *)gadget);
    char buf[20];
    if (value > 0x64) {
        sprintf(buf, Translate("No Limit"));
        value = -1;
    } else {
        _itoa(value, buf, 10);
    }
    g_unitRestrictEntries[n + desc->u.list.firstRow].max = value;
    ((UnitSync *)g_game->sync)->SetUnitLimit(
        &g_game->defs.get()[g_unitRestrictEntries[n + desc->u.list.firstRow].unitIndex], value);
    desc->u.list.flags[n + desc->u.list.firstRow] = g_unitRestrictEntries[n + desc->u.list.firstRow].peerEnabled == 0;
    desc->u.list.flags[n + desc->u.list.firstRow] |= g_unitRestrictEntries[n + desc->u.list.firstRow].max == 0 ? 2 : 0;
    SetTranslatedTextByName(obj, count, (char*)buf, 0);
}

// FUNCTION: 0x44bfd0
void __stdcall UpdateUnitSliders(Gui* param_1, int unused)
{
    int i;
    Gadget* desc;
    int human;
    int base;
    Gadget* slider;
    int en;
    int value;
    char name[20];

    desc = FindGadgetChecked(param_1->layer->entries, "DESCLIST");
    human = IsHostLocal();
    base = desc->u.list.firstRow;

    for (i = 0; i < 12; i++) {
        sprintf(name, "SLIDER%d", i);
        slider = FindGadgetChecked_D(param_1->layer->entries, name);
        if (slider != 0) {
            if (human == 0 || g_unitRestrictEntries[base + i].peerEnabled == 0)
                en = 1;
            else
                en = 0;
            desc->u.list.flags[base + i] = en != 0;
            value = g_unitRestrictEntries[base + i].max;
            if (value == -1)
                value = slider->max;
            SetSliderFromValue(slider, value);
            SetGadgetGrayedOutByName(param_1, name, en);
            slider->sliderCallback(param_1, (int)slider->sliderUser);
        }
    }
}

// FUNCTION: 0x44c0d0
void LoadUnitPortrait()
{
    UnitRestrictRecord rec;
    char path[256];
    Gadget* pic = FindGadgetChecked(g_game->gui.layer->entries, "PICLIST");
    if (g_unitRestrictPicLoadIndex == 0) {
        g_unitRestrictRecordCursor = (int)pic->u.list.rows;
        g_unitRestrictPicCursor = (int)g_unitRestrictPics;
    }
    int i = g_unitRestrictPicLoadIndex++;
    if (i < g_game->unitDefCount) {
        int type = g_unitRestrictEntries[i].unitIndex;
        UnitDef* defs = g_game->defs.get();
        if (defs[type].unitname && ((unsigned char)(defs[type].flags2 >> 15) & 1) == 0) {
            BuildDataPath(path, "unitpics", defs[g_unitRestrictEntries[i].unitIndex].unitname, "PCX");
            void* img = LoadPcx(path, 0);
            *(void**)g_unitRestrictPicCursor = img;
            g_unitRestrictPicCursor += 4;
            if (img != 0) {
                FrameFromSurface(&rec, img);
                rec.flag8 = 9;
            } else {
                rec.a = pic->width;
                rec.b = 0x20;
                rec.d = 0;
            }
            *(UnitRestrictRecord*)g_unitRestrictRecordCursor = rec;
            g_unitRestrictRecordCursor += 0x18;
            MarkChanged((Dialog *)&g_game->gui);
        }
    }
}

// FUNCTION: 0x44c220
void UnitRestrictDialogFrame()
{
    UnitSyncEntry event;
    int n = 0;
    Gadget* entry = (Gadget*)FindGadgetChecked(g_game->gui.layer->entries, "PICLIST");

    if (g_unitRestrictNextPicTick < (int)GetTicks()) {
        g_unitRestrictNextPicTick = GetTicks() + 2;
        LoadUnitPortrait();
    }

    while (((UnitSync *)g_game->sync)->PopChangedEntry(&event) != 0) {
        n++;
        for (int i = 0; i < entry->u.list.rowCount; i++) {
            if (event.fbiChecksum == g_game->defs.get()[g_unitRestrictEntries[i].unitIndex].fbiChecksum) {
                entry->u.list.flags[i] = (event.peerEnabled == 0);
                g_unitRestrictEntries[i].peerEnabled = event.peerEnabled;
                g_unitRestrictEntries[i].max = event.limit;
                entry->u.list.flags[i] |= (event.limit != 0) ? 0 : 2;
            }
        }
    }

    if (n != 0) {
        UpdateUnitSliders(&g_game->gui, 0);
        MarkChanged((Dialog *)&g_game->gui);
    }
}

// FUNCTION: 0x44c370
void __stdcall ShowSelectedUnitCosts(Gui* panel, Gadget* unit)
{
    char buf[20];
    UnitDef* def = &g_game->defs.get()[unit->u.list.restrictions[unit->u.list.selected].unitIndex];
    sprintf(buf, "%d", (int)def->energyCost);
    SetTranslatedTextByName(panel, "ENERGYTEXT", (char*)buf, 0);
    sprintf(buf, "%d", (int)def->metalCost);
    SetTranslatedTextByName(panel, "METALTEXT", (char*)buf, 0);
}

// FUNCTION: 0x44c7a0
int __cdecl CompareUnitRestrictEntries(const char* a, const char* b)
{
    return strcmp(a, b);
}

void __stdcall BindUnitRestrictSlider(char* name, int max, int value, UnitRestrictSliderCallback callback)
{
    Gui* gui = &g_game->gui;
    Gadget* gadgets = gui->layer->entries;
    int index = FindGadgetIndex(gadgets, name, 0xe);
    if (index != -1) {
        Gadget* gadget = FindGadgetChecked_D(gadgets, name);
        gadget->max = max;
        gadget->sliderCallback = callback;
        gadget->knobPos = value;
        SetSliderFromValue(gadget, gadget->knobPos);
        gadget->sliderUser = (int)g_game;
    }
    callback(gui, index);
    MarkChanged((Dialog *)gui);
}

// FUNCTION: 0x44c7e0
void OpenUnitRestrictions()
{
    int host = g_game->players_1b63[g_game->localPlayer].info->host & 1;
    ListDialogLayer* layer;
    Gadget* entries;
    char* flags;
    Gadget* desc;
    Gadget* pic;
    int* pics;
    char* text;
    char* dst;
    int n;
    int i;

    layer = LoadGuiLayer(&g_game->gui, "RESTRICT2.GUI", 0x880);
    layer->handler = HandleRestrictionsClick;
    layer->data = nullptr;
    layer->cb1c = UnitRestrictDialogFrame;
    g_unitRestrictPicLoadIndex = 0;
    LoadPictureCached("UnitRestrict5x", 0, 0, 0);

    entries = layer->entries;
    flags = (char*)GameAllocIgnoreTag("FLAGS", g_game->unitDefCount);
    desc = FindGadgetChecked(entries, "DESCLIST");
    desc->u.list.callback = ShowSelectedUnitCosts;
    desc->u.list.flags = flags;
    desc->u.list.scroll = 0x20;
    desc->attribs |= 0x100;

    pic = FindGadgetChecked(layer->entries, "PICLIST");
    pic->u.list.flags = flags;
    pic->attribs |= 0x180;
    pic->u.list.scroll = desc->u.list.scroll;

    pics = (int*)GameAllocIgnoreTag("UNITPICARRAY", g_game->unitDefCount * 0x18);
    memset(pics, 0, g_game->unitDefCount * 0x18);
    text = (char*)GameAllocIgnoreTag("UNITTEXTARRAY", g_game->unitDefCount << 5);
    *(int*)text = 0;

    g_unitRestrictEntries = (UnitRestrictEntry*)GameAllocIgnoreTag("UNITSRESTRICTINFO", g_game->unitDefCount * 0x62);
    desc->u.list.records = g_unitRestrictEntries;
    for (i = 0; i < g_game->unitDefCount; i++)
        g_unitRestrictEntries[i].unitIndex = 0;

    g_unitRestrictPics = (int*)GameAllocIgnoreTag("UNITSPICS", g_game->unitDefCount << 2);
    memset(g_unitRestrictPics, 0, g_game->unitDefCount << 2);
    memset(g_unitRestrictEntries, 0, g_game->unitDefCount * 0x62);
    g_unitRestrictOldCounts = (int*)GameAllocIgnoreTag("OLDCOUNTS", g_game->unitDefCount << 2);

    n = 0;
    for (i = 1; i < g_game->unitDefCount; i++) {
        if (g_game->defs.get()[i].norestrict)
            continue;
        if (!g_game->defs.get()[i].unitname)
            continue;
        {
            UnitDef* type = &g_game->defs.get()[i];
            UnitSyncEntry info;
            int count;
            sprintf(g_unitRestrictEntries[n].name, "%s\r%s %dM  %dE",
                    g_game->defs.get()[i].name, Translate(type->side),
                    (int)type->metalCost, (int)type->energyCost);
            g_unitRestrictEntries[n].unitIndex = i;
            ((UnitSync *)g_game->sync)->GetUnitEntry(&g_game->defs.get()[i], &info);
            count = info.limit == -1 ? 0x65 : info.limit;
            g_unitRestrictEntries[n].max = count;
            g_unitRestrictOldCounts[n] = count;
            g_unitRestrictEntries[n].peerEnabled = info.peerEnabled;
            n++;
        }
    }

    qsort(g_unitRestrictEntries, n, 0x62, (int (__cdecl*)(const void*, const void*))CompareUnitRestrictEntries);

    dst = text;
    for (i = 0; i < g_game->unitDefCount; i++) {
        strcpy(dst, g_unitRestrictEntries[i].name);
        dst += strlen(g_unitRestrictEntries[i].name) + 1;
    }

    for (i = 0; i < 0xc; i++) {
        char name[0x14];
        Gadget* slider;
        sprintf(name, "SLIDER%d", i);
        slider = FindGadgetChecked_D(entries, name);
        slider->sliderUser = (int)slider;
        slider->max = 0x65;
        slider->sliderCallback = (void (__stdcall*)(Gui*, int))HandleUnitCountSlider;
    }

    BindUnitRestrictSlider("SCROLLSLIDER", 0xd2, 0, UpdateUnitSliders);

    ConfigureListBoxByName(&g_game->gui, "DESCLIST", text, n, 0);
    SetGadgetRows(g_game->gui.layer, "PICLIST", pics, n);
    UpdateUnitSliders(&g_game->gui, 0);

    {
        Gui* gui = &g_game->gui;
        UnitDef* type = &g_game->defs.get()[desc->u.list.restrictions[desc->u.list.selected].unitIndex];
        char buf[0x14];
        sprintf(buf, "%d", (int)type->energyCost);
        SetTranslatedTextByName(gui, "ENERGYTEXT", buf, 0);
        sprintf(buf, "%d", (int)type->metalCost);
        SetTranslatedTextByName(gui, "METALTEXT", buf, 0);
    }

    {
        int enabled = host == 0;
        SetGrayedOutByName(&g_game->gui, "Load", enabled);
        SetGrayedOutByName(&g_game->gui, "Save", enabled);
        SetGrayedOutByName(&g_game->gui, "Reset", enabled);
    }
    RenderLayer(&g_game->gui, 0x40);
    SetKeyboardInput(&g_game->gui, 1);
    DAT_005129c0 = 0;
}


// FUNCTION: 0x443ff0
int __stdcall SelectConnection(int index)
{
    if (index < 0) {
        GUID* guid = nullptr;
        switch (DAT_00512c80) {
        case 0: break;
        case 1: guid = &g_dpspGuidTcpip; break;
        case 2: guid = &g_dpspGuidIpx; break;
        case 3: guid = &g_dpspGuidModem; break;
        case 4: guid = &g_dpspGuidSerial; break;
        }
        DAT_00512c80 = 0;
        if (guid != nullptr) {
            int i = 0;
            for (; i < g_game->count; i++) {
                if (memcmp(&((GUID *)g_game->guids)[i], guid, sizeof(GUID)) == 0)
                    break;
            }
            if (i < g_game->count)
                index = i;
        }
    }
    if (index >= 0) {
        (*(GUID*)&g_game->info.guid) = ((GUID *)g_game->sessions)[index];
        g_game->info.conn = g_game->conns[index];
        int size = g_game->conns[index].size;
        g_game->info.conn.data = GameAllocIgnoreTag("DPLAY CONNECTION INFO", size);
        if (g_game->info.conn.data != nullptr) {
            memcpy(g_game->info.conn.data, g_game->conns[index].data, size);
        } else {
            g_game->info.conn.size = 0;
        }
        if (IsOnlineConfigLoaded() && g_onlineLobbyPassword != 0) {
            lstrcpynA(g_game->name_2be3, &g_onlineLobbyPassword, 0xb);
        }
        return 1;
    }
    return 0;
}

// FUNCTION: 0x4441a0
void __stdcall HandleSelectProviderClick(Gui* menu)
{
    Gadget* entries = menu->layer->entries;
    int i;
    int id;
    int cur;
    if (menu->hotGadgetIndex == -1) {
        if (g_game->sessions != nullptr) {
            GameFreeThunk(g_game->sessions);
            for (i = 0; i < 10; i++) {
                if (g_game->conns[i].data != nullptr) {
                    host_net_free(g_game->conns[i].data);
                    g_game->conns[i].data = nullptr;
                }
            }
            GameFreeThunk(g_game->conns);
            GameFreeThunk(g_game->data[0]);
            g_game->sessions = nullptr;
            g_game->data[0] = nullptr;
        }
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)menu, "DPLAY") || IsCurrentGadgetNamed((Dialog *)menu, "SELECT")) {
        SelectConnection(FindGadgetChecked(entries, "DPLAY")->u.list.selected);
        g_game->frontendNextSubState = 2;
        PlaySoundByName("BigButton", 0);
        return;
    }
    if (_strnicmp(entries[menu->hotGadgetIndex].name, "SERVICE", 7) == 0) {
        PlaySoundByName("BigButton", 0);
        int link = atoi(entries[menu->hotGadgetIndex].name + 7);
        char message[0x140];
        message[0] = 0;
        int result = 2;
        try {
            SetCursorMode(0x14);
            result = OnlineProcessButtonCommand(g_linkInfo[link].id, message, sizeof(message));
            SetCursorMode(0x13);
        } catch (...) {
            SetCursorMode(0x13);
        }
        if (result == 0) {
            OnlineUnload();
            exit(0);
            return;
        }
        if (message[0] == 0)
            sprintf(message, "The %s service failed or took%stoo long. Please see the readme%sfile for more information.",
                    g_linkInfo[link].name, "\n\n", "\n\n");
        char* msg = Translate(message);
        OpenMessageBox((Gui *)menu, msg, sizeof(message), 1, 1);
        ClearSelectedGadget(menu);
        return;
    }
    if (FindGadgetIndex(entries, "PREVMENU", 0xe) == menu->hotGadgetIndex) {
        g_game->frontendNextSubState = 3;
        PlaySoundByName("Previous", 0);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)menu, "SETTINGS")) {
        PlaySoundByName("Options", 0);
        ClearSelectedGadget(menu);
        OpenOptionsPanel();
        return;
    }
    ClearSelectedGadget(menu);
}

// FUNCTION: 0x444580
void FillProviderList()
{
    if (g_game->gui.layer != nullptr && strcmp(g_game->gui.layer->entries->name, "SELPROV.GUI") == 0)
        CloseTopScreen(&g_game->gui);
    SetPaletteColors((unsigned char*)g_game->paletteRgba, 0, 0x100);
    RemapPaletteToClosestIndices(&g_game->gui, g_game->paletteRgba, g_game->gui.palette);
    Layer* menu = LoadGuiLayer(&g_game->gui, "SELPROV.GUI", 0x80);
    menu->handler = HandleSelectProviderClick;
    menu->data = g_game;
    LoadPictureCached("selconnect2", 1, 0, 0);
    g_game->data[0] = GameAllocIgnoreTag("PROVIDER DESCRIPTIONS", 0x500);
    g_game->sessions = (V4i*)GameAllocIgnoreTag("PROVIDER GUIDS", 0xa0);
    g_game->conns = (ConnectionBlob*)GameAllocIgnoreTag("DPLAY CONNECTIONS", 0x50);
    if (g_game->conns != nullptr)
        memset(g_game->conns, 0, 0x50);
    g_game->net->RefreshMapList(1);
    host_net_uninit((host_net*)g_game->net_14);
    host_net_get_connections((host_net*)g_game->net_14, g_game->sessions, g_game->conns, g_game->data[0], (const host_guid*)DAT_004fcfb8);
    Gadget* entries = menu->entries;
    int tmpl = FindGadgetIndex(entries, "SERVICEX", 1);
    int y;
    int k;
    int n = 0;
    int count = 0;
    try {
        count = OnlineGetLinkInfo(g_linkInfo);
    } catch (...) {
    }
    if (count != 0 && tmpl != -1) {
        y = entries[tmpl].y;
        k = 0;
        for (n = 0; n < count; k++) {
            if (g_linkInfo[k].id != -1) {
                n++;
                char name[0x1c];
                sprintf(name, "SERVICE%d", k);
                CloneServiceSlot(entries, tmpl, y, (int)g_linkInfo[k].name, name);
                y += entries[tmpl].height + 1;
            }
        }
    }
    strcpy(entries->u.names.choice, "SELECT");
    ConfigureListBoxByName(&g_game->gui, "DPLAY", (char*)g_game->data[0], g_game->count, 0);
    OrLabelAttribs();
    SetKeyboardInput(&g_game->gui, 1);
    RenderLayer(&g_game->gui, 0x40);
}

// FUNCTION: 0x449bb0
void OpenBattleRoom()
{
    int energy = 1000;
    int metal = 1000;
    Layer* layer;
    Player* player;
    PlayerInfo* info;
    Gadget* entries;
    short host;
    short isHost;
    int i;

    g_battleRoomSlotsBuilt = 0;
    g_lastPlayerCount = -1;
    g_game->bits_2bee_1.dirty = 1;
    memset(g_game->playerIds, 0, sizeof(g_game->playerIds));
    info = g_game->players[g_game->localPlayer].info.get();
    player = &g_game->players[g_game->localPlayer];
    host = info->host;
    g_game->lobbyInitScratch = 0;
    if (host)
        g_game->maxUnits = g_game->maxUnits_37eec;
    info->width = g_game->width_37f1b_2;
    info->height = g_game->height_37f1f_2;
    info->f9d_2 = 1;

    layer = LoadGuiLayer(&g_game->gui, "LOUNGE2.GUI", 0);
    layer->handler = HandleBattleRoomClick;
    layer->data = g_game;
    LoadPictureCached("battleroom", 0, 1, 0);
    entries = g_game->gui.layer->entries;
    g_battleRoomBaseGadgetCount = layer->entries->u.count;

    i = FindGadgetIndex(entries, "MESSAGE", 3);
    if (i != -1)
        entries[i].status = 0x7f;
    if (!IsHostLocal()) {
        i = FindGadgetIndex(entries, "MAP", 1);
        if (i != -1) {
            entries[i].attribs = 2;
            SetTranslatedTextByName(&g_game->gui, "MAP", "View Map", 0);
        }
    }

    if (IsOnlineConfigLoaded()) {
        if (g_lobbyMaxUnits) {
            info->maxUnits = g_lobbyMaxUnits;
            g_game->maxUnits = g_lobbyMaxUnits;
        }
        g_game->bits_2c74.locked = g_lobbyLock != 0;
        if (g_lobbyCommander) {
            int commander = g_lobbyCommander - 1;
            if (commander >= 0 && commander <= 2) {
                g_game->multiCommanderDeath = commander;
                g_game->commanderDeath = commander;
                Player* p = &g_game->players[g_game->localPlayer];
                p->info.get()->commander = commander;
            }
        }
        if (g_lobbyEnergy)
            energy = g_lobbyEnergy;
        if (g_lobbyMetal)
            metal = g_lobbyMetal;
        if (g_lobbyLineOfSight) {
            switch (g_lobbyLineOfSight) {
            case 1:
                info->los = 1;
                info->losType = 1;
                break;
            case 2:
                info->los = 1;
                info->losType = 0;
                break;
            case 3:
                info->los = 0;
                break;
            }
        }
        if (g_lobbyCheating)
            info->cheating = g_lobbyCheating == 2;
        if (g_lobbyFixedLoc)
            info->fixedloc = g_lobbyFixedLoc == 1;
        if (g_lobbyMapping)
            info->mapping = g_lobbyMapping == 1;
        if (g_lobbyWatching)
            info->watching = g_lobbyWatching == 2;
    } else if (host) {
        info->mapping = g_game->multiMapping;
        info->los = g_game->multiLineOfSight;
        info->losType = g_game->multiLOSType;
        info->fixedloc = g_game->options->fixedLocations;
        info->commander = g_game->multiCommanderDeath;
    }

    if (!host || g_game->bits_2c74.locked) {
        for (char** p = g_hostOnlyGadgets; *p; p++)
            SetGrayedOutByName(&g_game->gui, *p, 1);
    }

    UpdateBattleRoomFlags();
    g_game->list = GameAllocIgnoreTag("LOUNGE CHATTER", 0xa00);
    *(char*)g_game->list = 0;
    {
        Gadget* mem = FindGadgetChecked_C(g_game->gui.layer->entries, "MEMx");
        mem->colour = player->info.get()->memory < g_game->net->GetTerrainSizeTier() ? 0xc : 0;
        sprintf(mem->u.text, "%d", g_game->players[g_game->localPlayer].info.get()->memory);
    }
    isHost = g_game->players[g_game->localPlayer].info.get()->host;
    CreateUnitSync(isHost);
    SetGadgetActiveByName(&g_game->gui, "START", ((UnitSync *)g_game->sync)->AllPlayersSynced());
    SetGrayedOutByName(&g_game->gui, "START",
                 host && AreAllPlayersReady() && ((UnitSync *)g_game->sync)->AllPlayersSynced() ? 0 : 1);
    SetGrayedOutByName(&g_game->gui, "RESTRICTIONS", 0);
    ConfigureListBoxByName(&g_game->gui, "OUTPUT", (char*)g_game->list, 0, 0);
    {
        Gadget* output = FindGadgetChecked(entries, "OUTPUT");
        output->attribs |= 0x100;
    }
    SetTranslatedTextByName(&g_game->gui, "METALTEXT", "0", 0);
    SetTranslatedTextByName(&g_game->gui, "ENERGYTEXT", "0", 0);

    BindNamedSliderWithCallback("METAL", 0x2711, metal, UpdateMetalText);
    {
        BindNamedSliderWithCallback("MAXUNITS", g_game->maxUnits - 20, g_game->maxUnits - 20, UpdateMaxUnitsText);
    }
    if (!isHost || g_game->bits_2c74.locked) {
        SetGadgetGrayedOutByName(&g_game->gui, "MAXUNITS", 1);
        SetGadgetGrayedOutByName(&g_game->gui, "ENERGY", 1);
        SetGadgetGrayedOutByName(&g_game->gui, "METAL", 1);
    }
    BindNamedSliderWithCallback("ENERGY", 0x2711, energy, UpdateEnergyText);

    g_game->net->RefreshMapList(1);
    if (isHost && IsOnlineConfigLoaded() && g_lobbyMapName[0])
        g_game->net->LoadMissionByName(g_lobbyMapName);
    if (!g_game->net->HasMissionName())
        FatalError("Could not find the multiplayer map!!");
    strcpy(info->map, g_game->net->GetMissionName());
    info->mapCrc = g_game->net->ComputeMapChecksum();
    BroadcastPlayerInfo();
    BeginTextEdit((Gui *)&g_game->gui, FindGadgetIndex(g_game->gui.layer->entries, "MESSAGE", 0xe));

    for (char** p = g_battleRoomGadgetNames; *p; p++) {
        int k = FindGadgetIndex(layer->entries, *p, 0xe);
        if (k != -1)
            layer->entries[k].active = 0;
    }

    BuildPlayerSlotGadgets();
    RefreshTeamIcons();
    RefreshBattleRoomRows();
    OrLabelAttribs();

    if (IsScreenNamed((Gui *)&g_game->gui, "LOUNGE2.GUI")) {
        Gadget* start = FindGadgetChecked_E(layer->entries, "battlestart");
        start->u.hotspot.gaf = (GafEntry*)FindGafEntry(layer->entries->u.assets.archive, "battlestart");
        start->u.hotspot.index = 0;
        start->u.hotspot.clickable = 1;
        SetGadgetActiveByName(&g_game->gui, "battlestart", 1);
    }

    SetKeyboardInput(&g_game->gui, 1);
    RenderLayer((Gui *)&g_game->gui, 0x40);
}

// FUNCTION: 0x44a680
void UpdateBattleRoom()
{
    Player* pl;
    Gadget* entries;

    g_game->ticks++;

    if (HandleNetPackets())
        g_game->bits_2bee_1.dirty = 1;
    else if (!LobbyMapCrcMatches())
        g_game->bits_2bee_1.dirty = 1;

    pl = &g_game->players[g_game->localPlayer];
    if (pl->rejectReason != 0) {
        g_game->frontendNextSubState = 3;
        CloseTopScreen(&g_game->gui);
        return;
    }

    entries = g_game->gui.layer->entries;
    if (IsScreenNamed(&g_game->gui, "LOUNGE2.GUI") != 0) {
        int idx = FindGadgetIndex(entries, "PLAYER0", 0xe);
        entries[idx].colours = 0x18;
    }

    if (g_game->bits_2bee_1.dirty) {
        if (g_battleRoomSlotsBuilt == 0) {
            BuildPlayerSlotGadgets();
        } else {
            CompactActivePlayerSlots();
        }

        if ((unsigned int)g_game->numPlayers != g_lastPlayerCount) {
            g_lastPlayerCount = g_game->numPlayers;
            UpdateNetGameInfo();
        }

        if ((pl->info->flags_97 & 1) == 0) {
            unsigned char host = FindHostSlot();
            if (host != 10) {
                if (IsScreenNamed(&g_game->gui, "LOUNGE2.GUI") != 0) {
                    PlayerInfo* info = g_game->players[host].info;
                    g_seams.map->LoadMissionByName(g_game->net, 0, (char*)info);
                    SetNamedSliderValue(&g_game->gui, "MAXUNITS", g_game->players[host].info->maxUnits - 0x14);
                    SetNamedSliderValue(&g_game->gui, "METAL", g_game->players[host].info->metal * 100);
                    SetNamedSliderValue(&g_game->gui, "ENERGY", g_game->players[host].info->energy * 100);
                    UpdateMaxUnitsText(&g_game->gui, 0);
                    UpdateEnergyText(&g_game->gui, 0);
                    UpdateMetalText(&g_game->gui, 0);
                } else if (IsScreenNamed(&g_game->gui, "viewmap.gui") != 0) {
                    PlayerInfo* info = g_game->players[host].info;
                    if (strcmp(g_seams.map->GetMissionName(g_game->net), info->map) != 0) {
                        g_seams.map->LoadMissionByName(g_game->net, 0, (char*)g_game->players[host].info.get());
                        ShowSelectedMapInfo();
                        MarkLayerChanged((Dialog *)&g_game->gui);
                    }
                }
            }
        }
        g_game->bits_2bee_1.dirty = 0;
        if (IsScreenNamed(&g_game->gui, "LOUNGE2.GUI") != 0) {
            if (pl->info->flags_97 & 1) {
                int synched = ((UnitSync *)g_game->sync)->AllPlayersSynced();
                int ready = AreAllPlayersReady();
                Gadget* start;

                SetGrayedOutByName(&g_game->gui, "SYNCHING", 1);
                start = FindGadgetChecked_E(g_game->gui.layer->entries, "battlestart");
                if (start->u.hotspot.index > 0 && g_startCountdownNextTick < (int)GetTicks()) {
                    if (start->u.hotspot.index < 8) {
                        start->u.hotspot.index++;
                        MarkChanged((Dialog *)&g_game->gui);
                        g_game->bits_2bee_1.dirty = 1;
                    }
                    g_startCountdownNextTick += 4;
                    if (start->u.hotspot.index == 4)
                        PlaySoundByName("Panel", 0);
                }
                if (start->u.hotspot.index != 0 && start->u.hotspot.index < *(unsigned short*)start->u.hotspot.gaf - 1)
                    g_game->bits_2bee_1.dirty = 1;
                if (ready != 0) {
                    g_game->bits_2bee_1.dirty = 1;
                    if (start->u.hotspot.index == 0) {
                        start->u.hotspot.index = 1;
                        g_startCountdownNextTick = GetTicks();
                        PlaySoundByName("Options", 0);
                    }
                    start->u.hotspot.clickable = 0;
                    {
                        int idx = FindGadgetIndex(entries, "START", 1);
                        unsigned int colour = GetTicks() & 0x1f;
                        if (colour != (unsigned int)entries[idx].colours) {
                            entries[idx].colours = colour;
                            MarkChanged((Dialog *)&g_game->gui);
                        }
                    }
                }
                SetGrayedOutByName(&g_game->gui, "START", ready == 0);
                SetGadgetActiveByName(&g_game->gui, "START", synched);
                SetGadgetActiveByName(&g_game->gui, "SYNCHING", synched == 0);
            }
            RefreshBattleRoomRows();
            MarkChanged((Dialog *)&g_game->gui);
        }
    }

    if (IsScreenNamed(&g_game->gui, "LOUNGE2.GUI") != 0) {
        SetCurrentFont(&g_game->gui, 1);
        {
            for (unsigned char i = 0; i < 10; i++) {
                Player* p = &g_game->players[i];
                if (p->type != 0 && p->type != 4) {
                    RECT rect;
                    char buf[20];
                    int widget;
                    PlayerInfo* info;
                    int w;
                    int h;
                    sprintf(buf, "LOGO%i", i);
                    widget = FindGadgetIndex(entries, buf, 0xe);
                    GetGadgetRectByIndex((char *)entries, widget, (Rect *)&rect);
                    info = p->info;
                    sprintf(buf, "%i.%i", info->versionMajor, info->versionMinor);
                    w = GetTextPixelWidth(buf);
                    h = GetFontLineHeight();
                    DrawTextClipped(0, buf,
                                 (rect.left + rect.right - w) / 2,
                                 (rect.top + rect.bottom - h) / 2,
                                 w, 0);
                }
            }
        }
        SetCurrentFont(&g_game->gui, 0);
    }

    ((UnitSync *)g_game->sync)->ProcessSync();
    if (g_heartbeatNextTick < (unsigned int)GetTicks()) {
        PlayerInfo* info;
        g_heartbeatNextTick = GetTicks() + 0x3c;
        info = pl->info;
        info->f9d_2 = 1;
        SendNetHeartbeat();
    }
}

// FUNCTION: 0x44c420
void __stdcall HandleRestrictionsClick(Gui* menu)
{
    int i;
    int n;
    int type;
    int* p;
    UnitDef* item;
    Gadget* desc;
    Gadget* pic;

    desc = FindGadgetChecked(menu->layer->entries, "DESCLIST");
    if (menu->hotGadgetIndex == -1) {
        i = 0;
        p = g_unitRestrictPics;
        if ((short)desc->u.list.rowCount > 0) {
            do {
                if (*p != 0)
                    FreeSurface((Surface *)((UnitRestrictPicView*)*p));
                p++;
                i++;
            } while (i < (short)desc->u.list.rowCount);
        }
        GameFreeThunk(desc->u.list.rows);
        GameFreeThunk(g_unitRestrictPics);
        g_unitRestrictPics = nullptr;
        GameFreeThunk(desc->u.list.lines);
        SetDescListCleanupFlag(menu, 1);
        if (IsHostLocal() != 0) {
            for (i = 0; i < g_game->unitDefCount; i++) {
                type = g_unitRestrictEntries[i].unitIndex;
                if (type != 0) {
                    item = &g_game->defs.get()[type];
                    if (item->norestrict) {
                    } else {
                        if (g_unitRestrictEntries[i].max == 0)
                            ((UnitSync *)g_game->sync)->DisallowUnit((UnitDef *)item);
                        else
                            ((UnitSync *)g_game->sync)->AllowUnit((UnitDef *)item);
                    }
                }
            }
        }
        GameFreeThunk(g_unitRestrictEntries);
        GameFreeThunk(g_unitRestrictOldCounts);
        GameFreeThunk(desc->u.list.flags);
        pic = FindGadgetChecked(menu->layer->entries, "PICLIST");
        if (pic != nullptr && pic->u.list.rows != nullptr)
            GameFreeThunk(pic->u.list.rows);
        g_unitRestrictEntries = nullptr;
        return;
    }

    if (IsCurrentGadgetNamed((Dialog *)menu, "Load") != 0) {
        PlaySoundByName("Options", 0);
        OpenLoadListDialog();
        ClearSelectedGadget(menu);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)menu, "Save") != 0) {
        PlaySoundByName("Options", 0);
        OpenSaveGameDialog();
        ClearSelectedGadget(menu);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)menu, "Reset") != 0) {
        PlaySoundByName("Options", 0);
        for (i = 0; i < g_game->unitDefCount; i++) {
            type = g_unitRestrictEntries[i].unitIndex;
            if (type != 0) {
                if ((g_game->defs.get()[type].flags2 & 0x10000) == 0)
                    g_unitRestrictEntries[i].max = 100;
                else
                    g_unitRestrictEntries[i].max = 0;
                if (g_unitRestrictEntries[i].max != g_unitRestrictEntries[i].previousMax) {
                    ((UnitSync *)g_game->sync)->SetUnitLimit(
                        &g_game->defs.get()[g_unitRestrictEntries[i].unitIndex],
                        g_unitRestrictEntries[i].max);
                }
            }
        }
        UpdateUnitSliders((Gui *)menu, 0);
        MarkChanged((Dialog *)menu);
        ClearSelectedGadget(menu);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)menu, "OK") != 0) {
        PlaySoundByName("Options", 0);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)menu, "Cancel") != 0) {
        PlaySoundByName("Previous", 0);
        n = 0;
        for (i = 1; i < g_game->unitDefCount; i++) {
            if (g_game->defs.get()[i].norestrict) {
            } else if (g_game->defs.get()[i].unitname != nullptr) {
                ((UnitSync *)g_game->sync)->SetUnitLimit(
                    &g_game->defs.get()[i], g_unitRestrictOldCounts[n]);
                n++;
            }
        }
        return;
    }
    if (menu->hotGadgetIndex != -1)
        ClearSelectedGadget(menu);
}
