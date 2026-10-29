#include "win_types.h"
#include <stdio.h>
#include <string.h>
#include <new>
#include <new.h>
#include <stdlib.h>
#include "../../interfaces/host.h"

#include "../sound/sound.h"

#include "../map/mission.h"

#include "../graphics/memory_cache.h"

#include "../map/grid.h"

#pragma pack(push, 1)


#include "../graphics/rect.h"


struct Gui;

#include "../gui/gadget.h"

struct Layer;
#include "../gui/gui.h"

#include "../gui/layer.h"

#include "../graphics/display.h"

#include "../network/player_info.h"
#include "../../interfaces/seams.h"

struct EndGamePlayerStat {
    char name[0x1e];
    union {
        int stats[7];
        struct {
            int kills;
            int losses;
            int energyProduced;
            int metalProduced;
            int energyWasted;
            int metalWasted;
            int score;
        };
    };
};

#include "endgame.h"

struct Bar {
    Header h;
    char pad_2a[0xb6 - 0x2a];
    int max;
    int current;
    int value;
    int interval;
    int next;
    float scale;
    int active;
    int showText;
};

struct Button {
    Header h;
    char pad_2a[0xb6 - 0x2a];
    char unknown_b6[0xc8 - 0xb6];
    int flags;
};

#include "../game/settings.h"

#include "../network/player.h"

#include "../game/game.h"
#include "../state/sim_globals.h"
#pragma pack(pop)

// GLOBAL: 0x511de8
extern Game* g_game;
#include "../../link/globals.h"
#include "endgame.h"

#include "../debug/debug_lib.h"
#include "../graphics/object_pool.h"
unsigned int __cdecl GetTicks();
#include "../graphics/palette.h"
#include "../graphics/draw.h"
Display* GetDisplay();
#include "../game/data_files.h"
#include "../files/files_api.h"
void* __stdcall LoadBitmapByName(char* name, unsigned char* palette);
void __stdcall LoadPictureCached(const char* name, int a, int b, int c);
#include "../graphics/surface.h"
#include "frontend.h"
void FlipScreen();
#include "../network/net_game.h"
#include "../game/savegame.h"
#include "../game/cd_check.h"
#include "frontend.h"
#include "../game/game_state.h"
#include "../gui/keyboard.h"
void __stdcall SetFrontendState(int state, int line, const char* file);
#include "../map/map_list.h"
void __stdcall SetCursorMode(int n);
#include "campaign_menu.h"
const char* __stdcall Translate(const char* text);
#include "../gui/dialogs.h"
void __stdcall PlaySoundByName(const char* name, int broadcast);
int __stdcall FindGadgetIndex(Gadget* entries, char* name, int type);
Gadget* __stdcall FindGadgetChecked(Gadget* entries, char* name);
int __stdcall AddHotspotGadget(Gui* menu, Button* record);
int __stdcall AddBarGadget(Gui* menu, Bar* record);
int __stdcall AddTextGadget(Layer* holder, char* type, char* text, int x, int y,
                           int width, int attribs);
Layer* __stdcall LoadGuiLayer(Gui* menu, const char* name, int flags);
#include "endgame.h"
void __stdcall ConfigureListBoxByName(Gui* menu, char* name, void* items, int count, int flag);
Gadget* __stdcall FindGadgetChecked_D(char* entries, char* name);
void __stdcall SetGadgetActiveByName(Gui* menu, const char* name, int value);
void __stdcall SetTranslatedTextByName(Gui* menu, char* name, char* text, int value);
int __stdcall GetGafFrame(unsigned short* gaf, int frame);
void __stdcall DrawFrame(void* dst, int bmp, int x, int y);
void __stdcall SetKeyboardInput(Gui* menu, int value);
void __stdcall UpdateMenu(Gui* menu);
void __stdcall BlitMenuLayers(Gui* menu, void* dest, void* clip);
void __stdcall DrawSurface(void* dest, void* image, int x, int y);
#include "../network/net_chat.h"
void __stdcall SetOffscreenSurface(void* surface);
void* __stdcall AllocSurface(const char* name, int width, int height);
#include "../network/unit_sync.h"
#include "../graphics/display.h"
#include "../graphics/font.h"
void ShowSoftwareCursor();
void HideSoftwareCursor();
void __stdcall SendPlayerEconomy(Player* player, int a, int b);

// FUNCTION: 0x41d920
void CreateGameObject()
{
    char* mem = (char*)SimAlloc(sizeof(Game));
    memset(mem, 0, sizeof(Game));
    AllocNotifyNop((int)mem);
    g_game = new (mem) Game;
}

