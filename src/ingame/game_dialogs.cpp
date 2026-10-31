#include <stdio.h>
#include "../units/unit_def.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "win_types.h"
#include <math.h>
#include <vector>

#include "../map/mission.h"

#include "../sound/sound.h"

#include "../../interfaces/seams.h"

#include "../graphics/display.h"

#include "../network/packet_manager.h"
#include "../util/tdf.h"

struct Game;

#pragma pack(push, 1)

#include "../graphics/rect.h"

#include "../util/point.h"

struct Quad {
    Point p[4];
};

struct Gui;

#include "../gui/gadget.h"

#include "../gui/layer.h"

#include "../gui/gui.h"

#include "../network/player.h"
#include "../network/player_info.h"

#include "../units/unit.h"

struct Saved {
    int a;
    int b;
    short c;
};

struct Flags16 {
    unsigned short bits0_7 : 8;
    unsigned short bit8 : 1;
    unsigned short bits9_15 : 7;
};

#include "../game/game.h"
#include "../state/sim_globals.h"
#pragma pack(pop)

static inline void AccumulateProfileTime(FrameTimers* timers, int i)
{
    unsigned int t = GetMilliseconds();
    int d = t - timers->last;
    int v = timers->acc[i];
    v = v + d;
    timers->acc[i] = v;
    timers->last = t;
}

extern int g_shareDialogPlayerNetIds[10];
extern int g_nonCampaignGame;
extern char g_chatDraftText[];
extern char g_livePlayerPrefix[];
extern char g_smallButtonSoundName[];
extern char g_sendTypeGadgetName[];
extern char g_sendToGadgetName[];
extern char g_talkGadgetName[];
extern char g_chatTargetSeparators[];
extern char g_enemiesChatTarget[];
extern char g_alliesChatTarget[];
extern int g_chatDraftInitialized;
extern unsigned char g_scorePanelKillFlash[10];
extern unsigned char g_scorePanelLossFlash[10];
extern int g_scorePanelSlidePos;
extern int g_scorePanelFlashDecayTick;
extern unsigned int g_cdActivityLastSampleTick;
extern int g_cdActivityStableTicks;
extern int g_cdActivityRingWriteIdx;
extern int g_cdActivitySampleRing[];
extern int g_lastCdActivityMode;
extern int g_usePacketManager;
extern char g_yesNoGuiName[];
extern char g_choice1GadgetName[];
extern char g_choice2GadgetName[];
extern char g_titleGadgetName[];
extern char g_continueWatchingTitle[];
extern char g_yesText[];
extern char g_noText[];

Gadget* __stdcall FindGadgetChecked_D(Gadget* entries, char* name);
#include "../frontend/options.h"
void __stdcall PlaySoundByName(char* name, int flag);
Gadget* __stdcall FindGadgetChecked(Gadget* entries, char* name);
#include "../graphics/object_pool.h"
#include "../gui/dialogs.h"
void __stdcall TransferEnergy(unsigned char from, unsigned char to, float amount, int flag);
void __stdcall TransferMetal(unsigned char from, unsigned char to, float amount, int flag);
#include "../network/net_game.h"
#include "../units/units.h"
void __stdcall CollectSelectedUnits(std::vector<Unit*>* list);
#include "../units/unit_commands.h"
#include "game_dialogs.h"
int __stdcall FindGadgetIndex(Gadget* entries, char* name, int type);
#include "../debug/debug_lib.h"
void __stdcall SetButtonStageByName(Gui* menu, char* name, int value);
int __stdcall GetButtonStage(Gui* menu, int index);
Gadget* __stdcall FindGadgetChecked_B(Gadget* entries, char* name);
int __stdcall ExecuteCommandLine(char* cmd, int flags);
void __stdcall SendChatMessage(Player* from, char* text, int param_3, char* to);
#include "../frontend/multi.h"
Gadget* __stdcall FindGadgetChecked_E(Gadget* entries, char* name);
#include "../graphics/surface.h"
void __stdcall DrawSurface(void* dest, void* image, int x, int y);
#include "../game/data_files.h"
void* __stdcall LoadPcx(char* path, int param);
#include "../units/unit_targets.h"
void __stdcall AddTextGadget(Layer* obj, char* name, char* text, int x, short y, int w, int flags);
#include "../game/players.h"
#include "../frontend/options.h"
unsigned int GetTicks();
#include "../gui/keyboard.h"
#include "../graphics/draw.h"
int __stdcall GetTextPixelWidth(char* text);
void* __stdcall GetGafFrame(void* glyphs, int c);
void __stdcall DrawFrameQuad(void* surface, void* pic, Quad* dst, Quad* src);
#include "../network/unit_sync.h"
void HideSoftwareCursor();
void ShowSoftwareCursor();
#include "../network/net_stats.h"
#include "control_panel.h"
#include "../graphics/particles.h"
#include "../frontend/frontend.h"
#include "../network/net_chat.h"

// FUNCTION: 0x493340
void __stdcall UpdateMetalReadout(Gui* obj, int unused)
{
    char buf[52];
    Gadget* value = FindGadgetChecked_D(obj->layer->entries, "METAL");
    if (value != 0) {
        sprintf(buf, "%d", ReadSliderValue(value));
        SetTranslatedTextByName(obj, "METAL#", buf, 0);
    }
}

