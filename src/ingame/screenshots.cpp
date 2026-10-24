#include <stdio.h>

#include "../map/mission.h"
#include "../../interfaces/seams.h"

struct Input_00499890 {
    int fields[6];
};

struct Ctx_00499890 {
    char unknown_0[0xf1];
    unsigned char flags;
};

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

#include "../../link/globals.h"

#include "../gui/keyboard.h"
#include "../graphics/display.h"
#include "../game/game_state.h"
#include "../files/files_api.h"
int __stdcall GetTicks();
#include "../gui/mouse.h"
void __stdcall UpdateMenu(void*);
#include "dialogs_493340.h"
#include "../sound/sound.h"
void __stdcall PopMouseEvent(int*);
#include "../graphics/pcx.h"

// FUNCTION: 0x499890
void MainFrameTick()
{
    Input_00499890 first;
    Input_00499890 second;
    char path[0x100];
    int key = PeekKey();

    if (key == 0x7e) {
        if (g_game->bits_37f2f_2.screenBitB) {
            PopKey();
            ToggleFullScreen();
        }
    }
    if ((g_game->flags_37ebe & 1) && key == 0xe3) {
        PopKey();
        PopUntilNamedLayout(0);
        g_game->flags_37ebe &= 0xfffe;
        if (g_seams.map->GetGameType(g_game->net) != 3)
            g_game->otherFlags &= 0xfffe;
    }
    if (key == 0xd6) {
        PopKey();
        MakeDirectoryPath(g_game->path);
        sprintf(path, g_screenshotDirFormat, g_game->path);
        MakeDirectoryPath(path);
        SaveScreenshot(path, g_screenshotPrefix);
        (*(int*)&g_game->lastTick) = GetTicks();
    }
    PeekMouseEvent((Event_4c2d60 *)&first);
    UpdateMenu(g_game->menu_519);
    PeekMouseEvent((Event_4c2d60 *)&second);
    UpdateCdCategoryByActivity();
    PlayNextSpeech();
    if (first.fields[4] == second.fields[4])
        PopMouseEvent((*(Input_00499890*)&g_game->view).fields);
    else if (first.fields[4] == 0x205 || first.fields[4] == 0x202)
        (*(Input_00499890*)&g_game->view) = first;
    else
        (*(Input_00499890*)&g_game->view) = second;
    UpdateTimers();
    if (!(g_game->unknown_c->flags & 8))
        ((void (__cdecl*)())g_game->handler)();
}
