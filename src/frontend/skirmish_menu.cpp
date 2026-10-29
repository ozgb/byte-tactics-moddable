#include "win_types.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct GafEntry;
struct Gaf;
struct Gui;
struct Layer;
struct Cheat;
class Mission;

#pragma pack(push, 1)

struct Backdrop {
    char unknown_0[0xc0];
    Gaf* gaf;
};

#include "../gui/gui.h"

#include "endgame.h"

#include "../gui/gadget.h"
#include "../gui/layer.h"

#include "../game/settings.h"

#include "../network/player.h"
#include "../network/player_info.h"
#include "../../interfaces/seams.h"

#include "../game/game.h"

#pragma pack(pop)

#include "../../link/globals.h"

int __stdcall FindGadgetIndex(Gadget* entries, const char* name, int flag);

// FUNCTION: 0x479660
void RefreshAllyIcons(void)
{
    int i = 0;
    Gadget* entries = g_game->gui.layer->entries;
    for (; i < g_game->playerCount; i++) {
        char name[64];
        sprintf(name, "Allies%d", i);
        int index = FindGadgetIndex(entries, name, 6);
        if (index != -1) {
            Gadget* gadget = &entries[index];
            if (gadget != 0) {
                int team = g_game->options->players[i].allyGroup;
                int count = 0;
                int j;
                for (j = 0; j < g_game->playerCount; j++) {
                    if (g_game->options->players[j].allyGroup == team &&
                        g_game->options->players[j].controller != 0) {
                        count++;
                    }
                }
                switch (count) {
                case 0:
                    gadget->u.hotspot.index = 10;
                    break;
                case 1:
                    gadget->u.hotspot.index = team * 2 + 1;
                    break;
                default:
                    gadget->u.hotspot.index = team * 2;
                    break;
                }
            }
        }
    }
    MarkChanged((Dialog *)&g_game->gui);
}

#pragma auto_inline(off)
// FUNCTION: 0x479760
int AreAllPlayersInOneAllyGroup(void)
{
    int type;
    int i;
    for (i = 0; i < g_game->playerCount; i++) {
        if (g_game->options->players[i].controller != 0 && (type = g_game->options->players[i].allyGroup) != 5)
            break;
    }
    if (i == g_game->playerCount)
        return 0;
    for (i = 0; i < g_game->playerCount; i++) {
        if (g_game->options->players[i].allyGroup != type && g_game->options->players[i].controller != 0)
            return 0;
    }
    return 1;
}
#pragma auto_inline(on)

void __stdcall SetGadgetActiveByName(Gui* menu, char* name, int value);
void __stdcall SetTranslatedTextByName(Gui* menu, char* key, char* text, int flag);
char* __stdcall Translate(char* key);

int __cdecl CountHumanSlots()
{
    int n = 0;
    for (int i = 0; i < g_game->playerCount; i++) {
        if (g_game->options->players[i].controller == 1)
            n++;
    }
    return n;
}

int __stdcall IsColorTaken(int color, int skip)
{
    for (int i = 0; i < g_game->playerCount; i++) {
        if (g_game->options->players[i].color == color && g_game->options->players[i].controller != 0 && i != skip) {
            return 1;
        }
    }
    return 0;
}

int FindFreeColor()
{
    int color = 0;
    do {
        int i;
        for (i = 0; i < g_game->playerCount; i++) {
            if (g_game->options->players[i].color == color)
                break;
        }
        if (i == g_game->playerCount)
            return color;
        color++;
    } while (color < 10);
    return -1;
}

static void NewColour(int playerIndex)
{
    char name[64];
    Gadget* entries = g_game->gui.layer->entries;
    g_game->options->players[playerIndex].color = FindFreeColor();
    sprintf(name, "Color%d", playerIndex);
    int index = FindGadgetIndex(entries, name, 6);
    if (index != -1) {
        Gadget* e = &entries[index];
        if (e != 0) {
            e->u.hotspot.gaf = (GafEntry*)g_game->logos32;
            e->u.hotspot.index = (unsigned short)g_game->options->players[playerIndex].color;
        }
    }
}