// FUNCTION: 0x493390
void __stdcall UpdateEnergyReadout(Gui* obj, int unused)
{
    char buf[52];
    Gadget* value = FindGadgetChecked_D(obj->layer->entries, "ENERGY");
    if (value != 0) {
        sprintf(buf, "%d", ReadSliderValue(value));
        SetTranslatedTextByName(obj, "ENERGY#", buf, 0);
    }
}

static inline int TestBit(unsigned int* set, unsigned short n)
{
    return set[n >> 5] & (1 << (n & 0x1f));
}

// FUNCTION: 0x4933e0
void __stdcall GiveSelectedUnitsToPlayer(unsigned char player)
{
    extern Game* g_game;
    std::vector<Unit*> list;
    CollectSelectedUnits(&list);
    Player* p = &g_game->players[player];
    unsigned int* set = (unsigned int *)GetCategoryMask("Commander");
    for (std::vector<Unit*>::iterator it = list.begin(); it != list.end(); it++) {
        Unit* unit = *it;
        if ((unit->flags & 3) != 2 && unit->cargo == 0 && unit->carrier == 0
            && !TestBit(set, unit->unitDefIndex)) {
            GiveUnitToPlayer(unit, p, 0);
        }
    }
}

static inline int IsPlayerInGame(Player* p)
{
    return p->active != 0 && (p->type == 1 || p->type == 2 || p->type == 3)
        && p->index != 10;
}

static inline int IsPlayerAlive(Player* p)
{
    return (p->type == 1 || p->type == 2 || p->type == 3)
        && (p->unitCount != 0 || p->unitsCreated == 0);
}

// FUNCTION: 0x4934b0
void __stdcall HandleShareDialogEvent(Gui* obj)
{
    extern Game* g_game;
    Gadget* data = obj->layer->entries;

    if (obj->hotGadgetIndex == -1) {
        Gadget* e = FindGadgetChecked(data, "PLYRLIST");
        GameFreeThunk(e->u.list.records);
        g_game->flags_37ebe &= ~0x40;
        return;
    }
    if (IsCurrentGadgetNamed(obj, "MAPINFO")) {
        MarkChanged(obj);
        PlaySoundByName("Options", 0);
        ClearSelectedGadget(obj);
        return;
    }
    if (IsCurrentGadgetNamed(obj, "SHARUNIT")) {
        MarkChanged(obj);
        PlaySoundByName("Options", 0);
        ClearSelectedGadget(obj);
        return;
    }
    if (IsCurrentGadgetNamed(obj, "OK")) {
        PlaySoundByName("Options", 0);
        Gadget* plyr = FindGadgetChecked(data, "PLYRLIST");
        short idx = plyr->u.list.selected;
        if (idx < 0)
            return;
        int pi = FindSlotByDpid(g_shareDialogPlayerNetIds[idx]);
        Player* p = &g_game->players[pi];
        if (IsPlayerInGame(p) && !(p->info->flags_9b & 0x40) && IsPlayerAlive(p)) {
            TransferEnergy(g_game->localPlayer, pi,
                         (float)ReadSliderValue(FindGadgetChecked_D(data, "METAL")), 1);
            TransferMetal(g_game->localPlayer, pi,
                         (float)ReadSliderValue(FindGadgetChecked_D(data, "ENERGY")), 1);
            if (GetButtonStageByName(obj, "SHARUNIT"))
                GiveSelectedUnitsToPlayer(pi);
            if (GetButtonStageByName(obj, "MAPINFO")) {
                ShareMapInfo(g_game->localPlayer, pi);
                SendShareMapInfo(g_game->localPlayer, pi);
            }
        }
        return;
    }
    if (IsCurrentGadgetNamed(obj, "CANCEL")) {
        PlaySoundByName("Previous", 0);
        return;
    }
    if (obj->hotGadgetIndex != -1)
        ClearSelectedGadget(obj);
}

// FUNCTION: 0x4936f0
void OpenShareDialog()
{
    extern Game* g_game;
    if (g_game->players[g_game->localPlayer].info->bit6)
        return;
    Layer* layer = LoadGuiLayer(&g_game->gui, "SHARE.GUI", 0x800);
    g_game->bits_37ebe_3.bit6_37ebe = 1;
    Gadget* entries = layer->entries;
    layer->handler = HandleShareDialogEvent;
    layer->data = g_game;
    int idx = FindGadgetIndex(entries, "METAL", 0xe);
    if (idx != -1) {
        Gadget* e = &entries[idx];
        e->knobSize = layer->entries[idx].height;
        e->range = layer->entries[idx].width - e->knobSize;
        e->max = (int)g_game->players[g_game->localPlayer].metal;
        e->sliderCallback = (void (__stdcall*)(Gui*, int))UpdateMetalReadout;
        e->knobPos = 0;
        SetSliderFromValue(e, 0);
        e->sliderUser = (int)g_game;
    }
    idx = FindGadgetIndex(layer->entries, "ENERGY", 0xe);
    if (idx != -1) {
        Gadget* e = FindGadgetChecked_D(layer->entries, "ENERGY");
        e->knobSize = layer->entries[idx].height;
        e->range = layer->entries[idx].width - e->knobSize;
        e->max = (int)g_game->players[g_game->localPlayer].energy;
        e->sliderCallback = (void (__stdcall*)(Gui*, int))UpdateEnergyReadout;
        e->knobPos = 0;
        SetSliderFromValue(e, 0);
        e->sliderUser = (int)g_game;
    }

    char* names = (char*)GameAllocIgnoreTag("PLAYERS", g_game->numPlayers * 30);
    char* np = names;
    *np = 0;
    memset(g_shareDialogPlayerNetIds, -1, sizeof(g_shareDialogPlayerNetIds));
    int* ids = g_shareDialogPlayerNetIds;
    int count = 0;
    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (p->active && (p->type == 1 || p->type == 2 || p->type == 3) && p->index != 10 &&
            (p->unitCount != 0 || p->unitsCreated == 0) && p->type != 1 && !p->info->bit6) {
            strcpy(np, p->name);
            np += strlen(p->name) + 1;
            *ids = p->id;
            count++;
            ids++;
        }
    }
    if (count == 0) {
        CloseTopScreen(&g_game->gui);
        return;
    }
    ConfigureListBoxByName(&g_game->gui, "PLYRLIST", names, count, 0);
    char text[0x34];
    Gui* menu = &g_game->gui;
    Layer* lyr = menu->layer;
    Gadget* ents = lyr->entries;
    Gadget* e = FindGadgetChecked_D(ents, "METAL");
    if (e) {
        sprintf(text, "%d", ReadSliderValue(e));
        SetTranslatedTextByName(menu, "METAL#", text, 0);
    }
    menu = &g_game->gui;
    lyr = menu->layer;
    ents = lyr->entries;
    e = FindGadgetChecked_D(ents, "ENERGY");
    if (e) {
        sprintf(text, "%d", ReadSliderValue(e));
        SetTranslatedTextByName(menu, "ENERGY#", text, 0);
    }
    MarkChanged(&g_game->gui);
    SetKeyboardInput(&g_game->gui, 1);
    RenderLayer(&g_game->gui, 0x40);
}

