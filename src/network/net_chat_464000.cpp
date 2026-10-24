struct ChatHudEntryView {
    char unknown_0[0x1e];
    unsigned char flags;
    char unknown_1f[0x29];
};

#include "../game/game.h"

#include "../../link/globals.h"
#include "net_chat.h"

// FUNCTION: 0x464000
void CycleMessageUnits(void)
{
    int i;
    for (i = 0; i < 30; i++)
        ((ChatHudEntryView*)((char*)g_game + 0x1318))[i].flags &= ~0x20;
    if (ScrollToNextMessageUnit() == 0) {
        for (i = 0; i < 30; i++)
            ((ChatHudEntryView*)((char*)g_game + 0x1318))[i].flags &= ~0x10;
        ScrollToNextMessageUnit();
    }
}