// FUNCTION: 0x4797e0
void __stdcall CycleSlotController(int playerIndex)
{
    char buffer[64];

    sprintf(buffer, "Player%d", playerIndex);
    switch (g_game->options->players[playerIndex].controller) {
    case 0:
        g_game->options->players[playerIndex].controller = 2;
        SetTranslatedTextByName(&g_game->gui, buffer, Translate("Computer"), 0);
        break;
    case 1:
        g_game->options->players[playerIndex].controller = 0;
        SetTranslatedTextByName(&g_game->gui, buffer, Translate("Open"), 0);
        break;
    case 2:
        if (CountHumanSlots() == 0) {
            g_game->options->players[playerIndex].controller = 1;
            SetTranslatedTextByName(&g_game->gui, buffer, Translate("Player"), 0);
        } else {
            g_game->options->players[playerIndex].controller = 0;
            SetTranslatedTextByName(&g_game->gui, buffer, Translate("Open"), 0);
        }
        break;
    }

    if (g_game->options->players[playerIndex].controller == 0) {
        sprintf(buffer, "Player%d", playerIndex);
        SetGadgetActiveByName(&g_game->gui, buffer, 1);
        sprintf(buffer, "Side%d", playerIndex);
        SetGadgetActiveByName(&g_game->gui, buffer, 0);
        sprintf(buffer, "Allies%d", playerIndex);
        SetGadgetActiveByName(&g_game->gui, buffer, 0);
        sprintf(buffer, "Metal%d", playerIndex);
        SetGadgetActiveByName(&g_game->gui, buffer, 0);
        sprintf(buffer, "Energy%d", playerIndex);
        SetGadgetActiveByName(&g_game->gui, buffer, 0);
        sprintf(buffer, "Color%d", playerIndex);
        SetGadgetActiveByName(&g_game->gui, buffer, 0);
    } else {
        if (IsColorTaken(g_game->options->players[playerIndex].color, playerIndex))
            NewColour(playerIndex);
        sprintf(buffer, "Player%d", playerIndex);
        SetGadgetActiveByName(&g_game->gui, buffer, 1);
        sprintf(buffer, "Side%d", playerIndex);
        SetGadgetActiveByName(&g_game->gui, buffer, 1);
        sprintf(buffer, "Allies%d", playerIndex);
        SetGadgetActiveByName(&g_game->gui, buffer, 1);
        sprintf(buffer, "Metal%d", playerIndex);
        SetGadgetActiveByName(&g_game->gui, buffer, 1);
        sprintf(buffer, "Energy%d", playerIndex);
        SetGadgetActiveByName(&g_game->gui, buffer, 1);
        sprintf(buffer, "Color%d", playerIndex);
        SetGadgetActiveByName(&g_game->gui, buffer, 1);
    }
    RefreshAllyIcons();
}

#pragma pack(push, 1)
struct Rec1 {
    Header h;
    char unknown_2a[0x2f - 0x2a];
    GafEntry* entry;
    char text[0x103];
    unsigned char stages;
    unsigned char stageIndex;
    short f138;
    char pad_13a;
    unsigned char frame;
    char pad_13c[2];
};

struct Rec2 {
    Header h;
    char unknown_2a[0x33 - 0x2a];
    char text[0x83];
    short count;
    char unknown_b8[0xc8 - 0xb8];
    int flags;
};
#pragma pack(pop)

GafEntry* __stdcall FindGafEntry(Gaf* gaf, const char* name);
int __stdcall GetGafFrame(unsigned short* gaf, int frame);

// FUNCTION: 0x479bf0
void __stdcall BindGadgetAnimSequence(Rec1* obj, char* name)
{
    Backdrop* h = (Backdrop*)g_game->gui.layer->entries;
    obj->entry = 0;
    if (h->gaf) {
        GafEntry* e = FindGafEntry(h->gaf, name);
        if (e) {
            obj->entry = e;
            short* f = (short*)GetGafFrame((unsigned short*)e, obj->frame);
            if (f) {
                obj->h.width = f[0];
                obj->h.height = f[1];
            }
        }
    }
}

int __stdcall AddButtonGadget(void* obj, void* record);
int __stdcall AddHotspotGadget(void* obj, void* record);

// FUNCTION: 0x479c50
void BuildSkirmishPlayerRows(void)
{
    Rec1 rec1;
    Rec2 rec2;
    int step = 200 / g_game->playerCount;
    int y = (0xb4 - (g_game->playerCount - 1) * step) / 2 + 0x4f;
    memset(&rec1, 0, sizeof(rec1));
    memset(&rec2, 0, sizeof(rec2));
    rec1.h.flag = 1;
    rec2.h.flag = 1;
    rec2.flags |= 1;
    int i = 0;
    while (i < g_game->playerCount) {
        rec1.h.y = y;
        rec2.h.y = y;
        sprintf(rec1.h.name, "Player%d", i);
        rec1.h.x = 0x2d;
        rec1.h.width = 0x70;
        rec1.h.height = 0x14;
        rec1.h.attribs = 2;
        rec1.text[0] = 0;
        BindGadgetAnimSequence(&rec1, "skirmname");
        AddButtonGadget(&g_game->gui, &rec1);

        sprintf(rec1.h.name, "Side%d", i);
        rec1.h.x = 0xa3;
        rec1.h.width = 0x2d;
        BindGadgetAnimSequence(&rec1, "SIDEx");
        rec1.frame = 0;
        rec1.stages = 2;
        AddButtonGadget(&g_game->gui, &rec1);

        sprintf(rec2.h.name, "Color%d", i);
        rec2.h.x = 0xd6;
        rec2.h.width = 0x14;
        rec2.h.height = 0x14;
        rec2.text[0] = 0;
        AddHotspotGadget(&g_game->gui, &rec2);

        sprintf(rec2.h.name, "Allies%d", i);
        rec2.h.x = 0xf1;
        rec2.h.width = 0x28;
        rec2.h.height = 0x14;
        strcpy(rec2.text, Translate("Click to select an allegiance symbol."));
        AddHotspotGadget(&g_game->gui, &rec2);

        sprintf(rec1.h.name, "Metal%d", i);
        rec1.h.x = 0x11e;
        rec1.h.width = 0x2d;
        rec1.h.height = 0x14;
        rec1.h.attribs |= 0x10000;
        rec1.stages = 0;
        rec1.f138 = 0;
        rec1.entry = 0;
        BindGadgetAnimSequence(&rec1, "skirmmet");
        strcpy(rec1.text, Translate("Left click to increase metal. Right click to decrease metal."));
        AddButtonGadget(&g_game->gui, &rec1);

        sprintf(rec1.h.name, "Energy%d", i);
        rec1.h.x = 0x151;
        rec1.h.width = 0x2d;
        BindGadgetAnimSequence(&rec1, "skirmmet");
        strcpy(rec1.text, Translate("Left click to increase energy. Right click to decrease energy."));
        AddButtonGadget(&g_game->gui, &rec1);
        y += step;
        ++i;
    }
}