// FUNCTION: 0x493ae0
void ResetPlayerGadgets(void)
{
    extern Game* g_game;
    char buf[52];
    unsigned char* p = g_game->players[g_game->localPlayer].allied;
    for (int i = 0; i < 10; i++, p++) {
        sprintf(buf, "PLAYER%d", i);
        SetGadgetActiveByName(&g_game->gui, buf, 0);
        unsigned char state = g_game->players[i].type;
        if (state != 0 && state != 4 && i != g_game->localPlayer) {
            sprintf(buf, "LIVEPLYR%d", i);
            int value = 0;
            unsigned char* flags = g_game->chatRecipients;
            switch (g_game->mode_2bf0) {
            case 1:
                value = *p;
                break;
            case 2:
                value = (*p == 0);
                break;
            case 0:
                value = 1;
                break;
            case 3:
                value = flags[i];
                break;
            }
            SetButtonStageByName(&g_game->gui, buf, value);
        }
    }
}

// FUNCTION: 0x493bf0
void __stdcall HandleTalkDialogEvent(Gui* gadget)
{
    char buf2[0x12c];
    char buf[0x100];
    int oldmode;
    int mode;
    int n;
    int d;
    extern Game* g_game;
    Gadget* entries = gadget->layer->entries;
    if (gadget->hotGadgetIndex == -1) {
        g_game->flags_37ebe &= ~4;
        return;
    }
    if (_strnicmp(entries[gadget->hotGadgetIndex].name, g_livePlayerPrefix, 8) == 0) {
        PlaySoundByName(g_smallButtonSoundName, 0);
        g_game->mode_2bf0 = 3;
        SetButtonStageByName(gadget, g_sendTypeGadgetName, g_game->mode_2bf0);
        n = atoi(&entries[gadget->hotGadgetIndex].name[8]);
        unsigned char v = (unsigned char)GetButtonStage(gadget, gadget->hotGadgetIndex);
        g_game->chatRecipients[n] = v;
        MarkChanged(gadget);
        ClearSelectedGadget(gadget);
        goto tail;
    }
    if (IsCurrentGadgetNamed(gadget, g_sendToGadgetName)) {
        PlaySoundByName(g_smallButtonSoundName, 0);
        unsigned char v = (unsigned char)GetButtonStage(gadget, gadget->hotGadgetIndex);
        (*(Flags16*)&g_game->lobbyUiDirtyFlags_2bee).bit8 = v & 1;
        GetGadgetText(gadget, g_talkGadgetName, g_chatDraftText);
        CloseTopScreen(gadget);
        OpenTalkDialog();
        ClearSelectedGadget(gadget);
        return;
    }
    if (IsCurrentGadgetNamed(gadget, g_sendTypeGadgetName)) {
        PlaySoundByName(g_smallButtonSoundName, 0);
        g_game->mode_2bf0 = (unsigned char)GetButtonStageByName(gadget, g_sendTypeGadgetName);
        if (g_game->mode_2bf0 >= 4)
            g_game->mode_2bf0 = 0;
        ResetPlayerGadgets();
        ClearSelectedGadget(gadget);
        goto tail;
    }
    if (IsCurrentGadgetNamed(gadget, g_talkGadgetName)) {
        Gadget* talk = FindGadgetChecked_B(entries, g_talkGadgetName);
        mode = g_game->mode_2bf0;
        {
            const char* src = (char*)talk + 0xb6;
            int i = 0;
            while (i < 0xff && src[i]) {
                buf[i] = src[i];
                i++;
            }
            buf[i] = 0;
        }
        char* p = buf;
        while (*p && *p == ' ')
            p++;
        if (*p == '+') {
            int flags = 1;
            if (flags & (unsigned char)(g_game->uiOptionFlags >> 1))
                flags = 7;
            if (g_nonCampaignGame)
                flags |= 2;
            int r = ExecuteCommandLine(p + 1, flags);
            entries = gadget->layer->entries;
            if (r & 2)
                mode = 0;
        }
        if (strlen(p) != 0) {
            oldmode = g_game->mode_2bf0;
            char* to = 0;
            Player* base = &g_game->players[g_game->localPlayer];
            Saved saved = *(Saved*)g_game->chatRecipients;
            if (!(' ' < p[1] && strchr(g_chatTargetSeparators, p[1]) != 0)) goto skip0;
            if (isdigit(p[0])) {
                d = p[0] - '0';
                if (d < 0 || d > 9 || g_game->players[d].id == 0)
                    goto clear;
                p += 2;
                mode = 3;
                to = g_game->players[d].name;
                memset(g_game->chatRecipients, 0, 11);
                g_game->chatRecipients[d] = 1;
            } else {
                int c = tolower(p[0]);
                if (c != 'a') {
                    if (c == 'e') {
                        mode = 2;
                        to = g_enemiesChatTarget;
                    } else {
                        goto after;
                    }
                } else {
                    mode = 1;
                    to = g_alliesChatTarget;
                }
                p += 2;
            }
skip0:;
after:
            g_game->mode_2bf0 = mode;
            memset(buf2, 0, sizeof(buf2));
            SendChatMessage(base, p, 4, to);
            *(Saved*)g_game->chatRecipients = saved;
            g_game->mode_2bf0 = oldmode;
        }
clear:
        memset(g_chatDraftText, 0, 0x81);
        (*(Flags16*)&g_game->lobbyUiDirtyFlags_2bee).bit8 = 0;
    }
tail:
    int index = FindGadgetIndex(entries, g_talkGadgetName, 3);
    TrySetFocus(&g_game->gui, index);
    g_game->gui.layer->current = FindGadgetIndex(entries, g_talkGadgetName, 3);
}

