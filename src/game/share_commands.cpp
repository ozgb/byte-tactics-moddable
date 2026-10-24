#include <stdio.h>
#include <stdlib.h>

#pragma pack(push, 1)
#include "../network/player.h"
#include "../network/player_info.h"
#include "../../interfaces/seams.h"

#include "game.h"
#pragma pack(pop)

extern Game* g_game;

class CommandArgs {
public:
    int GetIntArg(int index, int fallback);
};

void __stdcall AddMessage(char* text, int key, int value, int last);
void BroadcastPlayerInfo();

extern char g_fxEventPoolBlocked;

// FUNCTION: 0x418cd0
void __stdcall CmdShareMetal(int unused)
{
    char buf[256];
    if (g_game->netGameFlags & 1) {
        g_game->players[g_game->localPlayer].info->shareMetal =
            !g_game->players[g_game->localPlayer].info->shareMetal;
        sprintf(buf, "Toggled ShareMetal to: %s",
                (g_game->players[g_game->localPlayer].info->shareMetal != 0)
                    ? "ON" : "OFF");
        AddMessage(buf, 2, 0, 10);
        BroadcastPlayerInfo();
    }
}

// FUNCTION: 0x418d90
void __stdcall CmdShareEnergy(int unused)
{
    char buf[256];
    if (g_game->netGameFlags & 1) {
        PlayerInfo* data = g_game->players[g_game->localPlayer].info;
        data->flags_97_wide = (data->flags_97_wide & ~4) | (~data->flags_97_wide & 4);
        sprintf(buf, "Toggled ShareEnergy to: %s",
                (g_game->players[g_game->localPlayer].info->flags_97_wide & 4)
                    ? "ON" : "OFF");
        AddMessage(buf, 2, 0, 10);
        BroadcastPlayerInfo();
    }
}

// FUNCTION: 0x418e50
void __stdcall CmdShareMapping(int unused)
{
    char buf[256];
    if (g_game->netGameFlags & 1) {
        PlayerInfo* data = g_game->players[g_game->localPlayer].info;
        data->flags_97_wide = (data->flags_97_wide & ~0x20) | (~data->flags_97_wide & 0x20);
        sprintf(buf, "Toggled ShareMapping to: %s",
                (g_game->players[g_game->localPlayer].info->flags_97_wide & 0x20)
                    ? "ON" : "OFF");
        AddMessage(buf, 2, 0, 10);
        BroadcastPlayerInfo();
    }
}

// FUNCTION: 0x418f10
void __stdcall ToggleShareLos(int unused)
{
    char buf[256];
    if (g_game->netGameFlags & 1) {
        g_game->players[g_game->localPlayer].info->shareLOS =
            !g_game->players[g_game->localPlayer].info->shareLOS;
        sprintf(buf, "Toggled ShareLOS to: %s",
                (g_game->players[g_game->localPlayer].info->shareLOS != 0)
                    ? "ON" : "OFF");
        AddMessage(buf, 2, 0, 10);
        BroadcastPlayerInfo();
    }
}

// FUNCTION: 0x418fd0
void __stdcall CmdShareRadar(int unused)
{
    char buf[256];
    if (g_game->netGameFlags & 1) {
        g_game->players[g_game->localPlayer].info->shareRadar =
            !g_game->players[g_game->localPlayer].info->shareRadar;
        sprintf(buf, "Toggled ShareRadar to: %s",
                (g_game->players[g_game->localPlayer].info->flags_97_wide & 0x40)
                    ? "ON" : "OFF");
        AddMessage(buf, 2, 0, 10);
        BroadcastPlayerInfo();
    }
}

static inline void ShareMetal(int unused)
{
    char buf[256];
    if (g_game->netGameFlags & 1) {
        PlayerInfo* data = g_game->players[g_game->localPlayer].info;
        data->flags_97_wide = (data->flags_97_wide & ~2) | (~data->flags_97_wide & 2);
        sprintf(buf, "Toggled ShareMetal to: %s",
                (g_game->players[g_game->localPlayer].info->flags_97_wide & 2)
                    ? "ON" : "OFF");
        AddMessage(buf, 2, 0, 10);
        BroadcastPlayerInfo();
    }
}

static inline void ShareEnergy(int unused)
{
    char buf[256];
    if (g_game->netGameFlags & 1) {
        PlayerInfo* data = g_game->players[g_game->localPlayer].info;
        data->flags_97_wide = (data->flags_97_wide & ~4) | (~data->flags_97_wide & 4);
        sprintf(buf, "Toggled ShareEnergy to: %s",
                (g_game->players[g_game->localPlayer].info->flags_97_wide & 4)
                    ? "ON" : "OFF");
        AddMessage(buf, 2, 0, 10);
        BroadcastPlayerInfo();
    }
}

// FUNCTION: 0x419090
void __stdcall CmdShareAll(int unused)
{
    if (g_game->netGameFlags & 1) {
        ShareMetal(unused);
        ShareEnergy(unused);
        g_seams.los_radar->CmdShareMapping(unused);
        g_seams.los_radar->CmdShareRadar(unused);
    }
}

// FUNCTION: 0x419340
void __stdcall CmdSetShareMetal(CommandArgs* args)
{
    char buf[256];
    if (g_game->netGameFlags & 1) {
        Player* p = &g_game->players[g_game->localPlayer];
        p->shareMetal = __min(p->metalCapacity, (float)args->GetIntArg(1, 0));
        sprintf(buf, "OK.  Will share metal if above %d", args->GetIntArg(1, 0));
        AddMessage(buf, 2, 0, 10);
    }
}

// FUNCTION: 0x419400
void __stdcall CmdSetShareEnergy(CommandArgs* args)
{
    char buf[256];
    if (g_game->netGameFlags & 1) {
        Player* p = &g_game->players[g_game->localPlayer];
        p->shareEnergy = __min(p->energyCapacity, (float)args->GetIntArg(1, 0));
        sprintf(buf, "OK.  Will share energy if above %d", args->GetIntArg(1, 0));
        AddMessage(buf, 2, 0, 10);
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