int __stdcall SetButtonStageByName(Gui* menu, char* name, int value);

// FUNCTION: 0x47a0e0
void RefreshSkirmishSetup()
{
    void* teamIcons;
    char buf[0x40];
    char num[0x40];
    int index;

    Gadget* entries = g_game->gui.layer->entries;
    g_game->options->baseGadgetCount = entries[0].u.count;
    BuildSkirmishPlayerRows();

    int empty = 1;
    if (g_game->playerCount > 0) {
        SkirmishPlayer* p = g_game->options->players;
        int n = g_game->playerCount;
        do {
            if (p->controller != 0)
                empty = 0;
            p++;
        } while (--n != 0);
    }
    if (empty != 0) {
        g_game->options->players[0].controller = 1;
        g_game->options->players[1].controller = 2;
    }

    unsigned short* icons =
        (unsigned short*)FindGafEntry(((Backdrop*)entries)->gaf, "TEAMICONSx");
    teamIcons = icons;
    if (icons != 0) {
        int j = 0;
        if (*icons > 0) {
            do {
                short* f = (short*)GetGafFrame(icons, 0);
                f[3] = 0;
                f[2] = 0;
                j++;
            } while (j < (int)*icons);
        }
    }

    {
        int i;
        for (i = 0; i < g_game->playerCount; i++) {
            sprintf(buf, "Player%d", i);
            int type = g_game->options->players[i].controller;
            switch (type) {
            case 2:
                SetTranslatedTextByName(&g_game->gui, buf, Translate("Computer"), 0);
                break;
            case 1:
                SetTranslatedTextByName(&g_game->gui, buf, Translate("Player"), 0);
                break;
            case 0:
                SetTranslatedTextByName(&g_game->gui, buf, "Open", 0);
                SetGadgetActiveByName(&g_game->gui, buf, 1);
                sprintf(buf, "Side%d", i);
                SetGadgetActiveByName(&g_game->gui, buf, 0);
                sprintf(buf, "Allies%d", i);
                SetGadgetActiveByName(&g_game->gui, buf, 0);
                sprintf(buf, "Metal%d", i);
                SetGadgetActiveByName(&g_game->gui, buf, 0);
                sprintf(buf, "Energy%d", i);
                SetGadgetActiveByName(&g_game->gui, buf, 0);
                sprintf(buf, "Color%d", i);
                SetGadgetActiveByName(&g_game->gui, buf, 0);
                break;
            }

            sprintf(buf, "Side%d", i);
            SetButtonStageByName(&g_game->gui, buf, g_game->options->players[i].side);

            sprintf(buf, "Metal%d", i);
            _itoa(g_game->options->players[i].metal, num, 10);
            SetTranslatedTextByName(&g_game->gui, buf, num, 0);

            sprintf(buf, "Energy%d", i);
            _itoa(g_game->options->players[i].energy, num, 10);
            SetTranslatedTextByName(&g_game->gui, buf, num, 0);

            sprintf(buf, "Color%d", i);
            index = FindGadgetIndex(entries, buf, 6);
            if (index != -1) {
                Gadget* gadget = &entries[index];
                if (gadget != 0) {
                    gadget->u.hotspot.gaf = (GafEntry*)g_game->logos32;
                    gadget->u.hotspot.index = g_game->options->players[i].color;
                }
            }

            sprintf(buf, "Allies%d", i);
            index = FindGadgetIndex(entries, buf, 6);
            if (index != -1) {
                Gadget* gadget = &entries[index];
                if (gadget != 0) {
                    gadget->u.hotspot.index = 10;
                    gadget->u.hotspot.gaf = (GafEntry*)teamIcons;
                }
            }
        }
    }

    RefreshAllyIcons();

    index = FindGadgetIndex(entries, "StartLocation", 1);
    {
        Gadget* g = &entries[index];
        if (g_game->options->fixedLocations != 0) {
            g->stageIndex = 0;
            strcpy(g->helpKey, Translate("Commanders are placed at pre-determined locations."));
        } else {
            g->stageIndex = 1;
            strcpy(g->helpKey, Translate("Commanders are randomly placed on the battle field."));
        }
    }

    index = FindGadgetIndex(entries, "CommanderDeath", 1);
    {
        Gadget* g = &entries[index];
        if (g_game->options->commanderDeath != 0) {
            g->stageIndex = 0;
            strcpy(g->helpKey, Translate("Game ends when commander is destroyed."));
        } else {
            g->stageIndex = 1;
            strcpy(g->helpKey, Translate("Game continues after Commander is destroyed."));
        }
    }

    index = FindGadgetIndex(entries, "Mapping", 1);
    {
        Gadget* g = &entries[index];
        if (g_game->options->mapping != 0) {
            g->stageIndex = 0;
            strcpy(g->helpKey, Translate("Terrain is blacked out until explored."));
        } else {
            g->stageIndex = 1;
            strcpy(g->helpKey, Translate("Terrain is visible."));
        }
    }

    index = FindGadgetIndex(entries, "LineOfSight", 1);
    {
        Gadget* g = &entries[index];
        if (g_game->options->lineOfSight == 0) {
            g->stageIndex = 0;
            strcpy(g->helpKey, Translate("All mapped terrain is visible."));
        } else if (g_game->options->losType == 1) {
            g->stageIndex = 1;
            strcpy(g->helpKey, Translate("Terrain elevations affect a unit's view."));
        } else {
            g->stageIndex = 2;
            strcpy(g->helpKey, Translate("Terrain elevations do not affect a unit's view."));
        }
    }

    SetTranslatedTextByName(&g_game->gui, "MapName", g_game->options->mapName, 0);
    MarkChanged((Dialog *)&g_game->gui);
}