// GLOBAL: 0x511de8
extern Game* g_game;
// FUNCTION: 0x494050
void OpenTalkDialog()
{
    PlayerInfo* info = g_game->players[g_game->localPlayer].info;
    if (info->bit6)
        return;
    if (g_chatDraftInitialized == 0) {
        g_chatDraftInitialized = 1;
        memset(g_chatDraftText, 0, 0x81);
    }
    if (g_game->flags_37ebe & 0x800)
        return;
    int multi = (g_game->lobbyUiDirtyFlags & 0x100)
                && g_game->net->GetGameType() == 3;
    Layer* d = LoadGuiLayer(&g_game->gui,
                            multi ? "TALK2.GUI" : "TALK.GUI",
                            multi ? 0x800 : 0x880);
    Gadget* entries = d->entries;
    d->handler = HandleTalkDialogEvent;
    g_game->flags_37ebe |= 4;
    SetTranslatedTextByName(&g_game->gui, "TALK", g_chatDraftText, 0);
    SetButtonStageByName(&g_game->gui, "SENDTO", multi);
    if (g_game->net->GetGameType() != 3) {
        SetGadgetActiveByName(&g_game->gui, "SENDTO", 0);
    } else if (multi) {
        SetButtonStageByName(&g_game->gui, "SENDTYPE", g_game->mode_2bf0);
        RefreshAlliesScreen(1);
        ResetPlayerGadgets();
    }
    TrySetFocus(&g_game->gui, FindGadgetIndex(entries, "TALK", 3));
    d->current = FindGadgetIndex(entries, "TALK", 3);
    d->data = g_game;
    RenderLayer(&g_game->gui, 0x40 | (multi ? 0 : 0x80));
}

// FUNCTION: 0x494220
void __stdcall HandleUnitInfoDialogEvent(Gui* gadget)
{
    if (gadget->hotGadgetIndex == -1) {
        Gadget* e = FindGadgetChecked_E(gadget->layer->entries, "HOTR");
        FreeSurface((Surface *)e->u.anim.value);
        g_game->flags_37ebe &= ~0x800;
        return;
    }
    if (IsCurrentGadgetNamed(gadget, "DONE")) {
        PlaySoundByName("smlbutton", 0);
        return;
    }
    ClearSelectedGadget(&g_game->gui);
}

// FUNCTION: 0x494290
void __stdcall DrawUnitInfoImage(Gui* gadget, Gadget* entry)
{
    if (entry->u.anim.value) {
        Rect r;
        GetGadgetRect(entry, &r);
        DrawSurface(gadget->layer->entries->u.assets.surface, (void*)entry->u.anim.value, r.left, r.top);
    }
}