// FUNCTION: 0x41d9f0
void __stdcall SetEndGameState(unsigned int state)
{
    unsigned int v = state & 0xff;
    g_game->endGameState = v;
}

// FUNCTION: 0x41da10
void __stdcall SetMissionStatus(int param1, int param2, int param3)
{
    char mask;
    if (param3)
        mask = -1;
    else
        mask = 0;
    *(unsigned char*)(param2 + param1) = (unsigned char)((mask & 0xb) + 0x4c);
}

// FUNCTION: 0x41da30
void InitMissionStatus(void)
{
    memset(&g_game->campaignMissionStatus[0], 0x55, 25);
    g_game->campaignMissionStatus[25] = 0;
}

// FUNCTION: 0x41da60
void SetUpEndMissionScreen()
{
    char path[256];
    g_game->fadeTable = (char*)GameAllocIgnoreTag("FadeTable", 0x400);
    g_game->desiredPalette = (unsigned char*)GameAllocIgnoreTag("desiredPalette", 0x400);
    g_game->currentPalette = (unsigned char*)GameAllocIgnoreTag("currentPalette", 0x400);
    Display* display = GetDisplay();
    *(float*)&g_game->savedPaletteBrightness = display->paletteBrightness;
    display->paletteBrightness = 1.0f;
    unsigned char* palette = (unsigned char*)GameAllocIgnoreTag("Palette", 0x400);
    char* name = g_game->net->GetNameSlot(5);
    if (name == 0)
        g_game->image_3907b = 0;
    if (g_game->net->GetGameType() == 1
        && g_game->bits_3923b_2.bit4_3923b && name != 0) {
        BuildDataPath(path, "bitmaps\\glamour", name + 1, "PCX");
        if (HAPI_FileLengthByName(path) == 0)
            strncpy(g_game->glamour, "glamour\\Arm01.PCX", 0x100);
        else
            strncpy(g_game->glamour, path + 8, 0x100);
        g_game->image_3907b = LoadBitmapByName(g_game->glamour, palette);
        g_game->palette_3907f = palette;
        return;
    }
    if (g_game->net->GetGameType() == 1)
        LoadPictureCached("Outcome1", 0, 0, 1);
    else
        LoadPictureCached("Outcome0", 0, 0, 1);
}

// FUNCTION: 0x41dc20
void CollectEndGameStats()
{
    EndGamePlayerStat* stats = (EndGamePlayerStat*)g_game->slots_38dd9;
    g_game->won = g_game->bits_3923b_2.bit4_3923b;
    if (g_game->net->GetGameType() == 1) {
        g_game->mission = g_game->net->GetMissionIndex();
        g_game->campaignMissionStatus[g_game->mission] = g_game->won ? 'W' : 'L';
    }
    g_game->maxKills = 10;
    g_game->maxLosses = 10;
    g_game->max_26 = 100;
    g_game->max_2a = 100;
    g_game->max_2e = 100;
    g_game->max_32 = 100;
    g_game->maxScore = 100;
    memset(stats, 0, sizeof g_game->slots_38dd9);
    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if ((p->active && (p->type == 1 || p->type == 2 || p->type == 3) && p->index != 10 && !p->info->bit6) || p->unitsCreated) {
            if (p->rejectReason == 0) {
                strncpy(stats[i].name, p->name, 30);
                stats[i].kills = p->kills;
                stats[i].losses = p->losses;
                stats[i].energyProduced = (int)p->totalEnergyProduced;
                stats[i].metalProduced = (int)p->totalMetalProduced;
                stats[i].energyWasted = (int)p->energyWasted;
                stats[i].metalWasted = (int)p->metalWasted;
                Mission* c = g_game->net;
                int t = (int)((unsigned int)g_game->ticks / 60 * c->timeMul);
                stats[i].score = t + (int)(stats[i].kills * c->killMul);
                if (stats[i].score < 0)
                    stats[i].score = 0;
                if (stats[i].kills > g_game->maxKills)
                    g_game->maxKills = stats[i].kills;
                if (stats[i].losses > g_game->maxLosses)
                    g_game->maxLosses = stats[i].losses;
                if (stats[i].energyProduced > g_game->max_26)
                    g_game->max_26 = stats[i].energyProduced;
                if (stats[i].metalProduced > g_game->max_2a)
                    g_game->max_2a = stats[i].metalProduced;
                if (stats[i].energyWasted > g_game->max_2e)
                    g_game->max_2e = stats[i].energyWasted;
                if (stats[i].metalWasted > g_game->max_32)
                    g_game->max_32 = stats[i].metalWasted;
                if (stats[i].score > g_game->maxScore)
                    g_game->maxScore = stats[i].score;
            }
        }
    }
}