// FUNCTION: 0x47a700
int __stdcall FindNextAllySlot(int self, int start)
{
    SkirmishPlayer* players = g_game->options->players;
    int n = g_game->playerCount;
    if (start != n) {
        for (int i = start; i < n; i++) {
            if (players[i].allyGroup == players[self].allyGroup && players[i].controller != 0 && players[i].allyGroup != 5)
                return i;
            if (i == self)
                return i;
        }
    }
    return -1;
}

// FUNCTION: 0x47a760
void ApplySlotsToGamePlayers()
{
    for (int i = 0; i < g_game->playerCount; i++) {
        if (g_game->options->players[i].controller == 1) {
            g_game->players[i].info->color = g_game->options->players[i].color;
            g_game->players[i].info->side = g_game->options->players[i].side;
            g_seams.players->SetupPlayerSlot(i, 1);
            g_game->playerIndex = i;
            g_game->localPlayer = i;
        } else if (g_game->options->players[i].controller == 2) {
            g_game->players[i].info->color = g_game->options->players[i].color;
            g_game->players[i].info->side = g_game->options->players[i].side;
            g_seams.players->SetupPlayerSlot(i, 2);
        } else {
            g_seams.players->SetupPlayerSlot(i, 0);
        }
        if (g_game->options->players[i].controller == 1 || g_game->options->players[i].controller == 2) {
            int j = 0;
            for (;;) {
                SkirmishPlayer* players = g_game->options->players;
                int n = g_game->playerCount;
                int k;
                int ii;
                if (j != n) {
                    for (ii = j; ii < n; ii++) {
                        if (players[ii].allyGroup == players[i].allyGroup && players[ii].controller != 0 && players[ii].allyGroup != 5)
                            goto found;
                        if (ii == i)
                            goto found;
                    }
                }
                k = -1;
                goto done;
            found:
                k = ii;
            done:
                if (k == -1)
                    break;
                g_game->players[i].allied[k] = 1;
                j = k + 1;
            }
        }
    }
}

// FUNCTION: 0x47a8e0
void __cdecl CycleCurrentPlayerSide()
{
    SkirmishOptionsTable* base = g_game->options;
    int* p = &base->players[base->current].side;
    *p = (*p + 1) % g_game->sides;
}

#include "../map/mission.h"

#include "../graphics/object_pool.h"
void __stdcall PlaySoundByName(const char* name, int broadcast);
Gadget* __stdcall FindGadgetChecked(Gadget* entries, char* name);
Gadget* __stdcall FindGadgetChecked_E(Gadget* entries, char* name);
void __stdcall SetControlTextByName(Gui* menu, char* name, char* value);
#include "../gui/dialogs.h"
#include "../util/registry.h"

// FUNCTION: 0x47a910
void __stdcall HandleSkirmishMapClick(Gui* menu)
{
    char buffer[0x100];
    Layer* holder = menu->layer;
    Data* list = (Data*)holder->data;
    Gadget* entries = holder->entries;

    if (menu->hotGadgetIndex == -1) {
        Gadget* pic = FindGadgetChecked_E(g_game->gui.layer->entries, "MAPPIC");
        if (pic->u.list.lines != 0) {
            GameFreeThunk(pic->u.list.lines);
            pic->u.list.lines = 0;
        }
        GameFreeThunk(list->items);
        GameFreeThunk(list);
        return;
    }
    if (!IsCurrentGadgetNamed((Dialog *)menu, "MAPNAMES") && !IsCurrentGadgetNamed((Dialog *)menu, "LOAD")) {
        if (IsCurrentGadgetNamed((Dialog *)menu, "PREVMENU")) {
            PlaySoundByName("Previous", 0);
        } else {
            ClearSelectedGadget(menu);
        }
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)menu, "LOAD")) {
        PlaySoundByName("SmallButton", 0);
    }
    Gadget* g = FindGadgetChecked(entries, "MAPNAMES");
    strncpy(g_game->options->mapName, SkipTextLines(g->u.list.lines, g->u.list.selected), 0x100);
    g_game->net->LoadMissionByName(g_game->options->mapName);
    strncpy(buffer, g_game->options->mapName, 0x100);
    SetControlTextByName(menu, "MapName", buffer);
}

