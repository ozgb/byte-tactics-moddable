#pragma pack(push, 1)
#include "../network/player.h"
#include "../network/player_info.h"

#include "game.h"
#pragma pack(pop)

#include "../../interfaces/services.h"
#include "../../link/globals.h"

#pragma auto_inline(off)
// FUNCTION: 0x44fe40
unsigned char __stdcall FindSlotByDpid(int id)
{
    if (id != -1) {
        for (unsigned char i = 0; i < 10; i++) {
            if (GetSlotDpid(i) == id)
                return i;
        }
    }
    return 10;
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x44ffd0
int __stdcall GetSlotDpid(unsigned char index)
{
    if (index != 10 && g_game->players[index].type != 0)
        return g_game->players[index].id;
    return -1;
}
#pragma auto_inline(on)