// FUNCTION: 0x41dee0
void StartScreenFade(void)
{
    g_game->steps = 10;
    g_game->nextTime = GetTicks() + 1;
    g_game->done = 0;
}

// FUNCTION: 0x41df20
void StepScreenFade(void)
{
    Rect rect;
    rect.left = rect.top = 0;
    rect.right = g_game->width_37e1f;
    rect.bottom = g_game->screenHeight;
    if (g_game->nextTime < GetTicks()) {
        FadeRectangle(0, &rect, g_game->steps - 0x1d);
        g_game->nextTime = GetTicks() + 1;
        g_game->steps--;
        if (g_game->steps == 0)
            g_game->done = 1;
    }
}

#define FADE_STEP(k)                                                        \
    if (current[k] > target[k]) {                                           \
        ((char*)g_game->fadeTable)[k] = min(-1, (target[k] - current[k]) / steps);       \
    } else if (current[k] == target[k]) {                                   \
        ((char*)g_game->fadeTable)[k] = 0;                                               \
    } else {                                                                \
        ((char*)g_game->fadeTable)[k] = max(1, (target[k] - current[k]) / steps);        \
    }

// FUNCTION: 0x41dfc0
void __stdcall StartPaletteFade(unsigned char* target, unsigned char* current, int steps)
{
    memcpy(((unsigned char*)g_game->desiredPalette), target, 0x400);
    memcpy(((unsigned char*)g_game->currentPalette), current, 0x400);
    SetPaletteColors(current, 0, 0x100);
    g_game->done = 0;
    for (int i = 0; i < 0x100; i++) {
        FADE_STEP(i * 4)
        FADE_STEP(i * 4 + 1)
        FADE_STEP(i * 4 + 2)
        FADE_STEP(i * 4 + 3)
    }
}

// FUNCTION: 0x41e240
void ScheduleFadeTick(void)
{
    g_game->nextTime = GetTicks() + 1;
}

// FUNCTION: 0x41e260
int IsFadeDone()
{
    return g_game->done;
}

#define FADE_TICK(k)                                                        \
    {                                                                       \
        int v = ((unsigned char*)g_game->currentPalette)[k] + ((char*)g_game->fadeTable)[k];                      \
        ((unsigned char*)g_game->currentPalette)[k] = ((char*)g_game->fadeTable)[k] < 0                           \
            ? (v < ((unsigned char*)g_game->desiredPalette)[k] ? ((unsigned char*)g_game->desiredPalette)[k] : v)               \
            : (v > ((unsigned char*)g_game->desiredPalette)[k] ? ((unsigned char*)g_game->desiredPalette)[k] : v);              \
    }

// FUNCTION: 0x41e270
void StepPaletteFade(void)
{
    if (g_game->nextTime <= GetTicks()) {
        for (int i = 0; i < 0x100; i++) {
            FADE_TICK(i * 4)
            FADE_TICK(i * 4 + 1)
            FADE_TICK(i * 4 + 2)
            FADE_TICK(i * 4 + 3)
        }
        if (memcmp(((unsigned char*)g_game->currentPalette), ((unsigned char*)g_game->desiredPalette), 0x400) == 0)
            g_game->done = 1;
        SetPaletteColors(((unsigned char*)g_game->currentPalette), 0, 0x100);
        g_game->nextTime = GetTicks() + 1;
    }
}

#define STAT_BAR(n, px, fmt)                                                \
    bar.h.x = px;                                                           \
    bar.value = g_game->slots_38dd9[i].stats[n];                                  \
    bar.max = g_game->maxStats[n];                                          \
    bar.scale = max(bar.value * 0.06666667f, 1.0f);                         \
    sprintf(bar.h.name, fmt, i);                                          \
    AddBarGadget(&g_game->gui, &bar);