#include "multi.h"

// FUNCTION: 0x47aaa0
void __stdcall UpdateSkirmishMapSelection(Gui* menu, Gadget* unused)
{
    Gadget* g = FindGadgetChecked(menu->layer->entries, "MAPNAMES");
    if (g_game->net->LoadMissionByName(SkipTextLines(g->u.list.lines, g->u.list.selected)) != 0) {
        ShowSelectedMapInfo();
    }
}

#include "../map/map_list.h"
Layer* __stdcall LoadGuiLayer(Gui* menu, const char* name, int flags);
#include "../debug/debug_lib.h"
void __stdcall LoadPictureCached(const char* name, int a, int b, int c);
#include "../gui/file_list.h"
void __stdcall ConfigureListBoxByName(Gui* menu, char* name, char* items, int count, int flag);
void __stdcall SetKeyboardInput(Gui* menu, int value);
void __stdcall SetCursorMode(int value);

// FUNCTION: 0x47aaf0
void OpenSkirmishMapSelector()
{
    int n = LoadMapList(0, 0, 0);
    if (n == 0) {
        OpenMessageBox(&g_game->gui,
                     Translate("There are no skirmish maps to choose from"),
                     0x140, 1, 1);
        return;
    }
    Layer* layer = LoadGuiLayer(&g_game->gui, "SELMAP.GUI", 0x880);
    layer->handler = (void (__stdcall*)(void*))HandleSkirmishMapClick;
    Data* data = (Data*)GameAllocIgnoreTag("SELECT MAP DATA", 0x20);
    layer->data = data;
    LoadPictureCached("DSELECTMAP2", 0, 0, 0);
    LoadMapList((void **)&data->items, 0, 0);
    SortFileList(data->items, 0, 0, n);
    ConfigureListBoxByName(&g_game->gui, "MAPNAMES", data->items, n, 0);
    FindGadgetChecked(layer->entries, "MAPNAMES")->u.list.callback = UpdateSkirmishMapSelection;

    for (int i = 0; i < n; i++) {
        if (strcmp(g_game->options->mapName, SkipTextLines(data->items, i)) == 0) {
            SetListBoxScrollByName((SetListBoxScrollByNameDialog *)&g_game->gui, "MAPNAMES", i);
            break;
        }
    }

    Gadget* g = FindGadgetChecked(g_game->gui.layer->entries, "MAPNAMES");
    if (g_game->net->LoadMissionByName(SkipTextLines(g->u.list.lines, g->u.list.selected)) != 0) {
        ShowSelectedMapInfo();
    }
    SetKeyboardInput(&g_game->gui, 1);
    RenderLayer(&g_game->gui, 0x40);
    SetCursorMode(0x13);
}

// FUNCTION: 0x47acb0
void __stdcall SelectLogosGadget(Gui* obj, void* unused)
{
    Layer* ptr = obj->layer;
    Gadget* val = ptr->entries;
    int result = FindGadgetIndex(val, "LOGOS", 2);
    obj->hotGadgetIndex = result;
}

static inline int color_taken(int me)
{
    int j;
    for (j = 0; j < g_game->playerCount; j++) {
        if (g_game->options->players[j].color == g_game->options->players[me].color &&
            g_game->options->players[j].controller != 0 && j != me)
            return 1;
    }
    return 0;
}

// FUNCTION: 0x47acd0
void __stdcall CyclePlayerColor(int reverse)
{
    char buf[0x40];
    Gadget* entries = g_game->gui.layer->entries;
    int current = g_game->options->current;

    do {
        g_game->options->players[current].color =
            (g_game->options->players[current].color + (reverse ? -1 : 1)) % *(unsigned short *)g_game->logos32;
        if (g_game->options->players[current].color == -1)
            g_game->options->players[current].color = *(unsigned short *)g_game->logos32 - 1;
    } while (color_taken(current));

    sprintf(buf, "Color%d", current);
    int index = FindGadgetIndex(entries, buf, 6);
    if (index != -1) {
        Gadget* gadget = &entries[index];
        if (gadget != 0) {
            gadget->u.hotspot.gaf = (GafEntry*)g_game->logos32;
            gadget->u.hotspot.index = (unsigned short)g_game->options->players[current].color;
        }
    }
    MarkChanged((Dialog *)&g_game->gui);
}

// FUNCTION: 0x47ae30
void __cdecl CycleCurrentPlayerAllyGroup()
{
    SkirmishOptionsTable* base = g_game->options;
    int* p = &base->players[base->current].allyGroup;
    *p = (*p + 1) % 6;
    RefreshAllyIcons();
}

#include "../util/mouse_event.h"

#pragma pack(push, 1)
struct ClickFrame {
    char sA[0xc];
    char sB[0xc];
    MouseEvent ev;
    char bf[0x40];
};
#pragma pack(pop)