// FUNCTION: 0x4942e0
void __stdcall OpenUnitInfoDialog(void)
{
    if (g_game->bits_37ebe_4.bit11_37ebe)
        return;

    int y;
    char* stats;
    char name[0x20];
    char buf[0x100];
    unsigned short type = 0;

    if (g_game->gui.hoverGadgetIndex != -1) {
        strncpy(name, g_game->gui.layer->entries[g_game->gui.hoverGadgetIndex].name, 0x10);
        name[0x10] = 0;
        type = FindUnitTypeId(name);
    } else {
        unsigned short t = g_game->hoverUnitId;
        if (t != 0) {
            Unit* unit = &g_game->units[t];
            Player* owner = &g_game->players[g_game->playerIndex];
            if (IsUnitVisibleToPlayer(owner, unit) == 0)
                type = 0;
            else
                type = unit->unitDefIndex;
        }
    }

    if (type == 0)
        return;

    Layer* layer = LoadGuiLayer(&g_game->gui, "UNITINFOx.GUI", 0x1000);
    Gadget* entries = layer->entries;
    layer->handler = HandleUnitInfoDialogEvent;
    layer->data = g_game;
    Gadget* hotr = FindGadgetChecked_E(entries, "HOTR");
    hotr->u.hotspot.callback = (void (__stdcall*)(Gui*, Gadget*))DrawUnitInfoImage;
    char* def = (char *)g_game->defs.get() + 0x249 * (unsigned)type;
    BuildDataPath(buf, "unitpics", def + 0x20, "PCX");
    hotr->u.anim.value = (int)LoadPcx(buf, 0);
    stats = MakePropList((UnitDef *)def);
    int n = entries->u.count;

    AddTextGadget(g_game->gui.layer, "TEXT", Translate("Cost"), 0x82, 0x20, -1, 2);
    n++;
    entries[n].attribs = 0x411;
    AddTextGadget(g_game->gui.layer, "TEXT", Translate("Energy"), 0x8c, 0x2f, -1, 2);
    n++;
    entries[n].attribs = 0x411;
    AddTextGadget(g_game->gui.layer, "TEXT", Translate("Metal"), 0x8c, 0x3e, -1, 2);
    n++;
    entries[n].attribs = 0x411;
    AddTextGadget(g_game->gui.layer, "TEXT", Translate("Build Time"), 0x8c, 0x4d, -1, 2);
    n++;
    entries[n].attribs = 0x411;
    AddTextGadget(g_game->gui.layer, "TEXT", Translate("Statistics"), 0x82, 0x5c, -1, 2);
    n++;
    entries[n].attribs = 0x411;
    AddTextGadget(g_game->gui.layer, "TEXT", Translate("Max Velocity"), 0x8c, 0x6b, -1, 2);
    n++;
    entries[n].attribs = 0x411;
    AddTextGadget(g_game->gui.layer, "TEXT", Translate("Acceleration"), 0x8c, 0x7a, -1, 2);
    n++;
    entries[n].attribs = 0x411;
    AddTextGadget(g_game->gui.layer, "TEXT", Translate("Turn Rate"), 0x8c, 0x89, -1, 2);
    n++;
    entries[n].attribs = 0x411;

    char* s = stats;
    if (*s != 0) {
        y = 0x20;
        do {
            AddTextGadget(g_game->gui.layer, "TEXT", s, 0xf0, y, -1, 2);
            n++;
            entries[n].attribs = 0x411;
            s += strlen(s) + 1;
            y += 0xf;
        } while (*s != 0);
    }

    GameFreeThunk(stats);
    SetTranslatedTextByName(&g_game->gui, "NAME", def, 0x80);
    MarkChanged(&g_game->gui);
}

// FUNCTION: 0x494740
void __stdcall HandleTabMenuEvent(Gui* gadget)
{
    if (gadget->hotGadgetIndex == -1) {
        g_game->lobbyUiDirtyFlags &= 0xff1f;
        DisableKeyCommands(gadget);
        return;
    }
    if (IsCurrentGadgetNamed(gadget, "OPTIONS")) {
        g_game->ordersPanelFlags |= 1;
        PlaySoundByName("BigButton", 0);
        OpenInGameOptions();
        ClearSelectedGadget(gadget);
        return;
    }
    if (IsCurrentGadgetNamed(gadget, "SHARE")) {
        PlaySoundByName("BigButton", 0);
        OpenShareDialog();
        ClearSelectedGadget(gadget);
        return;
    }
    if (IsCurrentGadgetNamed(gadget, "CONTROL")) {
        PlaySoundByName("BigButton", 0);
        OpenControlDialog();
        ClearSelectedGadget(gadget);
        return;
    }
    if (IsCurrentGadgetNamed(gadget, "ALLIES")) {
        PlaySoundByName("BigButton", 0);
        OpenAlliesDialog();
        ClearSelectedGadget(gadget);
        return;
    }
    if (IsCurrentGadgetNamed(gadget, "CANCEL")) {
        CloseTopScreen(gadget);
        return;
    }
    ClearSelectedGadget(gadget);
}

// FUNCTION: 0x494840
void __stdcall HandleTabMenuCancel(Gui* gadget)
{
    if (gadget->hotGadgetIndex == -1) {
        g_game->lobbyUiDirtyFlags &= 0xff1f;
        DisableKeyCommands(&g_game->gui);
        return;
    }
    if (!IsCurrentGadgetNamed(gadget, "CANCEL"))
        ClearSelectedGadget(gadget);
}

// FUNCTION: 0x494890
void __stdcall HandleMain2LayoutEvent(Gui* gadget)
{
    if (gadget->hotGadgetIndex != -1) {
        ClearSelectedGadget(gadget);
    }
}

// FUNCTION: 0x4948b0
void __stdcall FlashScorePanelKillLoss(int param_1, int param_2)
{
    if (param_1 >= 0) {
        g_scorePanelKillFlash[param_1] = 0x1e;
    }
    if (param_2 >= 0) {
        g_scorePanelLossFlash[param_2] = 0x1e;
    }
}