// FUNCTION: 0x41e420
void FillEndGameStatistics(void)
{
    Bar bar;
    Button button;
    char name[64];
    Gui* menu = &g_game->gui;
    Gadget* entries = menu->layer->entries;
    __builtin_assume(entries != nullptr);
    memset(&bar, 0, sizeof(bar));
    bar.h.flag = 0;
    bar.h.y = 0x5d;
    bar.h.width = 0x43;
    bar.h.height = 0x12;
    bar.h.color = g_game->gui.colours[4];
    bar.h.color2 = g_game->gui.colours[8];
    bar.showText = 1;
    bar.active = 1;
    bar.current = 0;
    bar.interval = 1;
    memset(&button, 0, sizeof(button));
    button.h.flag = 1;
    button.h.attribs = 0x400;
    button.flags |= 1;
    g_game->statBarStep = 0;
    for (int i = 0; i < 10; i++) {
        if (g_game->slots_38dd9[i].active) {
            sprintf(name, "PlayerColor%d", i);
            strcpy(button.h.name, name);
            button.h.x = 0x10;
            button.h.y = bar.h.y;
            button.h.width = 0x5b;
            button.h.height = 0x15;
            AddHotspotGadget(&g_game->gui, &button);
            int idx = FindGadgetIndex(entries, name, 6);
            if (idx != -1) {
                Gadget* e = &entries[idx];
                if (e) {
                    e->u.hotspot.gaf = (GafEntry*)g_game->logos32;
                    e->u.hotspot.index = g_game->players[i].info->color;
                }
            }
            menu->language = menu->values[1];
            int h = GetFontLineHeight();
            AddTextGadget(g_game->gui.layer, "TEXT", (char*)&g_game->slots_38dd9[i], 0x10,
                         (0x14 - h) / 2 + bar.h.y, -1, 2);
            entries[entries->u.count].attribs = 2;
            entries[entries->u.count].width = 0x5a;
            menu->language = menu->values[0];
            STAT_BAR(0, 0x70, "Kills%d")
            STAT_BAR(1, 0xba, "Losses%d")
            STAT_BAR(2, 0x104, "EProduced%d")
            STAT_BAR(3, 0x14e, "MProduced%d")
            STAT_BAR(4, 0x198, "EWasted%d")
            STAT_BAR(5, 0x1e2, "MWasted%d")
            STAT_BAR(6, 0x22c, "Score%d")
            bar.h.y += 0x14;
        }
    }
}

// FUNCTION: 0x41ea30
int AreStatBarsComplete(void)
{
    Gadget* entries = g_game->gui.layer->entries;
    int count = entries->u.count;
    for (int i = 0; i < count; i++) {
        if (entries[i].type == 0xd) {
            if (entries[i].active == 0 || entries[i].u.anim.value < entries[i].u.anim.max) {
                return 0;
            }
        }
    }
    return 1;
}

// FUNCTION: 0x41eaa0
char* __stdcall BuildScrollItems1(char* names, char* flags, int count)
{
    char* items = (char*)GameAllocIgnoreTag("ScrollItems1", count << 7);
    memset(items, 0, count << 7);
    char* src = names;
    char* out = items;
    for (int i = 0; i < count; i++) {
        if (flags[i] == 'L') {
            *out++ = (char)0xff;
            *out++ = ' ';
        } else if (flags[i] == 'W') {
            *out++ = (char)0xfe;
            *out++ = ' ';
        }
        if (flags[i] == 'U') {
            *out++ = (char)0xfd;
            *out++ = ' ';
        }
        strcpy(out, src);
        int len = strlen(src) + 1;
        src += len;
        out += len;
    }
    GameFreeThunk(names);
    return items;
}

// FUNCTION: 0x41eb60
char* __stdcall BuildScrollItems2(char* names, int a, int b)
{
    char* items = (char*)GameAllocIgnoreTag("ScrollItems2", b * 0x102);
    if (a > b) {
        a = b;
    }
    char* src = names;
    char* out = items;
    int i = 0;
    for (; i < a; i++) {
        strcpy(out, src);
        int len = strlen(src) + 1;
        src += len;
        out += len;
    }
    for (; i < b; i++) {
        strcpy(out, "&G");
        out += 2;
        strcpy(out, src);
        int len = strlen(src) + 1;
        src += len;
        out += len;
    }
    GameFreeThunk(names);
    return items;
}