#include "../game/cd_check.h"
#include "endgame.h"
#include "../game/settings.h"
void __stdcall UpdateHelpText(Gui* gui);
#include "../gui/keyboard.h"

// FUNCTION: 0x47ae60
void __stdcall HandleSkirmishClick(Gui* menu)
{
    ClickFrame frame;

    Gadget* entries = menu->layer->entries;
    int cmd = menu->hotGadgetIndex;
    if (cmd == -1) {
        return;
    }
    GetGadgetName(entries, frame.bf, cmd);

    int player = atoi(&frame.bf[strlen(frame.bf) - 1]);
    SkirmishOptionsTable* table = g_game->options;
    table->current = player;
    frame.bf[strlen(frame.bf) - 1] = 0;

    if (IsCurrentGadgetNamed((Dialog *)menu, "Start")) {
        PlaySoundByName("BigButton", 0);
        RegisterDataArchives();

        int n = 0;
        int count = g_game->playerCount;
        if (count > 0) {
            SkirmishPlayer* p = g_game->options->players;
            do {
                if (p->controller == 2)
                    n++;
                p++;
            } while (--count);
        }
        g_game->numPlayers = n + 1;

        if (g_game->net->LoadMissionByName(g_game->options->mapName) == 0) {
            OpenMessageBox(&g_game->gui,
                         Translate("The terrain for the selected map does not exist."),
                         0x1e0, 1, 1);
            ClearSelectedGadget(menu);
            return;
        }

        int n2 = g_game->playerCount;
        int c2 = 0;
        if (n2 > 0) {
            SkirmishPlayer* p = g_game->options->players;
            for (int i = n2; i > 0; i--) {
                if (p->controller == 2)
                    c2++;
                p++;
            }
        }
        if (c2 >= 1) {
            int c1 = 0;
            if (n2 > 0) {
                SkirmishPlayer* p = g_game->options->players;
                for (; n2 > 0; n2--) {
                    if (p->controller == 1)
                        c1++;
                    p++;
                }
            }
            if (c1 >= 1) {
                int maxPlayers = g_game->net->CountStartPositions();
                if ((int)g_game->numPlayers > maxPlayers) {
                    OpenMessageBox(&g_game->gui,
                                 Translate("There are too many players enabled for this map"),
                                 0x1e0, 1, 1);
                    ClearSelectedGadget(menu);
                    return;
                }

                if (AreAllPlayersInOneAllyGroup() != 0) {
                    OpenMessageBox(&g_game->gui,
                                 Translate("All players may not be in the same allied group."),
                                 0x1e0, 1, 1);
                    ClearSelectedGadget(menu);
                    return;
                }

                int n3 = g_game->playerCount;
                c2 = 0;
                if (n3 > 0) {
                    SkirmishPlayer* p = g_game->options->players;
                    for (int i = n3; i > 0; i--) {
                        if (p->controller == 2)
                            c2++;
                        p++;
                    }
                }
                c1 = 0;
                if (n3 > 0) {
                    SkirmishPlayer* p = g_game->options->players;
                    for (; n3 > 0; n3--) {
                        if (p->controller == 1)
                            c1++;
                        p++;
                    }
                }
                g_game->numPlayers = c1 + c2;

                ApplySlotsToGamePlayers();
                InitMissionStatus();
                SaveSettings();
                (*(char*)&g_game->frontendNextSubState) = 2;
                SetCursorMode(0x14);
                return;
            }
        }
        OpenMessageBox(&g_game->gui,
                     Translate("There must be at least one player and one computer opponent"),
                     0x1e0, 1, 1);
        ClearSelectedGadget(menu);
        return;
    }

    if (IsCurrentGadgetNamed((Dialog *)menu, "PrevMenu")) {
        PlaySoundByName("Previous", 0);
        SetCursorMode(0x14);
        (*(char*)&g_game->frontendNextSubState) = 3;
        return;
    }

    if (strcmp(frame.bf, "Player") == 0) {
        PlaySoundByName("Skirmish", 0);
        CycleSlotController(player);
    } else if (strcmp(frame.bf, "Side") == 0) {
        PlaySoundByName("Skirmish", 0);
        SkirmishOptionsTable* t = g_game->options;
        int* p = &t->players[t->current].side;
        *p = (*p + 1) % g_game->sides;
    } else if (strcmp(frame.bf, "Allies") == 0) {
        PlaySoundByName("Skirmish", 0);
        SkirmishOptionsTable* t = g_game->options;
        int* p = &t->players[t->current].allyGroup;
        *p = (*p + 1) % 6;
        RefreshAllyIcons();
    } else if (strcmp(frame.bf, "Color") == 0) {
        PlaySoundByName("Skirmish", 0);
        GetCurrentMouseEvent(&frame.ev);
        if (menu->layer->clickMode == 1) {
            CyclePlayerColor(0);
        }
        if (menu->layer->clickMode == 2) {
            CyclePlayerColor(1);
        }
    } else if (strcmp(frame.bf, "Energy") == 0) {
        GetCurrentMouseEvent(&frame.ev);
        if (menu->layer->clickMode == 1) {
            PlaySoundByName("Skirmish", 0);
            SkirmishOptionsTable* t = g_game->options;
            int* p = &t->players[player].energy;
            *p = min(*p + 0x1f4, 0x2710);
            SkirmishOptionsTable* t2 = g_game->options;
            int* q = &t2->players[player].energy;
            if (*q == 0x2bc)
                *q = 0x1f4;
            sprintf(frame.sB, "Energy%d", player);
            _itoa(g_game->options->players[player].energy, frame.sA, 10);
            SetTranslatedTextByName(menu, frame.sB, frame.sA, 10);
        }
        if (menu->layer->clickMode == 2) {
            PlaySoundByName("Skirmish", 0);
            SkirmishOptionsTable* t = g_game->options;
            int* p = &t->players[player].energy;
            *p = max(*p + -0x1f4, 0xc8);
            sprintf(frame.sB, "Energy%d", player);
            _itoa(g_game->options->players[player].energy, frame.sA, 10);
            SetTranslatedTextByName(menu, frame.sB, frame.sA, 10);
        }
    } else if (strcmp(frame.bf, "Metal") == 0) {
        if (menu->layer->clickMode == 1) {
            PlaySoundByName("Skirmish", 0);
            SkirmishOptionsTable* t = g_game->options;
            int* p = &t->players[player].metal;
            *p = min(*p + 0x1f4, 0x2710);
            SkirmishOptionsTable* t2 = g_game->options;
            int* q = &t2->players[player].metal;
            if (*q == 0x2bc)
                *q = 0x1f4;
            sprintf(frame.sA, "Metal%d", player);
            _itoa(g_game->options->players[player].metal, frame.sB, 10);
            SetTranslatedTextByName(menu, frame.sA, frame.sB, 10);
        }
        if (menu->layer->clickMode == 2) {
            PlaySoundByName("Skirmish", 0);
            SkirmishOptionsTable* t = g_game->options;
            int* p = &t->players[player].metal;
            *p = max(*p + -0x1f4, 0xc8);
            sprintf(frame.sA, "Metal%d", player);
            _itoa(g_game->options->players[player].metal, frame.sB, 10);
            SetTranslatedTextByName(menu, frame.sA, frame.sB, 10);
        }
    } else if (IsCurrentGadgetNamed((Dialog *)menu, "CommanderDeath")) {
        PlaySoundByName("Skirmish", 0);
        SkirmishOptionsTable* t = g_game->options;
        t->commanderDeath ^= 1;
        int index = FindGadgetIndex(entries, "CommanderDeath", 1);
        Gadget* e = &entries[index];
        if (g_game->options->commanderDeath != 0)
            strcpy(e->helpKey, Translate("Game ends when commander is destroyed."));
        else
            strcpy(e->helpKey, Translate("Game continues after Commander is destroyed."));
        UpdateHelpText(&g_game->gui);
    } else if (IsCurrentGadgetNamed((Dialog *)menu, "StartLocation")) {
        PlaySoundByName("Skirmish", 0);
        SkirmishOptionsTable* t = g_game->options;
        t->fixedLocations ^= 1;
        int index = FindGadgetIndex(entries, "StartLocation", 1);
        Gadget* e = &entries[index];
        if (g_game->options->fixedLocations != 0)
            strcpy(e->helpKey, Translate("Commanders are placed at pre-determined locations."));
        else
            strcpy(e->helpKey, Translate("Commanders are randomly placed on the battle field."));
        UpdateHelpText(&g_game->gui);
    } else if (IsCurrentGadgetNamed((Dialog *)menu, "Mapping")) {
        PlaySoundByName("Skirmish", 0);
        SkirmishOptionsTable* t = g_game->options;
        t->mapping ^= 1;
        int index = FindGadgetIndex(entries, "Mapping", 1);
        Gadget* e = &entries[index];
        if (g_game->options->mapping != 0)
            strcpy(e->helpKey, Translate("Terrain is blacked out until explored."));
        else
            strcpy(e->helpKey, Translate("Terrain is visible."));
        UpdateHelpText(&g_game->gui);
    } else if (IsCurrentGadgetNamed((Dialog *)menu, "LineOfSight")) {
        PlaySoundByName("Skirmish", 0);
        int index = FindGadgetIndex(entries, "LineOfSight", 1);
        Gadget* e = &entries[index];
        SkirmishOptionsTable* t = g_game->options;
        if (t->lineOfSight == 0) {
            t->lineOfSight = 1;
            g_game->options->losType = 1;
            strcpy(e->helpKey, Translate("Terrain elevations affect a unit's view."));
        } else if (t->losType == 1) {
            t->losType = 0;
            strcpy(e->helpKey, Translate("Terrain elevations do not affect a unit's view."));
        } else {
            t->lineOfSight = 0;
            g_game->options->losType = 1;
            strcpy(e->helpKey, Translate("All mapped terrain is visible."));
        }
        UpdateHelpText(&g_game->gui);
    } else if (IsCurrentGadgetNamed((Dialog *)menu, "SelectMap")) {
        PlaySoundByName("Skirmish", 0);
        SetCursorMode(0x14);
        OpenSkirmishMapSelector();
    } else if (IsCurrentGadgetNamed((Dialog *)menu, "Difficulty")) {
        PlaySoundByName("SKirmish", 0);
        int d = g_game->difficulty;
        if (d == 0) {
            g_game->options->difficulty = 1;
            g_game->difficulty = 1;
        } else if (d == 1) {
            g_game->options->difficulty = 2;
            g_game->difficulty = 2;
        } else if (d == 2) {
            g_game->options->difficulty = 0;
            g_game->difficulty = 0;
        }
    }

    ClearSelectedGadget(menu);
}