// FUNCTION: 0x4948e0
void __stdcall DrawScorePanel(void* surface)
{
    if (g_scorePanelFlashDecayTick < (int)GetTicks()) {
        g_scorePanelFlashDecayTick = GetTicks() + 1;
        for (int i = 0; i < 10; i++) {
            if (g_scorePanelKillFlash[i] > 0)
                g_scorePanelKillFlash[i] -= 2;
            if (g_scorePanelLossFlash[i] > 0)
                g_scorePanelLossFlash[i] -= 2;
        }
    }

    if (!(g_game->visualFlagsByte & 0x80)
        && (IsKeyDown(0x20) == 0
            || (g_game->gui.focus != -1
                && g_game->gui.layer->entries[g_game->gui.focus].type == 3))) {
        if (g_scorePanelSlidePos <= 0)
            return;
        if (g_scorePanelSlidePos == 0x7d)
            PlaySoundByName("Panel", 0);
        int q = g_scorePanelSlidePos / 4;
        if (q <= 1)
            q = 1;
        g_scorePanelSlidePos -= q;
        if (g_scorePanelSlidePos <= 0) {
            g_scorePanelSlidePos = 0;
            PlaySoundByName("Options", 0);
        }
    } else if (g_scorePanelSlidePos < 0x7d) {
        if (g_scorePanelSlidePos == 0)
            PlaySoundByName("Panel", g_scorePanelSlidePos);
        int q = (0x7d - g_scorePanelSlidePos) / 4;
        if (q <= 1)
            q = 1;
        g_scorePanelSlidePos += q;
        if (g_scorePanelSlidePos >= 0x7d) {
            g_scorePanelSlidePos = 0x7d;
            PlaySoundByName("Options", 0);
        }
    }

    Rect panel;
    panel.left = GetScreenWidth() - g_scorePanelSlidePos;
    panel.top = 0x20;
    panel.right = panel.left + 0x7d;
    panel.bottom = g_game->numPlayers * 0x28 + 0x2e;
    FadeRectangle((Surface *)surface, &panel, -0x18);

    panel.right = panel.left + 0x7d;
    int y = panel.top;
    int maxw = panel.right - panel.left - 6;
    char buf[100];
    Quad src;
    src.p[0].x=1; src.p[0].y=1; src.p[3].x=1; src.p[1].y=1;
    strcpy(buf, Translate("Kills"));
    DrawTextClipped(surface, buf, panel.left + 2, y, maxw, 0);
    strcpy(buf, Translate("Losses"));
    DrawTextClipped(surface, buf, panel.right - GetTextPixelWidth(buf) - 2, y, maxw, 0);
    y += 0xf;

    for (int i = 0; i < (int)g_game->numPlayers; i++) {
        Player* p = g_game->players;
        Quad dst;
        dst.p[0].x = dst.p[3].x = panel.left + 7;
        dst.p[2].x = dst.p[1].x = panel.left + maxw;
        dst.p[0].y = dst.p[1].y = y + 1;
        dst.p[2].y = dst.p[3].y = y + 0x25;

        int n;
        for (n = 0; n < 10; n++, p++) {
            if (p->active == 0)
                continue;
            unsigned char c = p->type;
            if (c != 1 && c != 2 && c != 3)
                continue;
            if (p->index == 0xa)
                continue;
            if (p->unitCount == 0 && p->unitsCreated != 0)
                continue;
            if (p->info->flags_9b & 0x40)
                continue;
            if (p->rank != i)
                continue;
            Rect hr;
            hr.left = panel.left + 4;
            hr.right = panel.right - 4;
            hr.top = y - 1;
            hr.bottom = y + 0x26;
            if (n == g_game->localPlayer) {
                FadeRectangle((Surface *)surface, &hr, 0x1f);
                FadeRectangle((Surface *)surface, &hr, 0x14);
            }
            unsigned short* frame = (unsigned short*)GetGafFrame(
                (void*)(int)g_game->logos32, p->info->color);

            src.p[1].x = frame[0] - 1;
            src.p[2].x = frame[0] - 1;
            src.p[2].y = frame[1] - 1;
            src.p[3].y = frame[1] - 1;
            DrawFrameQuad(surface, frame, &dst, &src);

            DrawTextClipped(surface, p->name, dst.p[0].x + 2, dst.p[0].y + 5, maxw, 0);
            int kills = g_game->commanderDeath == 2 ? p->commanderKills : p->kills;
            sprintf(buf, "%d", kills);
            DrawTextClipped(surface, buf, dst.p[0].x + 2, dst.p[0].y + 0x14, maxw,
                         g_scorePanelKillFlash[n]);
            int losses = g_game->commanderDeath == 2 ? p->commanderLosses : p->losses;
            sprintf(buf, "%d", losses);
            DrawTextClipped(surface, buf, dst.p[1].x - GetTextPixelWidth(buf) - 2,
                         dst.p[0].y + 0x14, maxw, g_scorePanelLossFlash[n]);
            y += 0x28;
            break;
        }
        if (n == 10) {
            Player* q = g_game->players;
            for (int k = n; k != 0; k--, q++) {
                if (q->active == 0)
                    continue;
                unsigned char c = q->type;
                if (c != 1 && c != 2 && c != 3)
                    continue;
                if (q->index == 0xa)
                    continue;
                if (q->unitCount == 0 && q->unitsCreated != 0)
                    continue;
                if (q->info->flags_9b & 0x40)
                    continue;
                if (q->rank > i)
                    q->rank--;
            }
        }
    }
}

