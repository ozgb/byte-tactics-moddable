#include "player.h"

#pragma pack(push, 1)
#include "player_info.h"

#include "../game/game.h"

struct Msg_00450530 {
    unsigned char type;
    unsigned char flag;
    int id;
    int arg;
};
#pragma pack(pop)

extern Game* g_game;

int __stdcall GetSlotDpid(unsigned char index);
int __stdcall SendPacketToPlayer(int from, int to, void* packet, int size);
unsigned char FindHostSlot();

static inline int IsPlaying_00450530(Player* player)
{
    if (player->active == 0)
        return 0;
    if (player->type == 1 || player->type == 2)
        return 1;
    return 0;
}

static inline int GetPlayerId_00450530(unsigned char i)
{
    if (i != 10 && g_game->players[i].type != 0)
        return g_game->players[i].id;
    return -1;
}

static inline unsigned char FindPlayer_00450530()
{
    for (unsigned char i = 0; i < 10; i++) {
        if (g_game->players[i].type != 0 && (g_game->players[i].info->flags_97 & 1))
            return i;
    }
    return 10;
}

static inline int FindFrom_00450530()
{
    for (int k = 0; k < 10; k++) {
        if (IsPlaying_00450530(&g_game->players[k]))
            return g_game->players[k].id;
    }
    return -1;
}

static inline int FindTo_00450530()
{
    for (int j = 0; j < 10; j++) {
        if (g_game->players[j].info->flags_97 & 1)
            return GetSlotDpid(j);
    }
    return -1;
}

static inline int FindToB_00450530()
{
    for (int j = 0; j < 10; j++) {
        if (g_game->players[j].info->flags_97 & 1)
            return GetPlayerId_00450530(j);
    }
    return -1;
}

// FUNCTION: 0x450530
void SendLobbySyncRequests()
{
    Msg_00450530 msg;
    if (g_game->mode_391f1 == 6)
        return;
    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (p->active != 0
            && (p->type == 1 || p->type == 2 || p->type == 3)
            && p->index != 10
            && p->lobbyDataSynced == 0) {
            if (IsPlaying_00450530(p) && p->type == 1) {
                Player* q = &g_game->players[FindHostSlot()];
                if (IsPlaying_00450530(q)) {
                    p->lobbyDataSynced = 1;
                    continue;
                }
                msg.type = 0x21;
                msg.flag = 1;
                msg.id = p->id;
                msg.arg = -1;
                if (FindHostSlot() == 10)
                    continue;
                SendPacketToPlayer(FindFrom_00450530(), FindTo_00450530(), &msg, 10);
            }
            else if (IsPlaying_00450530(p) && p->type == 2) {
                Player* q = &g_game->players[FindHostSlot()];
                if (IsPlaying_00450530(q)) {
                    p->lobbyDataSynced = 1;
                    continue;
                }
                msg.type = 0x21;
                msg.flag = 1;
                msg.id = p->id;
                msg.arg = g_game->players[g_game->localPlayer].id;
                if (FindHostSlot() == 10)
                    continue;
                SendPacketToPlayer(FindFrom_00450530(), FindToB_00450530(), &msg, 10);
            }
            else if (p->type == 3) {
                msg.type = 0x21;
                msg.flag = 0;
                msg.id = p->id;
                msg.arg = -1;
                if (FindPlayer_00450530() == 10)
                    continue;
                if (i == 9)
                    p->lobbyDataSynced = p->lobbyDataSynced;
                SendPacketToPlayer(FindFrom_00450530(), FindTo_00450530(), &msg, 10);
            }
        }
    }
}