#pragma pack(push, 1)
struct CheatText {
    char unknown_0[4];
    Gadget* target;
};

struct Cheat {
    char unknown_0[0x18];
    CheatText* text;
};
#pragma pack(pop)


#include "../game/settings_campaign.h"
#include "../game/settings.h"
void __stdcall SelectAdjacentGadget(Cheat*, int);

// FUNCTION: 0x47b9f0
void __stdcall HandleSkirmishCheatText(Cheat* cheat)
{
    int zero = 0;
    int code = 0;
    if (strncmp((char*)cheat->text + 0x34, g_cheatFourPlayers, 3) == 0)
        code = 4;
    else if (strncmp((char*)cheat->text + 0x35, g_cheatFivePlayers, 2) == 0)
        code = 5;
    else if (strncmp((char*)cheat->text + 0x34, g_cheatSixPlayers, 3) == 0)
        code = 6;
    else if (strncmp((char*)cheat->text + 0x33, g_cheatSevenPlayers, 4) == 0)
        code = 7;
    else if (strncmp((char*)cheat->text + 0x32, g_cheatEightPlayers, 5) == 0)
        code = 8;
    else if (strncmp((char*)cheat->text + 0x33, g_cheatThreePlayers, 4) == 0)
        code = 3;
    else if (strncmp((char*)cheat->text + 0x34, g_cheatNinePlayers, 3) == 0)
        code = 9;
    else if (strncmp((char*)cheat->text + 0x35, g_cheatTenPlayers, 2) == 0)
        code = 10;

    if (code != zero) {
        if (code == 3 || code == 4 || code == 8 || code == 9 || code == 10) {
            for (int i = 0; i < 0xf; i++)
                ((char*)cheat->text)[i + 0x28] = (char)zero;
        }
        SaveSettings();
        g_game->playerCount = code;
        SaveNumSkirmishPlayers();
        LoadSettings();
        cheat->text->target->u.count = g_game->options->baseGadgetCount;
        RefreshSkirmishSetup();
        PlaySoundByName(g_skirmishCheatSoundName, 0);
        SelectAdjacentGadget(cheat, 1);
        MarkChanged((Dialog *)&g_game->gui);
    }
}