// FUNCTION: 0x494e70
void UpdateCdCategoryByActivity()
{
    if ((g_game->netGameFlags & 4)
        && (!((*(volatile unsigned short*)&g_game->loadingFlags) & 1) || ((*(volatile unsigned short*)&g_game->loadingFlags) & 2))
        && GetTicks() > g_cdActivityLastSampleTick + 0x1e) {
        int state = ((Sound *)g_game->sound)->GetTrackCategory();
        if (++g_cdActivityStableTicks > 10) {
            int recent = 0;
            int n = 5;
            int i = g_cdActivityRingWriteIdx - 1;
            int total = 0;
            int* p = &g_cdActivitySampleRing[i];
            for (;;) {
                if (i < 0) {
                    i += 30;
                    p += 30;
                }
                int v = *p;
                total += v;
                if (n != 0) {
                    n--;
                    recent += v;
                }
                if (i == g_cdActivityRingWriteIdx)
                    break;
                i--;
                p--;
            }
            int newstate = g_lastCdActivityMode;
            if (state == 0 && (total > 0x32 || recent > 0x1e)
                && g_game->players[g_game->localPlayer].unitCount > 0x1e)
                newstate = 1;
            else if (state == 1 && total < 10 && recent == 0 && g_cdActivityStableTicks > 0x3c)
                newstate = 0;
            if (newstate != g_lastCdActivityMode) {
                ((Sound *)g_game->sound)->SetTrackCategory(newstate);
                g_cdActivityStableTicks = 0;
                g_lastCdActivityMode = newstate;
            }
        }
        if (++g_cdActivityRingWriteIdx >= 30)
            g_cdActivityRingWriteIdx = 0;
        g_cdActivitySampleRing[g_cdActivityRingWriteIdx] = 0;
        ReportIntervalTimer();
        g_cdActivityLastSampleTick = GetTicks();
    }
}

// FUNCTION: 0x494ff0
void __stdcall AddCdActivitySample(int param_1)
{
    g_cdActivitySampleRing[g_cdActivityRingWriteIdx] += param_1;
}

// FUNCTION: 0x495010
void ToggleTabMenu()
{
    PlaySoundByName("SmallButton", 0);
    unsigned short f = g_game->lobbyUiDirtyFlags;
    if (f & 0xe0) {
        g_game->lobbyUiDirtyFlags = f & 0xff1f;
        if (IsScreenNamed(&g_game->gui, "TABMENU.GUI"))
            CloseTopScreen(&g_game->gui);
        return;
    }
    g_game->lobbyUiDirtyFlags = (f & 0xff3f) | 0x20;
    HideSoftwareCursor();
    Layer* d = LoadGuiLayer(&g_game->gui, "TABMENU.GUI", 0x800);
    d->data = g_game;
    d->handler = HandleTabMenuEvent;

    int count = 0;
    Player* p = g_game->players;
    for (int i = 0; i < 10; i++, p++) {
        if (p->active != 0 && p->type == 1)
            continue;
        if (p->info->bit6)
            continue;
        count++;
    }
    int mode = g_game->net->GetGameType();
    if (mode == 3 && !g_game->players_1b63[g_game->localPlayer].info->bit6) {
        int v = count > 0;
        SetGadgetActiveByName(&g_game->gui, "ALLIES", v);
        SetGadgetActiveByName(&g_game->gui, "SHARE", v);
        int ctl = !(g_game->lockFlags & 1) && IsHostLocal();
        SetGadgetActiveByName(&g_game->gui, "CONTROL", ctl);
    } else {
        SetGadgetActiveByName(&g_game->gui, "ALLIES", 0);
        SetGadgetActiveByName(&g_game->gui, "SHARE", 0);
        SetGadgetActiveByName(&g_game->gui, "CONTROL", 0);
    }
    EnableKeyCommands(&g_game->gui);
    RenderLayer(&g_game->gui, 0x40);
    ShowSoftwareCursor();
}

// FUNCTION: 0x495200
void OpenMain2Layout()
{
    Layer* gadget = LoadGuiLayer(&g_game->gui, g_game->name, 0x20);
    gadget->handler = HandleMain2LayoutEvent;
    gadget->data = g_game;
}

#include "../../interfaces/ref.h"
#include "../harness/harness.h"

// FUNCTION: 0x495230
void UpdateFramePacing()
{
    if (HarnessTestMode()) {
        g_game->frames_38a3b = (int)HarnessStepsPerFrame();
        return;
    }
    unsigned int now = GetTicks();
    g_game->scrollScale = now - g_game->lastTick;
    g_game->lastTick = now;
    unsigned short speed = (unsigned short)g_game->effectiveGameSpeed;
    double rate = speed * 0.1;
    g_game->bits_38a51_3.faster = speed < g_game->gameSpeed;

    int best = g_game->ticks;
    g_game->slowest = 0;
    for (Player* p = g_game->players; p != g_game->players + 10; p++) {
        if (p->active != 0 && p->type == 3 && p->unitCount > 0) {
            int tick = p->syncTick;
            if (tick < best) {
                g_game->slowest = p;
                best = tick;
            }
        }
    }
    g_game->lag = g_game->ticks - best;

    if (g_game->lag >= 900) {
        int lag = g_game->lag;
        if (lag > 3600)
            lag = 3600;
        double f = (3600 - lag) * 0.00037037037037037035;
        if (f < 0.01)
            f = 0.01;
        g_game->bits_38a51_3.lagging = 1;
        rate *= f;
    } else {
        g_game->bits_38a51_3.lagging = 0;
    }

    double x = g_game->scrollScale * rate + (*(float*)&g_game->carry);
    double whole = floor(x);
    g_game->frames_38a3b = (int)whole;
    (*(float*)&g_game->carry) = x - whole;
    if (g_game->frames_38a3b < 0)
        g_game->frames_38a3b = 0;
    if (g_game->bits_38a51_3.paused) {
        g_game->frames_38a3b = 0;
        return;
    }
    if (g_game->frames_38a3b > 5) {
        g_game->frames_38a3b = 5;
        g_game->streak++;
        if (g_game->streak > 10) {
            g_game->streak = 0;
            if ((unsigned short)g_game->effectiveGameSpeed > 1)
                (*(unsigned short*)&g_game->effectiveGameSpeed)--;
        }
    } else {
        g_game->streak--;
        if (g_game->streak < -100) {
            g_game->streak = 0;
            if ((unsigned short)g_game->effectiveGameSpeed < g_game->gameSpeed)
                (*(unsigned short*)&g_game->effectiveGameSpeed)++;
        }
    }
}


