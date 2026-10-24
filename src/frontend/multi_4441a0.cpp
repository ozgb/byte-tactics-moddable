#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../interfaces/host.h"

#include "../gui/gadget.h"

#pragma pack(push, 1)
struct Holder_004441a0 {
    int unknown_0;
    Gadget* entries;
};

struct Menu_004441a0 {
    char unknown_0[0x18];
    Holder_004441a0* holder;
    char unknown_1c[0x60 - 0x1c];
    int selected;
};

struct Conn_004441a0 {
    void* data;
    int size;
};

struct Game_004441a0 {
    char unknown_0[0x2a4b];
    void* descriptions;
    char unknown_2a4f[0x2a9f - 0x2a4f];
    void* guids;
    Conn_004441a0* conns;
    char unknown_2aa7[0x2bc0 - 0x2aa7];
    char frontendSubstateRequest;
};
#pragma pack(pop)

#include "../network/link_info.h"

extern Game_004441a0* g_game;
// GLOBAL: 0x5127c8
extern LinkInfo g_linkInfo[];

#include "../graphics/object_pool.h"
#include "../gui/gui.h"
Gadget* __stdcall FindGadgetChecked(void* entries, char* name);
int __stdcall FindGadgetIndex(void* entries, const char* name, int flag);
#include "multi_443ff0.h"
void __stdcall PlaySoundByName(char* name, int flag);
void __stdcall SetCursorMode(int value);
#include "../network/online_45b250.h"
#include "../network/online.h"
char* __stdcall Translate(char* text);
#include "../gui/dialogs.h"
#include "options.h"

// FUNCTION: 0x4441a0
void __stdcall HandleSelectProviderClick(Menu_004441a0* menu)
{
    Gadget* entries = menu->holder->entries;
    int i;
    int id;
    int cur;
    if (menu->selected == -1) {
        if (g_game->guids != 0) {
            GameFreeThunk(g_game->guids);
            for (i = 0; i < 10; i++) {
                if (g_game->conns[i].data != 0) {
                    host_net_free(g_game->conns[i].data);
                    g_game->conns[i].data = 0;
                }
            }
            GameFreeThunk(g_game->conns);
            GameFreeThunk(g_game->descriptions);
            g_game->guids = 0;
            g_game->descriptions = 0;
        }
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)menu, "DPLAY") || IsCurrentGadgetNamed((Dialog *)menu, "SELECT")) {
        SelectConnection(FindGadgetChecked(entries, "DPLAY")->u.list.selected);
        g_game->frontendSubstateRequest = 2;
        PlaySoundByName("BigButton", 0);
        return;
    }
    if (_strnicmp(entries[menu->selected].name, "SERVICE", 7) == 0) {
        PlaySoundByName("BigButton", 0);
        int link = atoi(entries[menu->selected].name + 7);
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
    if (FindGadgetIndex(entries, "PREVMENU", 0xe) == menu->selected) {
        g_game->frontendSubstateRequest = 3;
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