#include "frontend.h"
Gadget* __stdcall FindGadgetOrNull(Gadget* entries, char* name);
void __stdcall SetGadgetStatusByName(Gui* menu, char* name, int value);

// FUNCTION: 0x47bbb0
void OpenSkirmishMenu(void)
{
    Gadget* difficulty;
    Layer* dialog;

    BlankScreen();
    dialog = LoadGuiLayer(&g_game->gui, g_skirmishGuiName, 0);
    dialog->handler = (void (__stdcall*)(void*))HandleSkirmishClick;
    dialog->data = (Data*)g_game;
    LoadPictureCached(g_skirmishSetupPictureName, 0, 0, 0);

    g_game->difficulty = g_game->options->difficulty;
    difficulty = FindGadgetOrNull(g_game->gui.layer->entries, g_difficultyKey);
    if (g_game->difficulty == 0) {
        difficulty->stageIndex = 0;
        SetGadgetStatusByName(&g_game->gui, g_easyGadgetName, 1);
    }
    if (g_game->difficulty == 1) {
        difficulty->stageIndex = 1;
        SetGadgetStatusByName(&g_game->gui, g_mediumGadgetName, 1);
    }
    if (g_game->difficulty == 2) {
        difficulty->stageIndex = 2;
        SetGadgetStatusByName(&g_game->gui, g_hardGadgetName, 1);
    }
    MarkChanged((Dialog *)&g_game->gui);

    if (!g_game->net->LoadMissionByName(g_game->options->mapName)) {
        g_game->net->RefreshMapList(0);
        strncpy(g_game->options->mapName, g_game->net->GetMissionName(), 0x100);
    }

    RefreshSkirmishSetup();
    g_game->gui.layer->textHandler = (void (__stdcall*)(void*))HandleSkirmishCheatText;
    SetKeyboardInput(&g_game->gui, 1);
    RenderLayer(&g_game->gui, 0x40);
    SetCursorMode(0x13);
}

void __stdcall AddMessage(char* text, int key, int value, unsigned char last);

#include "../harness/harness.h"

// FUNCTION: 0x47bd70
void __stdcall AnnounceForcesDestroyed(Player* player)
{
    char buf[200];
    const char* side = "Core";
    if (!player->info->side)
        side = "Arm";
    sprintf(buf, "%s %s", side, Translate(g_forcesDestroyedTexts[(unsigned int)HarnessViewRand() % 3]));
    AddMessage(buf, 4, 0, player->index);
}