// FUNCTION: 0x495490
void __stdcall RunGameSteps(int showStats)
{
    int n = g_game->frames_38a3b;

    while (n--) {
        g_game->ticks++;
        HarnessBeforeStep();

        if (showStats) {
            HandleNetPackets();
            HarnessPhase(0);
            AccumulateProfileTime(&g_game->timers, 0);
        }
        g_seams.units->UpdateAllUnits();
        HarnessPhase(1);
        AccumulateProfileTime(&g_game->timers, 1);
        g_seams.weapons->UpdateProjectiles();
        HarnessPhase(2);
        AccumulateProfileTime(&g_game->timers, 7);
        g_seams.weapons->UpdateExplosions();
        HarnessPhase(3);
        AccumulateProfileTime(&g_game->timers, 8);
        g_seams.players->UpdatePlayers();
        HarnessPhase(4);
        AccumulateProfileTime(&g_game->timers, 2);

        g_seams.features->UpdateFeatures();
        HarnessPhase(5);
        StepAllGafSequences();
        g_seams.wind_meteors->UpdateWind();
        HarnessPhase(6);
        g_seams.wind_meteors->UpdateMeteors();
        HarnessPhase(7);
        UpdateCameraFollow();
        AccumulateProfileTime(&g_game->timers, 8);

        UpdateParticles();
        AccumulateProfileTime(&g_game->timers, 6);
        g_seams.players->UpdateBlink();
        AccumulateProfileTime(&g_game->timers, 8);

        Player* shown = &g_game->players[g_game->playerIndex];
        if (shown->displayTimer < g_game->ticks)
            shown->displayTimer += 0x1e;
        g_seams.los_radar->ExpireEyeballs();

        if (showStats && g_usePacketManager != 0) {
            g_seams.economy->UpdateResourceSharing(&g_game->players[g_game->localPlayer]);
            g_packetManager.SendAllQueued(0);
            AccumulateProfileTime(&g_game->timers, 0);
        }
        HarnessAfterStep();
    }

    DrainSimEvents();
    EmptyPostSimStepHook();
    EmptyPostSimStepHook_B();
    EmptyPostSimStepHook_C();
    ExpireOldestMessage();
    AccumulateProfileTime(&g_game->timers, 8);
}

// FUNCTION: 0x4956c0
void __stdcall HandleDebugHotkey(int eventType)
{
    switch (eventType) {
    case 0x69:
        g_game->bits_3923b_1.bit0_3923b = !g_game->bits_3923b_1.bit0_3923b;
        break;
    case 0x6d:
        g_game->debugMode++;
        if (g_game->debugMode == 5)
            g_game->debugMode = 0;
        break;
    case 0x50:
        SetPageFlipping(1);
        break;
    case 0x70:
        SetPageFlipping(0);
        break;
    case 0x5d: {
        Unit* u = &g_game->units[g_game->hoverUnitId];
        if (u->unitDefIndex != 0) {
            u->lastAttackerSlot = 10;
            u->panel.flag_110 = 1;
            u->attacker = 0;
        }
        break;
    }
    case 0x3d: {
        int k = 0;
        unsigned char i = 0;
        do {
            if (i < 10) {
                if (g_game->players[k].active != 0) {
                    unsigned char kind = g_game->players[k].type;
                    if (kind == 1 || kind == 2 || kind == 3) {
                        if (g_game->players[k].index != 10) {
                            g_game->players[k].energy = g_game->players[k].energyCapacity;
                            g_game->players[k].metal = g_game->players[k].metalCapacity;
                        }
                    }
                }
            }
            i++;
            k++;
        } while (i < 10);
        break;
    }
    default:
        break;
    }
}

// FUNCTION: 0x464e70
void ShowContinueWatchingDialog()
{
    Layer* screen = LoadGuiLayer(&g_game->gui, g_yesNoGuiName, 0x900);
    if (screen) {
        Gadget* form;
        SetKeyboardInput(&g_game->gui, 1);
        form = screen->entries;
        SetTranslatedTextByName(&g_game->gui, g_choice1GadgetName, g_yesText, 0);
        SetTranslatedTextByName(&g_game->gui, g_choice2GadgetName, g_noText, 0);
        SetTranslatedTextByName(&g_game->gui, g_titleGadgetName, g_continueWatchingTitle, 0);
        strcpy(form->u.names.choice, g_choice1GadgetName);
        strcpy(form->u.names.choice2, g_choice2GadgetName);
        screen->handler = ContinueWatchingCallback;
        RenderLayer(&g_game->gui, 0x40);
    }
}
