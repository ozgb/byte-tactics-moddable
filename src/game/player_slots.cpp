#pragma pack(push, 1)
#include "../network/player.h"
#include "../network/player_info.h"

#include "game.h"
#pragma pack(pop)

#include "../../interfaces/services.h"
#include "../harness/replay.h"
#include "../../link/globals.h"

#pragma auto_inline(off)
// FUNCTION: 0x44fe40
unsigned char __stdcall FindSlotByDpid(int id)
{
    ReplayImport call(RI_FIND_SLOT_BY_DPID);
    call.Int(id);
    if (id != -1) {
        for (unsigned char i = 0; i < 10; i++) {
            if (GetSlotDpid(i) == id)
                return (unsigned char)call.Result((int)i);
        }
    }
    return (unsigned char)call.Result(10);
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x44ffd0
int __stdcall GetSlotDpid(unsigned char index)
{
    ReplayImport call(RI_GET_SLOT_DPID);
    call.Int((int)index);
    if (index != 10 && g_game->players[index].type != 0)
        return call.Result((int)g_game->players[index].id);
    return call.Result(-1);
}
#pragma auto_inline(on)

// FUNCTION: 0x456850
unsigned char FindHostSlot()
{
    unsigned char i;
    for (i = 0; i < 10; i++) {
        if (g_game->players[i].type != 0 && (g_game->players[i].info->flags_97 & 1))
            return i;
    }
    return 10;
}
