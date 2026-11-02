#include <stdio.h>
#include <stdlib.h>

#pragma pack(push, 1)
#include "../network/player.h"
#include "../network/player_info.h"
#include "../../interfaces/seams.h"

#include "game.h"
#pragma pack(pop)

#include "../../link/globals.h"
#include "share_commands.h"
#include "../util/commands.h"
#include "../state/sim_effects.h"

void __stdcall AddMessage(char* text, int key, int value, int last);
#include "../network/net_game.h"

// FUNCTION: 0x418cd0
void __stdcall CmdShareMetal(int unused)
{
    if (g_game->netGameFlags & 1) {
        g_game->players[g_game->localPlayer].info->shareMetal =
            !g_game->players[g_game->localPlayer].info->shareMetal;
        RecordMessage(SIM_MESSAGE_SHARE_METAL_TOGGLED, (g_game->players[g_game->localPlayer].info->shareMetal != 0));
        g_seams.network->BroadcastPlayerInfo();
    }
}

// FUNCTION: 0x418d90
void __stdcall CmdShareEnergy(int unused)
{
    if (g_game->netGameFlags & 1) {
        PlayerInfo* data = g_game->players[g_game->localPlayer].info;
        data->flags_97_wide = (data->flags_97_wide & ~4) | (~data->flags_97_wide & 4);
        RecordMessage(SIM_MESSAGE_SHARE_ENERGY_TOGGLED, (g_game->players[g_game->localPlayer].info->flags_97_wide & 4));
        g_seams.network->BroadcastPlayerInfo();
    }
}

// FUNCTION: 0x418e50
void __stdcall CmdShareMapping(int unused)
{
    if (g_game->netGameFlags & 1) {
        PlayerInfo* data = g_game->players[g_game->localPlayer].info;
        data->flags_97_wide = (data->flags_97_wide & ~0x20) | (~data->flags_97_wide & 0x20);
        RecordMessage(SIM_MESSAGE_SHARE_MAPPING_TOGGLED, (g_game->players[g_game->localPlayer].info->flags_97_wide & 0x20));
        g_seams.network->BroadcastPlayerInfo();
    }
}

// FUNCTION: 0x418f10
void __stdcall ToggleShareLos(int unused)
{
    if (g_game->netGameFlags & 1) {
        g_game->players[g_game->localPlayer].info->shareLOS =
            !g_game->players[g_game->localPlayer].info->shareLOS;
        RecordMessage(SIM_MESSAGE_SHARE_LOS_TOGGLED, (g_game->players[g_game->localPlayer].info->shareLOS != 0));
        g_seams.network->BroadcastPlayerInfo();
    }
}

// FUNCTION: 0x418fd0
void __stdcall CmdShareRadar(int unused)
{
    if (g_game->netGameFlags & 1) {
        g_game->players[g_game->localPlayer].info->shareRadar =
            !g_game->players[g_game->localPlayer].info->shareRadar;
        RecordMessage(SIM_MESSAGE_SHARE_RADAR_TOGGLED, (g_game->players[g_game->localPlayer].info->flags_97_wide & 0x40));
        g_seams.network->BroadcastPlayerInfo();
    }
}

// FUNCTION: 0x419090
void __stdcall CmdShareAll(int unused)
{
    if (g_game->netGameFlags & 1) {
        CmdShareMetal(unused);
        CmdShareEnergy(unused);
        g_seams.los_radar->CmdShareMapping(unused);
        g_seams.los_radar->CmdShareRadar(unused);
    }
}

// FUNCTION: 0x419340
void __stdcall CmdSetShareMetal(CommandArgs* args)
{
    if (g_game->netGameFlags & 1) {
        Player* p = &g_game->players[g_game->localPlayer];
        p->shareMetal = __min(p->metalCapacity, (float)args->GetIntArg(1, 0));
        RecordMessage(SIM_MESSAGE_SHARE_METAL_ABOVE, args->GetIntArg(1, 0));
    }
}

// FUNCTION: 0x419400
void __stdcall CmdSetShareEnergy(CommandArgs* args)
{
    if (g_game->netGameFlags & 1) {
        Player* p = &g_game->players[g_game->localPlayer];
        p->shareEnergy = __min(p->energyCapacity, (float)args->GetIntArg(1, 0));
        RecordMessage(SIM_MESSAGE_SHARE_ENERGY_ABOVE, args->GetIntArg(1, 0));
    }
}

// FUNCTION: 0x4194c0
void __stdcall CmdShowRanges(int unused)
{
    int* ptr = &g_game->showRanges;
    *ptr ^= 1;
}

// FUNCTION: 0x4194d0
void __stdcall CmdCompression(int unused)
{
    char buf[256];
    if (g_game->netGameFlags & 1) {
        g_game->compressionDisabled = (g_game->compressionDisabled == 0);
        sprintf(buf, "Ok.  Outgoing packet compression turned %s",
                g_game->compressionDisabled ? "OFF" : "ON");
        AddMessage(buf, 2, 0, 10);
    }
}

// FUNCTION: 0x419540
void __stdcall CmdBPS(int unused)
{
    g_game->showBps ^= 1;
}

// FUNCTION: 0x419550
void __stdcall CmdSFX(int arg1)
{
    g_fxEventPoolBlocked ^= 1;
}
