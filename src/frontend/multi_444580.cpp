#include <stdio.h>
#include <string.h>
#include "../../interfaces/host.h"

#pragma pack(push, 1)
struct Entry_00444580 {
    short unknown_0;
    char name[0x13];
    short y;
    char unknown_17[0x19 - 0x17];
    short height;
    int attribs;
    int colours;
    char unknown_23[0x29 - 0x23];
    char active;
    char unknown_2a[0xb6 - 0x2a];
    short count;
    char unknown_b8[0xcc - 0xb8];
    char choice[0x15b - 0xcc];
};

struct Layer {
    int unknown_0;
    Entry_00444580* entries;
};

#include "../gui/gui.h"

struct Gadget_00444580 {
    int unknown_0;
    Entry_00444580* entries;
    void (__stdcall* handler)(void*);
    int userData;
};

struct Conn_00444580 {
    void* data;
    int size;
};

#include "../map/mission.h"

struct Game_00444580 {
    char unknown_0[0x14];
    char net[0x4f9 - 0x14];
    int count;
    char unknown_4fd[0x519 - 0x4fd];
    Gui gui;
    char unknown_120f[0x2a4b - 0x120f];
    void* descriptions;
    char unknown_2a4f[0x2a9f - 0x2a4f];
    void* guids;
    Conn_00444580* conns;
    char unknown_2aa7[0x143a7 - 0x2aa7];
    unsigned char paletteRgba[0x391e9 - 0x143a7];
    Ref<Mission> mapInfo;
};
#pragma pack(pop)

#include "../network/link_info.h"

extern Game_00444580* g_game;
// GLOBAL: 0x5127c8
extern LinkInfo g_linkInfo[];
extern char DAT_004fcfb8[];

#include "../graphics/palette.h"
void __stdcall RemapPaletteToClosestIndices(void* menu, void* palette, void* copy);
Gadget_00444580* __stdcall LoadGuiLayer(void* menu, char* name, int size);
void __stdcall HandleSelectProviderClick(void* menu);
void __stdcall LoadPictureCached(const char* name, int a, int b, int c);
#include "../debug/debug_lib.h"
int __stdcall FindGadgetIndex(void* entries, const char* name, int flag);
#include "../network/online_45b490.h"
void __stdcall SetTranslatedText(void* menu, int index, int source, int value);
void __stdcall ConfigureListBoxByName(void* menu, char* name, void* items, int count, int flag);
#include "frontend.h"
void __stdcall SetKeyboardInput(void* menu, int value);

static int __stdcall CloneServiceSlot(Entry_00444580* entries, int from, short y, int source, char* name)
{
    int index = ++entries[0].count;
    entries[index] = entries[from];
    SetTranslatedText(&g_game->gui, index, source, 0);
    strcpy(entries[index].name, name);
    entries[index].y = y;
    entries[index].active = 1;
    entries[index].colours = 0;
    return index;
}

// FUNCTION: 0x444580
void FillProviderList()
{
    if (g_game->gui.layer != 0 && strcmp(g_game->gui.layer->entries->name, "SELPROV.GUI") == 0)
        CloseTopScreen(&g_game->gui);
    SetPaletteColors(g_game->paletteRgba, 0, 0x100);
    RemapPaletteToClosestIndices(&g_game->gui, g_game->paletteRgba, g_game->gui.palette);
    Gadget_00444580* menu = LoadGuiLayer(&g_game->gui, "SELPROV.GUI", 0x80);
    menu->handler = HandleSelectProviderClick;
    menu->userData = (int)g_game;
    LoadPictureCached("selconnect2", 1, 0, 0);
    g_game->descriptions = GameAllocIgnoreTag("PROVIDER DESCRIPTIONS", 0x500);
    g_game->guids = GameAllocIgnoreTag("PROVIDER GUIDS", 0xa0);
    g_game->conns = (Conn_00444580*)GameAllocIgnoreTag("DPLAY CONNECTIONS", 0x50);
    if (g_game->conns != 0)
        memset(g_game->conns, 0, 0x50);
    g_game->mapInfo->RefreshMapList(1);
    host_net_uninit((host_net*)g_game->net);
    host_net_get_connections((host_net*)g_game->net, g_game->guids, g_game->conns, g_game->descriptions, (const host_guid*)DAT_004fcfb8);
    Entry_00444580* entries = menu->entries;
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
    strcpy(entries->choice, "SELECT");
    ConfigureListBoxByName(&g_game->gui, "DPLAY", g_game->descriptions, g_game->count, 0);
    OrLabelAttribs();
    SetKeyboardInput(&g_game->gui, 1);
    RenderLayer(&g_game->gui, 0x40);
}