// FUNCTION: 0x41ec50
void __stdcall HandleEndMissionClick(Gui* gadget)
{
    Gadget* entries = gadget->layer->entries;
    Data* data = (Data*)gadget->layer->data;
    if (gadget->hotGadgetIndex == -1) {
        BlankScreen();
        if (g_game->lastFrame != 0)
            FreeSurface((Surface *)g_game->lastFrame);
        if (g_game->image_3907b != 0)
            FreeSurface((Surface *)g_game->image_3907b);
        if (g_game->palette_3907f != 0)
            GameFreeThunk(g_game->palette_3907f);
        if (((unsigned char*)g_game->currentPalette) != 0)
            GameFreeThunk(((unsigned char*)g_game->currentPalette));
        if (((unsigned char*)g_game->desiredPalette) != 0)
            GameFreeThunk(((unsigned char*)g_game->desiredPalette));
        if (((char*)g_game->fadeTable) != 0)
            GameFreeThunk(((char*)g_game->fadeTable));
        g_game->lastFrame = 0;
        g_game->image_3907b = 0;
        g_game->palette_3907f = 0;
        g_game->currentPalette = 0;
        g_game->desiredPalette = 0;
        g_game->fadeTable = 0;
        GameFreeThunk(data->items);
        GameFreeThunk(data);
        if (g_game->bits_2bee.flag4)
            LeaveNetGame();
        ((Sound*)g_game->sound)->SetTrackCategory(4);
        Display* display = GetDisplay();
        display->paletteBrightness = *(float*)&g_game->savedPaletteBrightness;
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gadget, "LoadGame")) {
        PlaySoundByName("BigButton", 0);
        ShowLoadGameScreen();
        ClearSelectedGadget(gadget);
        ClearSelectedGadget(gadget);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gadget, "SaveGame")) {
        PlaySoundByName("BigButton", 0);
        ShowSaveGameScreen();
        ClearSelectedGadget(gadget);
        ClearSelectedGadget(gadget);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gadget, "Start") || IsCurrentGadgetNamed((Dialog *)gadget, "Missions")) {
        RegisterDataArchives();
        PlaySoundByName("BigButton", 0);
        g_game->frontendNextSubState = 10;
        SetCursorOverlayEnabled(1);
        SetCursorMode(0x14);
        if (g_game->net->SelectMission(FindGadgetChecked(entries, "Missions")->u.list.selected)) {
            EnterMainMenuState();
            g_game->bits_2a44.bit2 = 0;
            g_game->bits_2a44.bit3 = 1;
            g_game->bits_2a44.bit0 = 0;
            SetMissionType(1);
            g_game->bits_3923b_2.bit4_3923b = 0;
            g_game->bits_3923b_2.bit2_3923b = 0;
            SetFrontendState(13, 757, "c:\\cavedog\\wargame\\endgame.cpp");
            SetGameMode(2);
            return;
        }
    } else if (IsCurrentGadgetNamed((Dialog *)gadget, "MainMenu")) {
        PlaySoundByName("BigButton", 0);
        SetFrontendState(2, 770, "c:\\cavedog\\wargame\\endgame.cpp");
        SetGameMode(1);
        SetCursorOverlayEnabled(1);
        SetCursorMode(0x14);
        return;
    } else if (IsCurrentGadgetNamed((Dialog *)gadget, "Difficulty")) {
        PlaySoundByName("SKirmish", 0);
        if (g_game->difficulty == 0) {
            g_game->options->difficulty = 1;
            g_game->difficulty = 1;
            ClearSelectedGadget(gadget);
            return;
        }
        if (g_game->difficulty == 1) {
            g_game->options->difficulty = 2;
            g_game->difficulty = 2;
            ClearSelectedGadget(gadget);
            return;
        }
        if (g_game->difficulty == 2) {
            g_game->options->difficulty = 0;
            g_game->difficulty = 0;
            ClearSelectedGadget(gadget);
            return;
        }
    }
    ClearSelectedGadget(gadget);
}

// FUNCTION: 0x41f040
int ShouldShowNextMission()
{
    if (g_game->net->GetGameType() == 1 &&
        ((g_game->won == 0 &&
          g_game->net->MissionExists(g_game->mission + 1) == 0) ||
         g_game->net->MissionExists(g_game->mission + 1) != 0)) {
        return 1;
    }
    return 0;
}

static inline int HasNextMission()
{
    if (g_game->net->GetGameType() == 1 &&
        ((g_game->won == 0 &&
          ((Mission*)g_game->net)->MissionExists(g_game->mission + 1) == 0) ||
         ((Mission*)g_game->net)->MissionExists(g_game->mission + 1) != 0)) {
        return 1;
    }
    return 0;
}

