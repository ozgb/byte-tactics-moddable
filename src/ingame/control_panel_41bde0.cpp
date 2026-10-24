#include <stdlib.h>
#include "control_panel_41bde0.h"

#pragma pack(push, 1)
struct Sub_0041bde0 {
    char unknown_0[0x22e];
    unsigned char pageCount;
};

#include "../units/unit.h"

#include "../game/game.h"
#pragma pack(pop)

#include "../../link/globals.h"
void __stdcall PlaySoundByName(char* name, int param);

// FUNCTION: 0x41bde0
void __stdcall StepBuildMenuPage(int toggle)
{
    unsigned short index = g_game->unitIndex;
    if (index != 0) {
        Unit* unit = &g_game->units[index];
        if (unit->unitDefIndex == 0)
            unit = 0;
        if (unit != 0) {
            if (toggle != 0) {
                if (!unit->menu.menuOpen) {
                    unit->menu.menuOpen = 1;
                    unit->menu.page = 1;
                } else if (unit->menu.page == ((Sub_0041bde0*)unit->def.get())->pageCount - 1) {
                    unit->menu.menuOpen = 0;
                } else {
                    unit->menu.page++;
                }
            } else {
                if (unit->menu.page == ((Sub_0041bde0*)unit->def.get())->pageCount - 1)
                    unit->menu.page = 1;
                else
                    unit->menu.page++;
                unit->menu.menuOpen = 1;
            }
            g_game->ordersPanelFlags |= 0x10;
        }
    }
    PlaySoundByName("nextbuildmenu", 0);
}