// FUNCTION: 0x41f0a0
void __stdcall OpenEndMissionScreen()
{
    BlankScreen();
    FillSurface((Surface *)((void*)g_game->screen), 0);
    FlipScreen();
    Layer* layer = LoadGuiLayer(&g_game->gui, "ENDMSN.GUI", 0x80);
    layer->handler = HandleEndMissionClick;
    Data* data = (Data*)GameAllocIgnoreTag("EndMsnGUI", 0x20);
    data->items = 0;
    layer->data = data;
    char* entries = (char*)layer->entries;
    char next = HasNextMission();
    if (next) {
        g_game->net->SelectMission(g_game->mission);
        LoadPictureCached("outcome1", 1, 1, 0);
        strcpy(layer->entries->u.names.choice, "Start");
    } else {
        LoadPictureCached("outcome0", 1, 1, 0);
        SelectGadgetByName(&g_game->gui, "MainMenu");
    }
    next = HasNextMission();
    if (next) {
        int count = g_game->net->BuildMissionList((char**)&data->items);
        data->items = BuildScrollItems1((char*)data->items, g_game->campaignMissionStatus, count);
        int i;
        for (i = 0; i < 0x19; i++) {
            if (g_game->campaignMissionStatus[i] == 'U')
                break;
        }
        ConfigureListBoxByName(&g_game->gui, "Missions", data->items, count, 0);
        Gadget* knob = FindGadgetChecked_D(entries, "KNOB");
        knob->range = knob->height - knob->knobSize - 3;
        SetListBoxScrollByName((SetListBoxScrollByNameDialog *)&g_game->gui, "Missions", g_game->mission);
        SetListBoxScrollByName((SetListBoxScrollByNameDialog *)&g_game->gui, "Missions", g_game->mission + (g_game->won != 0));
        ApplyDifficultyButtons();
    }
    Player* player = &g_game->players[g_game->localPlayer];
    int x = g_game->width_37e1f / 2;
    if (g_game->won != 0 && (player->active == 0 || !player->info->bit6)) {
        DrawFrame(layer->surface, GetGafFrame(g_game->image_14813, 0), x, 0x1c);
    } else {
        DrawFrame(layer->surface, GetGafFrame(g_game->image_14817, 0), x, 0x1c);
    }
    if (g_game->bits_2bee.flag4)
        SetTranslatedTextByName(&g_game->gui, "MainMenu", "OK", 0);
    SetKeyboardInput(&g_game->gui, 1);
    RenderLayer(&g_game->gui, 0x40);
    SetCursorMode(0x13);
}

// FUNCTION: 0x41f400
void EnableEndMissionButtons()
{
    char next = HasNextMission();
    if (next) {
        SetGadgetActiveByName(&g_game->gui, "Start", 1);
        SetGadgetActiveByName(&g_game->gui, "LoadGame", 1);
        SetGadgetActiveByName(&g_game->gui, "SaveGame", 1);
        SetGadgetActiveByName(&g_game->gui, "KNOB", 1);
        SetGadgetActiveByName(&g_game->gui, "Missions", 1);
        SetGadgetActiveByName(&g_game->gui, "Difficulty", 1);
        SetGadgetActiveByName(&g_game->gui, "AdjustDiff", 1);
        SetGadgetActiveByName(&g_game->gui, "MainMenu", 1);
        SelectGadgetByName(&g_game->gui, "Missions");
    } else {
        SetGadgetActiveByName(&g_game->gui, "MainMenu", 1);
        Gadget* entries = g_game->gui.layer->entries;
        entries[FindGadgetIndex(entries, "MainMenu", 1)].y = 0x1a0;
        SelectGadgetByName(&g_game->gui, "MainMenu");
    }
    MarkLayerChanged((Dialog *)&g_game->gui);
    MarkChanged((Dialog *)&g_game->gui);
}

// FUNCTION: 0x41f5c0
void __stdcall ActivatePlayerGadgets(char* prefix)
{
    char name[64];
    for (int i = 0; i < 10; i++) {
        if (g_game->slots_38dd9[i].active) {
            sprintf(name, "%s%d", prefix, i);
            SetGadgetActiveByName(&g_game->gui, name, 1);
        }
    }
}

// FUNCTION: 0x41f630
void ShowEndMissionScreen()
{
    BlankScreen();
    OpenEndMissionScreen();
    FillEndGameStatistics();
    EnableEndMissionButtons();
    MarkLayerChanged((Dialog *)&g_game->gui);
    MarkChanged((Dialog *)&g_game->gui);
    SetGameMode(7);
    g_game->endGameState = 7;
}

// FUNCTION: 0x41f680
void __stdcall HandleCdCheckClick(Gui* gadget)
{
    if (gadget->hotGadgetIndex != -1) {
        if (IsCurrentGadgetNamed((Dialog *)gadget, "OK")) {
            PlaySoundByName("Options", 0);
            g_game->endGameState = 5;
            return;
        }
        ClearSelectedGadget(gadget);
    }
}

// FUNCTION: 0x41f700
void OpenCdCheckDialog()
{
    LoadGuiLayer(&g_game->gui, "CDCHECK.GUI", 0x101)->handler = HandleCdCheckClick;
    SetCursorOverlayEnabled(1);
    SetKeyboardInput(&g_game->gui, 1);
    RenderLayer(&g_game->gui, 0x40);
}

// FUNCTION: 0x41f760
int DrawEndGameFrame()
{
    if (g_game->endGameState == 1) {
        Display* d = GetDisplay();
        DrawSurface(((void*)g_game->screen), g_game->lastFrame, d->width, d->height);
        DrawMessages(((void*)g_game->screen));
        UpdateMenu(&g_game->gui);
        BlitMenuLayers(&g_game->gui, 0, 0);
        ShowSoftwareCursor();
        FlipScreen();
        return 1;
    }
    return 0;
}

#define ENABLE_BARS(label) \
    for(int i=0;i<10;++i) { \
        if(g_game->slots_38dd9[i].active) { \
            sprintf(text,"%s%d",label,i); \
            SetGadgetActiveByName(&g_game->gui,text,1); \
        } \
    }

static inline int StatsComplete()
{
    Gadget* entries=g_game->gui.layer->entries;
    int count=entries->u.count;
    for(int i=0;i<count;++i) {
        if(entries[i].type==13) {
            if(!entries[i].active || entries[i].u.anim.value<entries[i].u.anim.max) return 0;
        }
    }
    return 1;
}

// FUNCTION: 0x41f7f0
void __stdcall RunEndGameState()
{
    char text[64];
    int event[6];
    unsigned palette[256];
    SetOffscreenSurface(((void*)g_game->screen));
    HandleNetPackets();
    switch(g_game->endGameState) {
    case 0:
        if(g_seams.map->GetGameType(g_game->net)==3) {
            Display* e=GetDisplay();
            g_game->lastFrame=AllocSurface("Copy of last game frame",e->width,e->height);
            DrawSurface(g_game->lastFrame,((void*)g_game->screen),e->width,e->height);
            ReportGameEvent(7);
            g_game->endGameState=1;
            Player* player=&g_game->players[g_game->localPlayer];
            if(player->rejectReason && player->rejectReason!=2) {
                const char* name=GetRejectReasonText(player->rejectReason);
                OpenMessageBox(&g_game->gui,(char *)Translate(name),320,1,1);
                MarkChanged((Dialog *)&g_game->gui);
                MarkLayerChanged((Dialog *)&g_game->gui);
                player->rejectReason=0;
            }
        } else { g_game->lastFrame=0; g_game->endGameState=2; }
        break;
    case 1:
        if(IsScreenNamed(&g_game->gui,"MSGBOX.GUI")) {
            if(g_game->endGameState==1) {
                Display* e=GetDisplay();
                DrawSurface(((void*)g_game->screen),g_game->lastFrame,e->width,e->height);
                DrawMessages(((void*)g_game->screen));
                UpdateMenu(&g_game->gui);
                BlitMenuLayers(&g_game->gui,0,0);
                ShowSoftwareCursor();
                FlipScreen();
            }
        } else g_game->endGameState=2;
        break;
    case 2:
        g_game->steps=10;
        g_game->nextTime=GetTicks()+1;
        g_game->done=0;
        SetCursorOverlayEnabled(0);
        g_game->endGameState=3;
        break;
    case 3:
        if(!g_game->done) {
            event[1]=0; event[0]=0;
            event[2]=g_game->width_37e1f; event[3]=g_game->screenHeight;
            if(g_game->nextTime<GetTicks()) {
                FadeRectangle(0,(Rect *)event,g_game->steps-29);
                g_game->nextTime=GetTicks()+1;
                --g_game->steps;
                if(!g_game->steps) g_game->done=1;
            }
        } else {
            Force640x480Surfaces(); g_game->endGameState=4;
            SetOffscreenSurface(((void*)g_game->screen));
        }
        break;
    case 4:
        g_game->endGameState=5;
        break;
    case 5: {
        SetUpEndMissionScreen();
        int next=g_seams.map->MissionExists(g_game->net, 0, g_game->mission+1);
        if(g_seams.map->GetGameType(g_game->net)==1 && g_game->bits_3923b_2.bit4_3923b && !next && !g_game->skip) {
            if((unsigned char)GetDisplay()->flags.bits.flag1) {
                if(!g_game->players[0].info->side) SetFrontendState(4,0x4ce,"c:\\cavedog\\wargame\\endgame.cpp");
                else SetFrontendState(5,0x4d3,"c:\\cavedog\\wargame\\endgame.cpp");
            } else SetFrontendState(2,0x4d9,"c:\\cavedog\\wargame\\endgame.cpp");
            SetGameMode(2);
        } else if(g_seams.map->GetGameType(g_game->net)==1 && g_game->bits_3923b_2.bit4_3923b && g_game->image_3907b) {
            memset(palette,0,sizeof(palette));
            StartPaletteFade((unsigned char*)g_game->palette_3907f,(unsigned char*)palette,5);
            g_game->nextTime=GetTicks()+1;
            g_game->endGameState=6;
            DrawSurface(((void*)g_game->screen),g_game->image_3907b,0,0);
        } else {
            OpenEndMissionScreen(); FillEndGameStatistics(); EnableEndMissionButtons();
            MarkLayerChanged((Dialog *)&g_game->gui); MarkChanged((Dialog *)&g_game->gui);
            g_game->endGameState=7;
        }
        break;
    }
    case 6:
        if(!g_game->done) {
            StepPaletteFade();
            unsigned now=GetTicks();
            now+=GetTickRate();
            g_game->deadline=now;
            g_endGameGlamourSoundStarted=0;
        } else {
            if(!g_endGameGlamourSoundStarted) { StartGlamourSound(); g_endGameGlamourSoundStarted=1; }
            if(g_game->deadline<GetTicks()) {
                GetCurrentMouseEvent((MouseEvent *)event);
                if(PopKey() || g_game->cursorKeyFlags) {
                    ((Sound*)g_game->sound)->StopStream();
                    OpenEndMissionScreen(); EnableEndMissionButtons(); FillEndGameStatistics();
                    MarkLayerChanged((Dialog *)&g_game->gui); MarkChanged((Dialog *)&g_game->gui);
                    g_game->endGameState=7;
                }
                unsigned deadline=GetTickRate()*5+g_game->deadline;
                if(deadline<GetTicks())
                    DrawOutlinedString((Surface *)((void*)g_game->screen),(unsigned char *)Translate("Click to continue."),g_game->gui.colours[0],g_game->gui.colours[0xf],g_game->screenHeight-20);
            }
        }
        break;
    case 7: {
        if(StatsComplete()) {
            EnableKeyCommands((Dialog *)&g_game->gui);
            g_game->endGameState=8; SetCursorMode(19); SetCursorOverlayEnabled(1);
            break;
        }
        UpdateMenu(&g_game->gui); BlitMenuLayers(&g_game->gui,0,0);
        int skip=0;
        int clicked=PopKey();
        if(clicked && g_seams.map->GetGameType(g_game->net)!=3) skip=1;
        if(g_game->deadline<GetTicks() || skip) {
            if(clicked) {
                { ENABLE_BARS("Kills") }
                { ENABLE_BARS("Losses") }
                { ENABLE_BARS("EProduced") }
                { ENABLE_BARS("MProduced") }
                { ENABLE_BARS("EWasted") }
                { ENABLE_BARS("MWasted") }
                { ENABLE_BARS("Score") }
                PlaySoundByName("ActivateAllStatBars",0);
            }
            switch(g_game->statBarStep) {
            case 0: { ENABLE_BARS("Kills") } PlaySoundByName("EndGameStatBar",0); break;
            case 1: { ENABLE_BARS("Losses") } PlaySoundByName("EndGameStatBar",0); break;
            case 2: { ENABLE_BARS("EProduced") } PlaySoundByName("EndGameStatBar",0); break;
            case 3: { ENABLE_BARS("MProduced") } PlaySoundByName("EndGameStatBar",0); break;
            case 4: { ENABLE_BARS("EWasted") } PlaySoundByName("EndGameStatBar",0); break;
            case 5: { ENABLE_BARS("MWasted") } PlaySoundByName("EndGameStatBar",0); break;
            case 6: { ENABLE_BARS("Score") } PlaySoundByName("EndGameScore",0); break;
            }
            if(g_seams.map->GetGameType(g_game->net)==3) {
                Player* player=&g_game->players[g_game->localPlayer];
                for(int j=0;j<2;++j) SendPlayerEconomy(player,0,0);
            }
            g_game->deadline=GetTicks()+10;
            ++g_game->statBarStep;
        }
        break;
    }
    case 8:
        HideSoftwareCursor(); UpdateMenu(&g_game->gui); ShowSoftwareCursor(); FlipScreen();
        HideSoftwareCursor(); BlitMenuLayers(&g_game->gui,0,0); ShowSoftwareCursor();
        break;
    }
    FlipScreen();
}

// FUNCTION: 0x4205f0
void InitMemoryCache(void)
{
    g_debrisMemCache.ClearPointers();
    atexit(FreeMemoryCache);
}

// FUNCTION: 0x420610
void __cdecl FreeMemoryCache()
{
    g_debrisMemCache.FreeBuffer();
}
