#include "win_types.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../../interfaces/host.h"
#include "../state/game_rand.h"
#include "../state/sim_globals.h"

struct DPNAME {
    unsigned long dwSize;
    unsigned long dwFlags;
    char* lpszShortNameA;
    char* lpszLongNameA;
};

typedef int (__stdcall *EntryFunc)(int);
typedef EntryFunc SimView_g_packetHandlers[45];
#undef g_packetHandlers
#define g_packetHandlers (*reinterpret_cast<SimView_g_packetHandlers*>(&Sim().g_packetHandlers))

class NetPacket {
public:
    void DispatchPacket();
};

#include "../map/mission.h"

#include "../network/packet_manager.h"

#include "../units/cob_script.h"

#include "../game/mission_conditions.h"

#pragma pack(push, 1)

#include "settings.h"

#include "../util/angles.h"

#include "../weapons/weapon_def.h"

struct Holder_00453640;
#include "../gui/gui.h"
#include "../gui/layer.h"
struct Net_4517b0;
struct ChatPacket;

#include "player_info.h"

#include "player.h"

#include "../units/unit.h"

#include "../game/game.h"

struct SideDataPacket {
    unsigned char type;
    PlayerInfo data;
};

struct PlayerIdPacket {
    unsigned char type;
    int id;
    unsigned char value;
};

#pragma pack(pop)

// GLOBAL: 0x511de8
extern Game* g_game;
#include "../../link/globals.h"

#include "packets.h"
#include "../game/statistics.h"
#include "net_game.h"
void __stdcall SetCloseHandler(void (__cdecl *callback)(int), int param);
#include "../graphics/display.h"
char* __stdcall Translate(char* text);
void __stdcall AddMessage(char* text, int key, int value, char last);
int __stdcall BroadcastPacket(int player, void* data, int size);
void __stdcall SetupPlayerSlot(unsigned char player, char type);
int GetTicks();
#include "unit_sync.h"
#include "unit_sync_state.h"
#include "../gui/dialogs.h"
#include <memory.h>
#include "../frontend/multi.h"
#include "../units/unit_position.h"
#include "../units/unit_targets.h"
#include "../map/features.h"
#include "../map/line_of_sight.h"
#include "../ingame/control_panel.h"
#include "../sound/sound.h"
#include "../units/unit_commands.h"
#include "../units/units.h"
#include "../game/game_state.h"
unsigned char FindHostSlot();
void __stdcall TransferMetal(unsigned char, unsigned char, int, int);
void __stdcall TransferEnergy(unsigned char, unsigned char, int, int);

// FUNCTION: 0x44fd40
int __stdcall DefaultPacketHandler(int arg1)
{
    return 0xffffffff;
}

// FUNCTION: 0x44fd50
int __stdcall RejectPacketType2(int arg1)
{
    return 0xffffffff;
}

// FUNCTION: 0x44fd60
int __stdcall RejectPacketType5(int)
{
    return 0xffffffff;
}

// FUNCTION: 0x44fd70
int __stdcall RejectPacketType6(int arg1)
{
    return 0xffffffff;
}

// FUNCTION: 0x44fd80
int __stdcall RejectPacketType7(int arg1)
{
    return 0xffffffff;
}

// FUNCTION: 0x44fd90
int __stdcall RejectPacketType8(int arg1)
{
    return 0xffffffff;
}

// FUNCTION: 0x44fda0
void NetPacket::DispatchPacket()
{
    unsigned int idx = 0;
    idx = *(unsigned char*)this;
    g_packetHandlers[idx]((int)this);
}

// FUNCTION: 0x44fdb0
int GetLocalDpid()
{
    int i = 0;
    while (!(g_game->players[i].active != 0
             && (g_game->players[i].type == 1 || g_game->players[i].type == 2))) {
        if (++i >= 10)
            return -1;
    }
    return g_game->players[i].id;
}

// FUNCTION: 0x44fe00
int GetLocalHumanDpid()
{
    for (int i = 0; i < 10; i++) {
        if (g_game->players[i].type == 1) {
            return g_game->players[i].id;
        }
    }
    return -1;
}

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

// FUNCTION: 0x44feb0
unsigned char __stdcall GetPlayerSlot(Player* player)
{
    if (player == 0) {
        return 10;
    }
    return player->index;
}

// FUNCTION: 0x44fed0
Player* __stdcall FindPlayerByDpid(int id)
{
    if (FindSlotByDpid(id) == 10)
        return 0;
    return &g_game->players[FindSlotByDpid(id)];
}

#pragma auto_inline(off)
// FUNCTION: 0x44ffd0
int __stdcall GetSlotDpid(unsigned char index)
{
    if (index != 10 && g_game->players[index].type != 0)
        return g_game->players[index].id;
    return -1;
}
#pragma auto_inline(on)

// FUNCTION: 0x450010
int __stdcall GetPlayerDpid(Player* obj)
{
    if (obj != 0 && obj->type != 0) {
        return (int)obj->id;
    }
    return -1;
}

// FUNCTION: 0x450030
int GetHostDpid()
{
    int i;
    for (i = 0; i < 10; i++) {
        if (g_game->players[i].info->flags_97 & 1)
            return GetSlotDpid(i);
    }
    return -1;
}

// FUNCTION: 0x450090
int __stdcall SetPlayerName(unsigned char index, char* shortName, char* longName)
{
    Player* p = &g_game->players[index];
    strncpy(p->name, longName, 30);
    strncpy(p->fullName, shortName, 30);
    DPNAME name;
    name.dwSize = sizeof(DPNAME);
    name.dwFlags = 0;
    name.lpszShortNameA = shortName;
    name.lpszLongNameA = longName;
    int r = host_net_set_player_name((host_net*)g_game->net_14, GetPlayerDpid(p), &name, 2);
    return r == 0 ? 1 : 0;
}

// FUNCTION: 0x450140
int __stdcall GetPlayerName(int dpid, char* shortName, char* longName)
{
    int r;
    if (dpid != -1) {
        char buf[0x400];
        unsigned long size = 0x400;
        r = host_net_get_player_name((host_net*)g_game->net_14, dpid, buf, &size);
        if (r == 0) {
            strcpy(shortName, ((DPNAME*)buf)->lpszShortNameA);
            strcpy(longName, ((DPNAME*)buf)->lpszLongNameA);
        }
    } else {
        strcpy(shortName, "COMPUTER");
        strcpy(longName, "COMPUTER");
        r = 0;
    }
    return r == 0;
}

// FUNCTION: 0x450240
unsigned char PickNewHost()
{
    unsigned int max = 0;
    Player* p = g_game->players;
    int n = 10;
    do {
        if ((p->active != 0 && p->type == 3)
            || (p->active != 0 && p->type == 1)) {
            if (p->id > max)
                max = p->id;
        }
        p++;
    } while (--n);

    Player* q = FindPlayerByDpid(max);
    if (q)
        q->info->flags_97 |= 1;
    return 10;
}

#include "../harness/harness.h"

// FUNCTION: 0x450380
void __stdcall AnnouncePlayerLeft(int id)
{
    char buf[200];
    if (g_game->players[g_game->localPlayer].rejectReason == 1)
        return;
    Player* p;
    if (FindSlotByDpid(id) == 10)
        p = 0;
    else
        p = &g_game->players[FindSlotByDpid(id)];
    if (p == 0)
        return;
    sprintf(buf, "%s %s", p->name, Translate(g_leftGameTexts[HarnessViewRand() & 7]));
    AddMessage(buf, 4, 0, p->index);
}

// FUNCTION: 0x4504f0
int FindFreeSlot()
{
    int found = 0;
    int i;
    for (i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (p->active == 0 && p->type != 4) {
            found = 1;
            break;
        }
    }
    if (!found)
        i = 10;
    return i;
}

#pragma pack(push, 1)
struct LobbySyncMessage {
    unsigned char type;
    unsigned char flag;
    int id;
    int arg;
};
#pragma pack(pop)

static inline int IsConnected(Player* player)
{
    if (player->active == 0)
        return 0;
    if (player->type == 1 || player->type == 2)
        return 1;
    return 0;
}

static inline unsigned char FindLobbyHostPlayer()
{
    for (unsigned char i = 0; i < 10; i++) {
        if (g_game->players[i].type != 0 && (g_game->players[i].info->flags_97 & 1))
            return i;
    }
    return 10;
}

static inline int FindConnectedPlayerId()
{
    for (int k = 0; k < 10; k++) {
        if (IsConnected(&g_game->players[k]))
            return g_game->players[k].id;
    }
    return -1;
}

static inline int FindLobbyHostDpid()
{
    for (int j = 0; j < 10; j++) {
        if (g_game->players[j].info->flags_97 & 1)
            return GetSlotDpid(j);
    }
    return -1;
}

static inline int FindLobbyHostPlayerId()
{
    for (int j = 0; j < 10; j++) {
        if (g_game->players[j].info->flags_97 & 1)
            return GetSlotDpid(j);
    }
    return -1;
}

// FUNCTION: 0x450530
void SendLobbySyncRequests()
{
    LobbySyncMessage msg;
    if (g_game->mode_391f1 == 6)
        return;
    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (p->active != 0
            && (p->type == 1 || p->type == 2 || p->type == 3)
            && p->index != 10
            && p->lobbyDataSynced == 0) {
            if (IsConnected(p) && p->type == 1) {
                Player* q = &g_game->players[FindHostSlot()];
                if (IsConnected(q)) {
                    p->lobbyDataSynced = 1;
                    continue;
                }
                msg.type = 0x21;
                msg.flag = 1;
                msg.id = p->id;
                msg.arg = -1;
                if (FindHostSlot() == 10)
                    continue;
                SendPacketToPlayer(FindConnectedPlayerId(), FindLobbyHostDpid(), (unsigned char *)&msg, 10);
            }
            else if (IsConnected(p) && p->type == 2) {
                Player* q = &g_game->players[FindHostSlot()];
                if (IsConnected(q)) {
                    p->lobbyDataSynced = 1;
                    continue;
                }
                msg.type = 0x21;
                msg.flag = 1;
                msg.id = p->id;
                msg.arg = g_game->players[g_game->localPlayer].id;
                if (FindHostSlot() == 10)
                    continue;
                SendPacketToPlayer(FindConnectedPlayerId(), FindLobbyHostPlayerId(), (unsigned char *)&msg, 10);
            }
            else if (p->type == 3) {
                msg.type = 0x21;
                msg.flag = 0;
                msg.id = p->id;
                msg.arg = -1;
                if (FindLobbyHostPlayer() == 10)
                    continue;
                if (i == 9)
                    p->lobbyDataSynced = p->lobbyDataSynced;
                SendPacketToPlayer(FindConnectedPlayerId(), FindLobbyHostDpid(), (unsigned char *)&msg, 10);
            }
        }
    }
}

// FUNCTION: 0x450910
char FindFreePlayerId(void)
{
    Game* game = g_game;
    for (int id = 1; id <= 10; id++) {
        int used = 0;
        for (int i = 0; i < 10; i++) {
            Player* p = &game->players[i];
            if (p->active != 0
                && (p->type == 1 || p->type == 2 || p->type == 3)
                && p->index != 10
                && p->lobbyDataSynced == id) {
                used = 1;
            }
        }
        if (!used) {
            return id;
        }
    }
    return 0;
}

// FUNCTION: 0x450980
void CheckDuplicatePlayerIds(void)
{
    int counts[11];
    memset(counts, 0, sizeof(counts));
    int dup = 0;
    for (int i = 0; i < 10; i++) {
        if (g_game->players[i].active != 0
            && (g_game->players[i].type == 1 || g_game->players[i].type == 2 || g_game->players[i].type == 3)
            && g_game->players[i].index != 10
            && g_game->players[i].lobbyDataSynced > 0 && g_game->players[i].lobbyDataSynced <= 10) {
            if (++counts[g_game->players[i].lobbyDataSynced] > 1) {
                dup = 1;
                break;
            }
        }
    }
    g_game->duplicateIds = dup;
}

static inline unsigned char FindSlot_00450a10(int id)
{
    unsigned char i;
    for (i = 0; i < 10; i++) {
        int v;
        if (i == 10) {
            v = -1;
        } else {
            v = g_game->players[i].type ? g_game->players[i].id : -1;
        }
        if (v == id) {
            return i;
        }
    }
    return 10;
}

// FUNCTION: 0x450a10
int __stdcall AddNetPlayer(int id)
{
    unsigned char slot;
    if (id == -1) {
        slot = 10;
    } else {
        slot = FindSlot_00450a10(id);
    }
    int flag = 0;
    if (slot != 10) {
        if (g_game->players[slot].type != 1 && g_game->players[slot].type != 2) {
            flag = 1;
        }
        if (g_game->players[slot].active != 0) {
            flag |= 1;
        }
    } else {
        int found = 0;
        int i = 0;
        while (i < 10) {
            Player* q = &g_game->players[i];
            if (q->active == 0 && q->type != 4) {
                found = 1;
                break;
            }
            i++;
        }
        int s = i;
        if (!found) {
            s = 10;
        }
        slot = s;
        if (slot == 10) {
            flag = 1;
        } else {
            g_game->players[slot].type = 3;
        }
    }
    Player* p = &g_game->players[slot];
    if (flag) {
        return 1;
    }
    unsigned long size;
    SideDataPacket packet;
    char buf[0x400];
    int result;
    char* d_full = p->fullName;
    char* d_name = p->name;
    if (id != -1) {
        size = 0x400;
        result = host_net_get_player_name((host_net*)g_game->net_14, id, buf, &size);
        if (result == 0) {
            strcpy(d_full, ((DPNAME*)buf)->lpszShortNameA);
            strcpy(d_name, ((DPNAME*)buf)->lpszLongNameA);
        }
    } else {
        strcpy(d_full, "COMPUTER");
        strcpy(d_name, "COMPUTER");
        result = 0;
    }
    if (result) {
        return 1;
    }
    SetupPlayerSlot(slot, g_game->players[slot].type);
    p->rejectReason = 0;
    p->id = id;
    p->lastHeard = GetTicks();
    g_game->numPlayers++;
    if (g_game->flags_2a44 & 1) {
        for (int i = 0; i < 10; i++) {
            Player* q = &g_game->players[i];
            if (q->active != 0 && (q->type == 1 || q->type == 2)) {
                packet.data = *q->info;
                packet.data.id = q->id;
                packet.type = 0x20;
                BroadcastPacket(q->id, &packet, sizeof(packet));
                if (q->active != 0 && (q->type == 1 || q->type == 2)) {
                    PlayerIdPacket* msg = (PlayerIdPacket*)g_game->recvPacketPtr;
                    msg->type = 0x24;
                    msg->id = q->id;
                    msg->value = q->alliance;
                    BroadcastPacket(q->id, msg, 6);
                    if (g_usePacketManager != 0) {
                        g_packetManager.SendAllQueued(1);
                    }
                }
            }
        }
        SendLobbySyncRequests();
        g_packetManager.SendAllQueued(1);
    }
    if (g_game->net->GetGameType() == 3 && g_game->numPlayers > 1) {
        ReportGameEvent(2);
    }
    return 1;
}

// FUNCTION: 0x450d80
void InitNetConnection()
{
    host_net_init_defaults((host_net*)g_game->net_14);
    g_game->netMode = 10;
    if (InitPacketManager(2, 100)) {
        host_net_init_connection((host_net*)g_game->net_14, &g_game->connection);
    }
}

// FUNCTION: 0x450dd0
void CloseNetSession()
{
    if (g_game->flags_2a44 & 1) {
        if (g_usePacketManager != 0) {
            g_packetManager.SendAllQueued(1);
        }
        if (!IsReporterDllLoaded()) {
            host_net_uninit((host_net*)g_game->net_14);
        }
        g_game->flags_2a44 &= 0xfffe;
    }
}

// FUNCTION: 0x450e20
void LeaveNetGame()
{
    if (g_usePacketManager != 0) {
        g_packetManager.SendAllQueued(1);
    }
    if (g_game->flags_2a44 & 1) {
        for (int i = 0; i < 10; i++) {
            if (IsConnected(&g_game->players[i])) {
                RemovePlayer(g_game->players[i].id);
            }
        }
        ShutdownScoreTables();
    }
    host_net_quit_game((host_net*)g_game->net_14);
    SetCloseHandler(0, 0);
    g_game->endGameFlags |= 4;
    int reason = g_game->players[g_game->localPlayer].rejectReason;
    char* text;
    if (reason != 0) {
        switch (reason) {
        case 4:
            text = "You did not have the correct password";
            break;
        case 3:
            text = "The game is closed";
            break;
        case 5:
            text = "The game is full";
            break;
        case 6:
            text = "You have lost connection with the game";
            break;
        case 7:
            text = "You need a unit you don't have for this game";
            break;
        case 8:
            text = "You need a newer version of the game to enter";
            break;
        case 9:
            text = "No watching is allowed for this game";
            break;
        case 10:
            text = "The creator has left the game";
            break;
        default:
            text = "You were rejected from the game";
            break;
        }
    } else {
        text = 0;
    }
    QuitApp(text);
}

// FUNCTION: 0x450f90
void BroadcastPlayerInfo()
{
    SideDataPacket packet;
    if (g_game->flags_2a44 & 1) {
        for (int i = 0; i < 10; i++) {
            Player* player = &g_game->players[i];
            if (IsConnected(player)) {
                packet.data = *player->info;
                packet.data.id = player->id;
                packet.type = 0x20;
                BroadcastPacket(player->id, &packet, sizeof(packet));
                if (IsConnected(player)) {
                    PlayerIdPacket* msg = (PlayerIdPacket*)g_game->recvPacketPtr;
                    msg->type = 0x24;
                    msg->id = player->id;
                    msg->value = player->alliance;
                    BroadcastPacket(player->id, msg, 6);
                    if (g_usePacketManager != 0) {
                        g_packetManager.SendAllQueued(1);
                    }
                }
            }
        }
        SendLobbySyncRequests();
        g_packetManager.SendAllQueued(1);
    }
}

// FUNCTION: 0x451090
void __stdcall BuildGameInfo(char* name, int* d, int* c, int* b, int* a)
{
    PlayerInfo* info = g_game->players[g_game->localPlayer].info;
    info->versionMajor = g_game->version;
    char* p = (char*)&info->memory;
    p[0xf] = g_game->versionMinor;
    unsigned short w = *(unsigned short*)(p + 2);
    p += 4;
    *(unsigned short*)(p - 2) = w ^ ((g_game->numPlayers ^ w) & 0xf);
    p += 4;
    *d = *(int*)(p - 8);
    *c = *(int*)(p - 4);
    *b = *(int*)(p);
    *a = *(int*)(p + 4);
    memset(name, ' ', 0x20);
    name[0x1f] = 0;
    strncpy(name, g_game->gameName, 0x10);
    int src = (int)g_game->net->GetMissionName();
    strncpy(name + 0x10, (char*)src, 0xf);
    char* q = name;
    int n = 0x20;
    do {
        if (*q == 0)
            *q = ' ';
        q++;
    } while (--n);
    name[0x1f] = 0;
}

// FUNCTION: 0x451180
void UpdateNetGameInfo(void)
{
    int a;
    int b;
    int c;
    int d;
    char name[32];

    BuildGameInfo(name, &d, &c, &b, &a);
    if (g_game->players[g_game->localPlayer].info->started) {
        g_game->settings.bits_475.flag_475_5 = 1;
    }
    host_net_update_game_info((host_net*)g_game->net_14, name, DAT_005119b8, d, c, b, a);
}

static inline unsigned char FindPlayerInUse()
{
    for (unsigned char i = 0; i < 10; i++) {
        if (g_game->players[i].type != 0 && g_game->players[i].info->host)
            return i;
    }
    return 10;
}

// FUNCTION: 0x451220
int __stdcall CreateLocalPlayer(unsigned char playerIndex, int flag)
{
    char buf[256];

    Player* player = &g_game->players[playerIndex];
    player->SetType(flag);

    int same = (playerIndex == FindPlayerInUse());

    if (flag == 1) {
        strcpy(buf, g_game->nickname);
    } else {
        sprintf(buf, "AI:%s", g_game->players[g_game->localPlayer].name);
        buf[16] = 0;
    }

    player->info->host = same;
    player->info->color = 0xff;
    player->keepaliveFlags &= 0xfd;
    player->joinTime = GetTicks();
    PlayerInfo* info = player->info;
    info->flags = (info->flags ^ ((g_game->numPlayersSigned ^ info->flags) & 0xf)) & 0x7fff;
    info->hasPassword = (strlen(g_game->password) != 0);
    info->maxUnits = 0x64;
    info->width = (unsigned short)g_game->width_37f1b;
    info->height = (unsigned short)g_game->height_37f1f;
    info->versionMajor = g_game->version;
    info->versionMinor = g_game->versionMinor;

    int r = host_net_add_player((host_net*)g_game->net_14, (unsigned long*)&player->id,
                         buf, buf, g_game->password, 0, 0x50);
    if (r == 0) {
        g_game->players[playerIndex].SetType(0);
        unsigned char i = FindPlayerInUse();
        Player* slot = &g_game->players[i];
        if (slot->active != 0 && (slot->type == 1 || slot->type == 2)) {
            OpenMessageBox((Gui *)g_game->menu,
                Translate("Direct Play failed to add new player.\n\nRecommended you go to previous screen and re-create the game session.\n"),
                500, 1, 1);
        }
        else {
            OpenMessageBox((Gui *)g_game->menu,
                Translate("Direct Play failed to add new player.\n\nRecommended you go to previous screen and re-join the game session.\n"),
                500, 1, 1);
        }
    }
    return r;
}

#define WIN32_LEAN_AND_MEAN
#include "win_types.h"
#include <string.h>
#include <stdio.h>
#include <iostream>

#pragma pack(push, 1)

#include "packet_receiver.h"

struct Guid_4517b0 {
    int data[4];
};

struct SessionDesc_004517b0 {
    char unknown_0[0x30];
    char* lpszSessionName;
};

struct Net_4517b0 {
    char unknown_0[4];
    unsigned int flags;
    SessionDesc_004517b0* desc;
    DPNAME* name;
};

struct Obj_00451640 {
    char unknown_0[0xf0];
    unsigned short bit0 : 1;
    unsigned short flag : 1;
};

struct Args_00451640 {
    char* name;
    int arg_4;
    int arg_8;
    int arg_c;
    int arg_10;
    int arg_14;
    int result;
    void* event;
};

struct Class_00451fd0 {
    char unknown_0[0x870];
    unsigned int lobbySyncTick;
    char unknown_874[0x1745 - 0x874];
    int recvPacketSize;
    union {
        int* buffer;
    };
};

struct Entry_00453640 {
    char unknown_0[0xc0];
    short count;
};

struct Holder_00453640 {
    int unknown_0;
    Entry_00453640* entries;
};

struct Entry_004538f0 {
    char unknown_0[0xb6];
    char text[0x15b - 0xb6];
};

struct OutEntry_00453a50 {
    char unknown_0[0x19];
    short height;
};

struct Gui_00453a50 {
    char unknown_0[4];
    void* entries;
    void (__stdcall* callback)(void*);
    char unknown_c[0x1c - 0xc];
    void (__stdcall* onFrame)(void*);
};

struct ChatPacket {
    unsigned char type;
    char text[0x40];
};

void Unused_00452960(int a, int b, int c, int d, int e);

struct AllyFlagsPacket {
    unsigned char type;
    int fromNetId;
    int toNetId;
    char allied;
    int force;
};

struct UnitCreatePacket {
    unsigned char type;
    short defIndex;
    short unitId;
    Vec3 pos;
    Angles16 angles;
};

struct BuilderLinkPacket {
    unsigned char type;
    short constructedUnitId;
    short builderUnitId;
};

struct JoinRequestData {
    char name[17];
    short versionMajor;
    short versionMinor;
};

struct Packet_00453d40 {
    unsigned int type;
    int playerType;
    int id;
    char* data;
    JoinRequestData* createData;
    int dataSize;
    char* longName;
};

struct PlayerIdIntPacket {
    unsigned char type;
    int id;
    int value;
};

struct UnitScriptCallRecv {
    unsigned char type;
    unsigned short id;
    short index;
    unsigned char argCount;
    int arg0;
    int arg1;
    int arg2;
    int arg3;
};

struct PlaySoundPacket {
    unsigned char type;
    unsigned char flag;
    int index;
    int pos[3];
};

struct OwnershipTransferPacket {
    unsigned char type;
    short unitId;
    int newOwnerNetId;
    char unknown_7[0x18 - 7];
};

struct ResourceSharePacket {
    unsigned char type;
    int subtype;
    int fromNetId;
    int toNetId;
    int amount;
};


#pragma pack(pop)

extern int DAT_00512c8c;
extern char g_onlineLobbyPlayerName;
extern char g_onlineLobbyPassword;
typedef EntryFunc SimView_g_packetHandlerType2;
#undef g_packetHandlerType2
#define g_packetHandlerType2 (*reinterpret_cast<SimView_g_packetHandlerType2*>(reinterpret_cast<char*>(&g_packetHandlers) + 0x8))

#include "../debug/debug_lib.h"
int __stdcall InitPacketTables(Class_00451fd0* obj);
void __stdcall JoinLobbyGameThread(Args_00451640* args);
unsigned __stdcall JoinLobbyGameThread(void* args);
int __stdcall SetAlliance(int from, int to, unsigned char value, int extra);
int __stdcall RejectPlayer(int id, unsigned char value);
void __stdcall UpdateTimeoutDialog(void* gadget);
#include "online.h"
void __stdcall SetCursorMode(int n);
#include "../graphics/object_pool.h"
void* __cdecl GameReallocTagged(void* ptr, const char* name, unsigned int size);
#include "../game/players.h"
Obj_00451640* GetDisplay(void);
#include "../graphics/display.h"
#include "net_stats.h"
void __stdcall CreateUnitFromPacket(unsigned char p, void* packet);
void __stdcall ApplyWeaponFirePacket(Player* player, void* packet);
void __stdcall ApplyProjectileHitPacket(Player* player, void* packet);
void __stdcall PlaySoundAt(int index, void* pos, int param_3);
void __stdcall PlaySoundByName(char* name, int param_2);
Entry_004538f0* __stdcall FindGadgetChecked_B(Entry_004538f0* entries, char* name);
int __stdcall SendChatMessage(Player* from, char* text, int key, char* to);
int __stdcall FindGadgetIndex(void* gadgets, const char* name, int flag);
void* __stdcall LoadGuiLayer(void* obj, const char* name, int size);
void* __stdcall FindGadgetChecked(void* entries, char* name);
void __stdcall ConfigureListBoxByName(void* obj, char* name, void* p, int count, int flags);
void __stdcall SetTranslatedTextByName(void* obj, char* name, void* out, int flag);
void __stdcall SetKeyboardInput(void* obj, int value);

// FUNCTION: 0x451540
void CreateNetGame(void)
{
    int a;
    int b;
    int c;
    int d;
    char name[32];

    g_game->players[g_game->localPlayer].info->host = 1;
    BuildGameInfo(name, &d, &c, &b, &a);
    g_game->numPlayers = 0;
    host_net_create_new_game((host_net*)g_game->session, name, DAT_005119b8, d, c, b, a);
}

// FUNCTION: 0x4515d0
void __stdcall EnumPlayersCallback(int id, int unused1, int unused2, int unused3, int unused4)
{
    if (id != -1) {
        for (unsigned char i = 0; i < 10; i++) {
            if (GetSlotDpid(i) == id)
                break;
        }
    }
    AddNetPlayer(id);
}

static void HOST_CALL JoinLobbyGameEntry(void* param)
{
    Args_00451640* args = (Args_00451640*)param;
    void* event = args->event;
    JoinLobbyGameThread(args);
    host_event_set(event);
}

// FUNCTION: 0x451640
int __stdcall JoinLobbyGame(Player* p)
{
    int result = 0;
    char* name = 0;

    if (host_net_password_required((host_net*)g_game->net_14) != 0)
        name = p->info->password;

    int count = 10;
    if (IsOnlineConfigLoaded() != 0 && DAT_00512c8c > 1) {
        unsigned int n = DAT_00512c8c;
        count = n < 10 ? n : 10;
        g_game->netMode = count;
    }

    Args_00451640* args = (Args_00451640*)GameAllocIgnoreTag("LOBBY JOIN INFO", 0x20);
    if (args != 0) {
        memset(args, 0, 0x20);
        args->arg_4 = count;
        args->name = name;
        args->arg_8 = 0;
        args->arg_c = 0;
        args->arg_10 = 0;
        args->arg_14 = 0;
        args->result = 0;

        args->event = host_event_create(0, 0);

        SetCursorMode(0x14);
        if (host_thread_start(JoinLobbyGameEntry, 0x8000, args)) {
            if (host_event_wait(args->event, 40000) == 0) {
                SetCursorMode(0x13);
                if (GetDisplay()->flag) {
                    ToggleFullScreen();
                    host_sleep_ms(500);
                }
                FatalError(Translate("Timed out while connecting to DirectPlay lobby!"));
            } else {
                result = args->result;
            }
        }

        SetCursorMode(0x13);
        host_event_destroy(args->event);
        GameFreeThunk(args);
    }

    return result;
}

// FUNCTION: 0x451770
void __stdcall JoinLobbyGameThread(Args_00451640* args)
{
    args->result = host_net_create_or_join_lobby_game((host_net*)g_game->net_14, args->name, args->arg_4, args->arg_8,
                                args->arg_c, args->arg_10, args->arg_14);
}

// FUNCTION: 0x4517b0
int __stdcall JoinNetGame(Guid_4517b0 guid, int player)
{
    char name[0x100];
    SideDataPacket packet;

    if (player == g_game->localPlayer) {
        Player* p = &g_game->players[player];

        Net_4517b0* net = g_game->dplConnection;
        if (net != 0) {
            int v = IsOnlineConfigLoaded();
            char* s = &g_onlineLobbyPlayerName;
            if (v == 0)
                s = DAT_005119b8;

            DPNAME* n2 = net->name;
            if (n2 != 0) {
                char* t = n2->lpszShortNameA;
                if (t != 0 && *t != 0) {
                    s = t;
                } else {
                    char* t2 = n2->lpszLongNameA;
                    if (t2 != 0 && *t2 != 0)
                        s = t2;
                }
            }

            if (*s != 0) {
                g_game->nickname[0] = 0;
                strncat(g_game->nickname, s, 0x10);
            }

            if (v != 0 && g_onlineLobbyPassword != 0) {
                p->info->password[0] = 0;
                strncat(p->info->password, &g_onlineLobbyPassword, 0xa);
                p->info->hasPassword = 1;
                if ((*(unsigned char*)&g_game->dplConnection->flags & 2) != 0) {
                    g_game->password[0] = 0;
                    strncat(g_game->password, &g_onlineLobbyPassword, 0xa);
                }
            }

            ResetPlayerSlots();
        }

        if (strlen(g_game->nickname) == 0) {
            host_user_name(name, 0x100);
        } else {
            strcpy(name, g_game->nickname);
        }

        if (g_game->dplConnection != 0) {
            p->info->host = g_game->dplConnection->flags >> 1;
        } else {
            p->info->host = 0;
        }

        p->info->color = 0xff;
        p->info->flags &= 0xffdf;
        p->info->width = (unsigned short)g_game->width_37f1b;
        p->info->height = (unsigned short)g_game->height_37f1f;
        g_game->numPlayers = 0;

        int result;
        if (g_game->dplConnection != 0) {
            result = JoinLobbyGame(p);
            if (result == 0) {
                if (GetDisplay()->flag) {
                    ToggleFullScreen();
                    host_sleep_ms(500);
                }
                FatalError(Translate("Unable to connect to DirectPlay lobby."));
            }
        } else {
            result = host_net_join_game((host_net*)g_game->net_14, (const host_guid*)&guid);
        }
        if (result == 0)
            return 0;

        CreateLocalPlayer(player, 1);
        host_net_enum_players((host_net*)g_game->net_14, 0, (void*)EnumPlayersCallback, 0, 0);
        if (g_game->netGameFlags & 1) {
            for (int i = 0; i < 10; i++) {
                Player* q = &g_game->players[i];
                if (q->active != 0 && (q->type == 1 || q->type == 2)) {
                    packet.data = *q->info;
                    packet.data.id = q->id;
                    packet.type = 0x20;
                    BroadcastPacket(q->id, &packet, sizeof(packet));
                    if (q->active != 0 && (q->type == 1 || q->type == 2)) {
                        PlayerIdPacket* msg = (PlayerIdPacket*)g_game->recvPacketPtr;
                        msg->type = 0x24;
                        msg->id = q->id;
                        msg->value = q->alliance;
                        BroadcastPacket(q->id, msg, 6);
                        if (g_usePacketManager != 0)
                            g_packetManager.SendAllQueued(1);
                    }
                }
            }
            SendLobbySyncRequests();
            g_packetManager.SendAllQueued(1);
        }

        RequestPlayerColor(0);
        return 1;
    }
    return 0;
}

// FUNCTION: 0x451b60
void RemoveLocalPlayers()
{
    if (!(g_game->flags_2a44 & 1))
        return;
    for (int i = 0; i < 10; i++) {
        if (g_game->players[i].active != 0
            && (g_game->players[i].type == 1 || g_game->players[i].type == 2)) {
            RemovePlayer(g_game->players[i].id);
        }
    }
    ShutdownScoreTables();
}

static inline unsigned char FindIndex_00451bc0(int id)
{
    for (unsigned char i = 0; i < 10; i++) {
        if (GetSlotDpid(i) == id)
            return i;
    }
    return 10;
}

// FUNCTION: 0x451bc0
int __stdcall SendPacketToPlayer(int from, int to, unsigned char* packet, int size)
{
    unsigned char fi;
    if (from == -1)
        fi = 10;
    else
        fi = FindIndex_00451bc0(from);
    Player* fromPlayer;
    if (fi == 10)
        fromPlayer = 0;
    else
        fromPlayer = &g_game->players[FindSlotByDpid(from)];

    unsigned char ti;
    if (to == -1)
        ti = 10;
    else
        ti = FindIndex_00451bc0(to);
    Player* toPlayer;
    if (ti == 10)
        toPlayer = 0;
    else
        toPlayer = &g_game->players[FindSlotByDpid(to)];

    if ((g_game->netGameFlags & 1) && fromPlayer != 0 && fromPlayer->active != 0 &&
        (fromPlayer->type == 1 || fromPlayer->type == 2) &&
        fromPlayer->rejectReason == 0 && toPlayer != 0 && toPlayer->active != 0 &&
        toPlayer->type == 3 && toPlayer->rejectReason == 0) {
        Player* target = &g_game->players[FindSlotByDpid(to)];
        if (target->active == 0 || (target->type != 1 && target->type != 2)) {
            if (g_usePacketManager != 0)
                return g_packetManager.QueuePacket(from, to, packet, size);
            if (host_net_send_packet((host_net*)g_game->net_14, from, to, packet, size) != 0)
                return 0;
            CountMessage(packet[0], size, 1);
            CountPacket(size, 0, 1);
        }
        return 1;
    }
    return 0;
}

// FUNCTION: 0x451df0
int __stdcall BroadcastPacket(int id, unsigned char* packet, int size)
{
    Player* p = FindPlayerByDpid(id);
    if (p == 0 || p->active == 0)
        return 0;
    if (p->type != 1 && p->type != 2)
        return 0;
    if (p->rejectReason != 0)
        return 0;
    if ((g_game->netGameFlags & 1) != 0) {
        if (g_game->duplicatePlayerTypeFlag == 0) {
            if (g_usePacketManager != 0)
                return g_packetManager.QueueOnChannel(id, g_packetChannels, (int)packet, size);
            if (host_net_send_packet((host_net*)g_game->net_14, id, 0, packet, size) != 0)
                return 0;
            CountMessage(packet[0], size, 1);
            CountPacket(size, 0, 1);
            return 1;
        }
        memset(g_broadcastPeersSeen, 0, 0x2c);
        for (int i = 0; i < 10; i++) {
            if (g_game->players[i].active == 0)
                continue;
            if (g_game->players[i].type != 3)
                continue;
            if (g_broadcastPeersSeen[g_game->players[i].lobbyDataSynced] != 0)
                continue;
            SendPacketToPlayer(id, g_game->players[i].id, packet, size);
            int c = g_game->players[i].lobbyDataSynced;
            if (c >= 0 && c < 10)
                g_broadcastPeersSeen[c] = 1;
        }
    }
    return 1;
}

// FUNCTION: 0x451fd0
int __stdcall InitPacketTables(Class_00451fd0* obj)
{
    g_packetModes[9] = 4;
    g_packetModes[44] = 4;
    g_packetModes[10] = 4;
    g_packetModes[11] = 4;
    g_packetModes[12] = 4;
    g_packetModes[13] = 4;
    g_packetModes[14] = 4;
    g_packetModes[15] = 4;
    g_packetModes[16] = 4;
    memset(&g_packetSizes[1], 0, 0xb0);
    g_packetSizes[17] = 4;
    g_packetModes[17] = 4;
    g_packetModes[18] = 4;
    g_packetModes[20] = 4;
    g_packetSizes[2] = 13;
    g_packetHandlerType2 = RejectPacketType2;
    g_packetModes[2] = 7;
    g_packetSizes[5] = 65;
    g_packetHandlers[5] = RejectPacketType5;
    g_packetModes[5] = 7;
    g_packetSizes[6] = 1;
    g_packetHandlers[6] = RejectPacketType6;
    g_packetModes[6] = 7;
    g_packetSizes[7] = 1;
    g_packetHandlers[7] = RejectPacketType7;
    g_packetModes[7] = 7;
    g_packetSizes[8] = 1;
    g_packetHandlers[8] = RejectPacketType8;
    g_packetModes[8] = 7;
    g_packetSizes[9] = 23;
    g_packetHandlers[9] = DefaultPacketHandler;
    g_packetSizes[44] = 3;
    g_packetHandlers[44] = DefaultPacketHandler;
    g_packetSizes[10] = 7;
    g_packetHandlers[10] = DefaultPacketHandler;
    g_packetSizes[11] = 9;
    g_packetHandlers[11] = DefaultPacketHandler;
    g_packetSizes[12] = 11;
    g_packetHandlers[12] = DefaultPacketHandler;
    g_packetSizes[13] = 36;
    g_packetHandlers[13] = DefaultPacketHandler;
    g_packetSizes[14] = 14;
    g_packetHandlers[14] = DefaultPacketHandler;
    g_packetSizes[15] = 6;
    g_packetHandlers[15] = DefaultPacketHandler;
    g_packetSizes[16] = 22;
    g_packetHandlers[16] = DefaultPacketHandler;
    g_packetHandlers[17] = DefaultPacketHandler;
    g_packetSizes[18] = 5;
    g_packetHandlers[18] = DefaultPacketHandler;
    g_packetSizes[19] = 18;
    g_packetHandlers[19] = DefaultPacketHandler;
    g_packetModes[19] = 7;
    g_packetSizes[20] = 24;
    g_packetHandlers[20] = DefaultPacketHandler;
    g_packetSizes[21] = 1;
    g_packetHandlers[21] = DefaultPacketHandler;
    g_packetModes[21] = 6;
    g_packetSizes[22] = 17;
    g_packetHandlers[22] = DefaultPacketHandler;
    g_packetModes[22] = 4;
    g_packetSizes[23] = 2;
    g_packetHandlers[23] = DefaultPacketHandler;
    g_packetModes[23] = 7;
    g_packetSizes[24] = 2;
    g_packetHandlers[24] = DefaultPacketHandler;
    g_packetModes[24] = 7;
    g_packetSizes[25] = 3;
    g_packetHandlers[25] = DefaultPacketHandler;
    g_packetModes[25] = 7;
    g_packetSizes[27] = 6;
    g_packetHandlers[27] = DefaultPacketHandler;
    g_packetModes[27] = 7;
    g_packetSizes[28] = 5;
    g_packetHandlers[28] = DefaultPacketHandler;
    g_packetModes[28] = 7;
    g_packetSizes[26] = 14;
    g_packetHandlers[26] = DefaultPacketHandler;
    g_packetModes[26] = 1;
    g_packetSizes[29] = 9;
    g_packetHandlers[29] = DefaultPacketHandler;
    g_packetSizes[37] = 5;
    g_packetHandlers[37] = DefaultPacketHandler;
    g_packetModes[37] = 1;
    g_packetSizes[38] = 41;
    g_packetHandlers[38] = DefaultPacketHandler;
    g_packetModes[38] = 7;
    g_packetSizes[35] = 14;
    g_packetHandlers[35] = DefaultPacketHandler;
    g_packetModes[35] = 7;
    g_packetSizes[36] = 6;
    g_packetHandlers[36] = DefaultPacketHandler;
    g_packetModes[36] = 7;
    g_packetSizes[30] = 2;
    g_packetHandlers[30] = DefaultPacketHandler;
    g_packetModes[30] = 6;
    g_packetSizes[31] = 5;
    g_packetHandlers[31] = DefaultPacketHandler;
    g_packetModes[31] = 6;
    g_packetSizes[32] = 186;
    g_packetHandlers[32] = DefaultPacketHandler;
    g_packetModes[32] = 7;
    g_packetSizes[33] = 10;
    g_packetHandlers[33] = DefaultPacketHandler;
    g_packetModes[33] = 1;
    g_packetSizes[34] = 6;
    g_packetHandlers[34] = DefaultPacketHandler;
    g_packetModes[34] = 1;
    g_packetSizes[39] = 17;
    g_packetHandlers[39] = DefaultPacketHandler;
    g_packetModes[39] = 7;
    g_packetSizes[40] = 58;
    g_packetHandlers[40] = DefaultPacketHandler;
    g_packetModes[40] = 7;
    g_packetSizes[41] = 3;
    g_packetHandlers[41] = DefaultPacketHandler;
    g_packetModes[41] = 7;
    g_packetSizes[3] = 3;
    g_packetHandlers[3] = DefaultPacketHandler;
    g_packetModes[3] = 7;
    g_packetSizes[42] = 2;
    g_packetHandlers[42] = DefaultPacketHandler;
    g_packetModes[42] = 7;
    obj->lobbySyncTick = GetTicks();
    obj->recvPacketSize = 0x2000;
    obj->buffer = (int*)GameAllocIgnoreTag(s_PACKET_DATA_00506524, 0x2000);
    return 0 != obj->buffer;
}

// FUNCTION: 0x452370
void __stdcall ReleasePacketData(Class_00451fd0* obj)
{
    if (obj->buffer) {
        GameFreeThunk(obj->buffer);
        obj->buffer = 0;
    }
    if (g_game->netGameFlags & 1) {
        if (g_usePacketManager != 0) {
            g_packetManager.SendAllQueued(1);
        }
        if (!IsReporterDllLoaded()) {
            host_net_uninit((host_net*)g_game->session);
        }
        g_game->flags_2a44 &= 0xfffe;
    }
}

// FUNCTION: 0x4523e0
int __stdcall AssignPlayerColor(int from, int to, int group)
{
    unsigned char packet[4];
    for (int i = 0; i < 10; i++, group++) {
        if (group >= 10)
            group = 0;
        int j;
        for (j = 0; j < 10; j++) {
            Player* p = &g_game->players[j];
            if (p->type != 0 && p->type != 4 && p->id != to && p->info->color == group)
                break;
        }
        if (j == 10) {
            packet[3] = group;
            break;
        }
    }
    if (to == g_game->players[g_game->localPlayer].id) {
        g_game->players[g_game->localPlayer].info->color = group;
        return 1;
    }
    packet[2] = 0x18;
    int result = SendPacketToPlayer(from, to, packet + 2, 2);
    if (result != 0 && g_usePacketManager != 0) {
        g_game->players[FindSlotByDpid(to)].info->color = group;
        g_packetManager.SendAllQueued(1);
    }
    return result;
}

static __inline unsigned char FindSlot_00452570(int id)
{
    unsigned char i;
    for (i = 0; i < 10; i++) {
        int v;
        if (i == 10) {
            v = -1;
        } else {
            v = g_game->players[i].type ? g_game->players[i].id : -1;
        }
        if (v == id) {
            return i;
        }
    }
    return 10;
}

// FUNCTION: 0x452570
int __stdcall IsColorFree(int id, int slot)
{
    if (slot == 0xff || slot < 0 || slot >= 10) {
        return 0;
    }

    unsigned char colorOwner[10];
    memset(colorOwner, -1, 10);

    unsigned char found;
    if (id == -1) {
        found = 10;
    } else {
        found = FindSlot_00452570(id);
    }
    if (found != 10 && id != -1) {
        FindSlot_00452570(id);
    }
    for (int j = 0; j < 10; j++) {
        if (g_game->players[j].type != 0 && g_game->players[j].id != id &&
            g_game->players[j].info->color != 0xff) {
            colorOwner[g_game->players[j].info->color] = (unsigned char)j;
        }
    }
    return colorOwner[slot] == 0xff;
}

static inline unsigned char FindReadyPlayer()
{
    for (unsigned char i = 0; i < 10; i++) {
        if (g_game->players[i].type && g_game->players[i].info->host)
            return i;
    }
    return 10;
}

// FUNCTION: 0x4526c0
int __stdcall RequestPlayerColor(int param)
{
    Player* p = &g_game->players[g_game->localPlayer];
    int i = FindReadyPlayer();
    if (i == g_game->localPlayer) {
        if (IsColorFree(p->id, param) == 0) {
            AssignPlayerColor(p->id, p->id, param);
            return 1;
        }
        p->info->color = param;
        return 1;
    }

    unsigned char* buffer = g_game->recvPacketPtr;
    buffer[0] = 0x17;
    buffer[1] = (unsigned char)param;

    int result;
    if (i == 10)
        result = BroadcastPacket(p->id, buffer, 2);
    else
        result = SendPacketToPlayer(p->id, g_game->players[i].id, buffer, 2);

    if (g_usePacketManager != 0)
        g_packetManager.SendAllQueued(1);
    return result;
}

// FUNCTION: 0x452800
int __stdcall BroadcastPlayerLeft(int id)
{
    if (FindSlotByDpid(id) == 10)
        return 0;
    PlayerIdPacket* msg = (PlayerIdPacket*)g_game->recvPacketPtr;
    msg->type = 0x1c;
    msg->id = id;
    unsigned char index = FindSlotByDpid(id);
    Player* player = &g_game->players[index];
    if ((player->active != 0 && player->type == 3)
        || !((*(volatile unsigned char*)&g_game->flags_38d75) & 1)
        || ((*(volatile unsigned char*)&g_game->flags_38d75) & 2)) {
        RemovePlayer(id);
    }
    return BroadcastPacket(g_game->players[g_game->localPlayer].id, msg, 5);
}

static inline unsigned char FindIndex_00452960(int id)
{
    for (unsigned char i = 0; i < 10; i++) {
        if (GetSlotDpid(i) == id)
            return i;
    }
    return 10;
}

static inline int IsActive12_00452960(Player* p)
{
    if (p->active == 0)
        return 0;
    if (p->type == 1 || p->type == 2)
        return 1;
    return 0;
}

static inline int IsType3_00452960(Player* p)
{
    if (p->active == 0)
        return 0;
    if (p->type == 3)
        return 1;
    return 0;
}

static inline int IsState2_00452960(Player* p)
{
    if (p->active == 0)
        return 0;
    if (p->type == 2)
        return 1;
    return 0;
}

static inline int IsState3_00452960(Player* p)
{
    if (p->active == 0)
        return 0;
    if (p->type == 3)
        return 1;
    return 0;
}

// FUNCTION: 0x452960
int __stdcall SetAlliance(int from, int to, unsigned char value, int extra)
{
    unsigned char fi;
    if (from == -1)
        fi = 10;
    else
        fi = FindIndex_00452960(from);

    Player* p1;
    if (fi == 10)
        p1 = 0;
    else
        p1 = &g_game->players[FindSlotByDpid(from)];

    Player* p2;
    if (FindSlotByDpid(to) == 10)
        p2 = 0;
    else
        p2 = &g_game->players[FindSlotByDpid(to)];

    int result = 0;
    if (p1 == 0 || p2 == 0)
        return 0;

    unsigned char idx_;
    unsigned char& idx = idx_;
    if (IsActive12_00452960(p1)) {
        idx = p2->index;
        p1->allied[idx] = value;
        if (IsState2_00452960(p2)
            || (IsState3_00452960(p2) && p2->info->kind == 2)
            || extra != 0) {
            idx = p2->index;
            p1->alliedBy[idx] = value;
        }
        result = 1;
    }
    if (IsActive12_00452960(p2)) {
        idx = p1->index;
        p2->alliedBy[idx] = value;
        if (IsState2_00452960(p2) || extra != 0) {
            idx = p1->index;
            p2->allied[idx] = value;
        }
        result = 1;
    } else if (IsType3_00452960(p2)) {
        AllyFlagsPacket* msg = (AllyFlagsPacket*)g_game->recvPacketPtr;
        msg->allied = value;
        msg->type = 0x23;
        msg->fromNetId = from;
        msg->toNetId = to;
        msg->force = extra;
        int r = SendPacketToPlayer(from, to, (unsigned char *)msg, 0xe);
        if (g_usePacketManager != 0)
            g_packetManager.SendAllQueued(1);
        result = r;
    }
    if (g_game->net->GetGameType() == 3)
        ReportGameEvent(4);
    return result;
}

// FUNCTION: 0x452b70
int __stdcall SendAlliance(int from, int to, char value, int extra)
{
    AllyFlagsPacket* msg = (AllyFlagsPacket*)g_game->recvPacketPtr;
    msg->allied = value;
    msg->type = 0x23;
    msg->fromNetId = from;
    msg->toNetId = to;
    msg->force = extra;
    int result = SendPacketToPlayer(from, to, (unsigned char *)msg, 0xe);
    if (g_usePacketManager != 0) {
        g_packetManager.SendAllQueued(1);
    }
    return result;
}

#pragma auto_inline(off)
// FUNCTION: 0x452bd0
int __stdcall BroadcastAllyTeam(Player* player)
{
    if (!IsConnected(player)) {
        return 0;
    }
    PlayerIdPacket* msg = (PlayerIdPacket*)g_game->recvPacketPtr;
    msg->type = 0x24;
    msg->id = player->id;
    msg->value = player->alliance;
    int result = BroadcastPacket(player->id, msg, 6);
    if (g_usePacketManager != 0) {
        g_packetManager.SendAllQueued(1);
    }
    return result;
}

#pragma auto_inline(on)

// FUNCTION: 0x452c40
char* __stdcall GetRejectReasonText(int reason)
{
    switch (reason) {
    case 4:
        return "You did not have the correct password";
    case 3:
        return "The game is closed";
    case 5:
        return "The game is full";
    case 6:
        return "You have lost connection with the game";
    case 7:
        return "You need a unit you don't have for this game";
    case 8:
        return "You need a newer version of the game to enter";
    case 9:
        return "No watching is allowed for this game";
    case 10:
        return "The creator has left the game";
    default:
        return "You were rejected from the game";
    }
}

static inline int IsType1_00452cc0(Player* p)
{
    if (p->active == 0)
        return 0;
    if (p->type == 1)
        return 1;
    return 0;
}

static inline int IsType3_00452cc0(Player* p)
{
    if (p->active == 0)
        return 0;
    if (p->type == 3)
        return 1;
    return 0;
}

static inline void Remove_00452cc0(Player* p)
{
    p->SetType(0);
    p->active = 0;
    p->id = -1;
    p->lobbyDataSynced = 0;
}

// FUNCTION: 0x452cc0
void __stdcall RemovePlayer(int id)
{
    Player* p = FindPlayerByDpid(id);
    if (p == 0)
        return;
    if (p->active == 0)
        return;
    if (p->type != 1 && p->type != 2 && p->type != 3)
        return;
    if (p->index == 10)
        return;

    unsigned char slot = p->index;
    int f = p->info->flags_97;
    p->info->flags_97 |= 0;
    int host = f & 1;

    for (int i = 0; i < 10; i++) {
        Player* q = &g_game->players[i];
        if (IsConnected(q)) {
            q->alliedBy[slot] = 0;
            q->allied[slot] = 0;
        }
    }

    KillPlayerUnits(FindSlotByDpid(id));

    if (g_game->bits_2a44.bit2) {
        if (!IsConnected(p))
            Remove_00452cc0(p);
    } else {
        if (IsConnected(p))
            host_net_remove_player((host_net*)g_game->net_14, p->id);
        Remove_00452cc0(p);
    }
    g_game->numPlayers--;
    p->info->flags_9d_wide &= 0xfffb;
    memset(&p->allied, 0, 11);

    if (g_game->net->GetGameType() == 3)
        ReportGameEvent(3);

    if ((g_game->flags_2a44 & 4) && host != 0) {
        unsigned int best = 0;
        for (int j = 0; j < 10; j++) {
            if (IsType3_00452cc0(&g_game->players[j]) || IsType1_00452cc0(&g_game->players[j])) {
                if (g_game->players[j].id > best)
                    best = g_game->players[j].id;
            }
        }
        Player* r = FindPlayerByDpid(best);
        if (r != 0)
            r->info->flags_97 |= 1;
    }
}

static inline unsigned char FindIndex_00453010(int id)
{
    for (unsigned char i = 0; i < 10; i++) {
        if (GetSlotDpid(i) == id)
            return i;
    }
    return 10;
}

static inline int FindActiveId_00453010()
{
    int i = 0;
    while (1) {
        if (g_game->players[i].active != 0
            && (g_game->players[i].type == 1 || g_game->players[i].type == 2))
            return g_game->players[i].id;
        i++;
        if (i >= 10)
            return -1;
    }
}
// FUNCTION: 0x453010
int __stdcall RejectPlayer(int id, unsigned char value)
{
    int result = 0;

    unsigned char fi;
    if (id == -1)
        fi = 10;
    else
        fi = FindIndex_00453010(id);

    Player* p;
    if (fi == 10)
        p = 0;
    else
        p = &g_game->players[FindSlotByDpid(id)];

    if (p == 0)
        return 0;

    PlayerIdPacket* msg = (PlayerIdPacket*)g_game->recvPacketPtr;
    msg->type = 0x1b;
    msg->id = -1;
    msg->value = value;

    if (p->active != 0
        && (p->type == 1 || p->type == 2)
        && p->rejectReason == 0) {
        if (p->type == 1) {
            for (int i = 0; i < 10; i++) {
                if (g_game->players[i].active != 0
                    && (g_game->players[i].type == 1 || g_game->players[i].type == 2)) {
                    msg->id = g_game->players[i].id;
                    BroadcastPacket(FindActiveId_00453010(), msg, 6);
                    RemovePlayer(p->id);
                    g_game->players[i].rejectReason = value;
                }
            }
            result = 1;
        } else {
            msg->id = p->id;
            BroadcastPacket(FindActiveId_00453010(), msg, 6);
            RemovePlayer(p->id);
            result = 1;
        }
    } else if (p->active != 0 && p->type == 3 && p->rejectReason == 0) {
        msg->id = id;
        result = BroadcastPacket(FindActiveId_00453010(), msg, 6);
        if (p->active != 0 && p->type == 3 && p->info->kind == 1) {
            unsigned char c = p->lobbyDataSynced;
            for (int i = 0; i < 10; i++) {
                if (g_game->players[i].lobbyDataSynced == c) {
                    RemovePlayer(g_game->players[i].id);
                    g_game->players[i].rejectReason = value;
                }
            }
        } else {
            RemovePlayer(p->id);
        }
    }

    p->rejectReason = value;
    return result;
}

// FUNCTION: 0x453320
void __stdcall SendProbe(int from, int to)
{
    unsigned char* buf = g_game->recvPacketPtr;
    *buf = 6;
    if (to == 0) {
        BroadcastPacket(from, buf, 1);
    } else {
        SendPacketToPlayer(from, to, buf, 1);
    }
}

static inline int FindTarget(Game* game)
{
    for (int i = 0; i < 10; i++) {
        if (game->players[i].type == 1)
            return game->players[i].id;
    }
    return -1;
}

// FUNCTION: 0x453360
int __stdcall SendChatPacket(char* text)
{
    int result;
    int i;
    g_game->chat->type = 5;
    strncpy(g_game->chat->text, text, 0x40);

    int target = FindTarget(g_game);

    if (text[0] == '+' || g_game->mode_2bf0 == 0) {
        result = BroadcastPacket(target, g_game->recvPacketPtr, 0x41);
    } else if (g_game->mode_2bf0 == 3) {
        for (i = 0; i < 10; i++) {
            Game* const g = g_game;
            if (g->chatRecipients[i] != 0) {
                int id = g_game->players[i].id;
                if (id != 0)
                    result = SendPacketToPlayer(target, id, g_game->recvPacketPtr, 0x41);
            }
        }
    } else {
        Player* lp = &g_game->players[g_game->localPlayer];
        for (int i = 0; i < 10; i++) {
            Player* p = &g_game->players[i];
            if (p->active != 0 && p->type == 3) {
                if ((g_game->mode_2bf0 == 1 && lp->allied[i] != 0) ||
                    (g_game->mode_2bf0 == 2 && lp->allied[i] == 0))
                    result = SendPacketToPlayer(target, p->id, g_game->recvPacketPtr, 0x41);
            }
        }
    }
    return result;
}

// FUNCTION: 0x4534e0
int __stdcall ReceiveNetPacket(void)
{
    int size;

    if (!(g_game->netGameFlags & 1))
        return 0;

    size = g_game->recvPacketSize;
    if (g_usePacketManager != 0) {
        while (1) {
            int result = g_packetManagerReceiver.ReceiveFrame(g_game->net_14, g_game->recvPacketPtr, &size);
            if (result == 0) {
                CountMessage(*g_game->recvPacketPtr, size, 0);
                return 1;
            }
            if (result == 0x887700be)
                return 0;
            if (result != 0x8877001e)
                return 0;
            g_game->recvPacketSize = size;
            g_game->recvPacketPtr = (unsigned char*)GameReallocTagged(g_game->recvPacketPtr, "PACKET DATA AGAIN", size);
        }
    } else {
        while (1) {
            int result = host_net_receive_packet((host_net*)g_game->net_14, g_game->recvPacketPtr, (unsigned long*)&size);
            if (result == 0) {
                CountMessage(*g_game->recvPacketPtr, size, 0);
                CountPacket(size, 0, 0);
                return 1;
            }
            if (result == 0x887700be)
                return 0;
            if (result != 0x8877001e)
                return 0;
            g_game->recvPacketSize = size;
            g_game->recvPacketPtr = (unsigned char*)GameReallocTagged(g_game->recvPacketPtr, "PACKET DATA AGAIN", size);
        }
    }
}

// FUNCTION: 0x453640
void UpdateTimeoutDialog()
{
    Entry_00453640* entry = (Entry_00453640*)FindGadgetChecked(((Holder_00453640*)g_game->layer)->entries, "OUTPUT");

    if (g_loungeRefreshTime < GetTicks()) {
        g_loungeRefreshTime = GetTicks() + 2;
        MarkChanged((Dialog *)g_game->message);
    }

    memset(g_loungeChatter, 0, 0xa00);

    int tail = g_game->tail;
    int count = entry->count;
    int idx = tail;
    for (int n = 1; n < count - 1; n++) {
        if (idx == g_game->head)
            break;
        idx--;
        if (idx < 0)
            idx = 29;
    }

    char* p = g_loungeChatter;
    if (idx != tail) {
        do {
            strcpy(p, g_game->entries[idx].text);
            p += strlen(g_game->entries[idx].text) + 1;
            idx++;
            if (idx >= 30)
                idx = 0;
        } while (idx != g_game->tail);
    }

    unsigned char found = FindSlotByDpid(g_timeoutPlayerDpid);
    Player* player;
    if (found == 10)
        player = 0;
    else
        player = &g_game->players[FindSlotByDpid(g_timeoutPlayerDpid)];

    if (player != 0 && player->active != 0 && player->type == 3) {
        int elapsed = (GetTicks() - player->lastHeard) / 30;
        char buf[200];
        sprintf(buf, Translate("will be rejected in %d seconds"),
                g_game->lobbyTimeoutSecs - elapsed + 0x78);
        SetTranslatedTextByName(g_game->message, "TIMETEXT", buf, 0);
        MarkChanged((Dialog *)g_game->message);
        if (elapsed < g_game->lobbyTimeoutSecs + 0x78)
            return;
        RejectPlayer(g_timeoutPlayerDpid, 6);
        CloseTopScreen((Gui *)g_game->message);
    }
}

// FUNCTION: 0x4538f0
void __stdcall HandleTimeoutDialog(Gui* gadget)
{
    Entry_004538f0* entries = (Entry_004538f0*)gadget->layer->entries;

    if (gadget->hotGadgetIndex == -1) {
        PlaySoundByName("Previous", 0);
        if (g_loungeChatter)
            GameFreeThunk(g_loungeChatter);
        g_loungeChatter = 0;
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gadget, "TALK")) {
        Entry_004538f0* entry = FindGadgetChecked_B(entries, "TALK");
        if (strlen(entry->text)) {
            SendChatMessage(&g_game->players[g_game->localPlayer], entry->text, 4, 0);
            g_game->bits_2bee_1.dirty = 1;
            strcpy(entry->text, DAT_005119b8);
        }
        BeginTextEdit((Gui *)g_game->message, FindGadgetIndex(((Layer*)g_game->layer)->entries, "TALK", 3));
        ClearSelectedGadget(g_game->message);
        MarkChanged((Dialog *)g_game->message);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gadget, "REJECT")) {
        RejectPlayer(g_timeoutPlayerDpid, 6);
        return;
    }
    ClearSelectedGadget(gadget);
}

static __inline unsigned char FindSlot_00453a50(int id)
{
    unsigned char i;
    for (i = 0; i < 10; i++) {
        int v;
        if (i == 10) {
            v = -1;
        } else {
            v = g_game->players[i].type ? g_game->players[i].id : -1;
        }
        if (v == id) {
            return i;
        }
    }
    return 10;
}

// FUNCTION: 0x453a50
void __stdcall OpenTimeoutDialog(int id)
{
    if (IsScreenNamed((Gui *)g_game->menu, "TIMEOUT.GUI")) {
        if (id == -1) {
            CloseTopScreen((Gui *)g_game->menu);
        }
        return;
    }
    if (id == -1) {
        return;
    }

    int i = FindSlot_00453a50(id);
    if (i == 10) {
        return;
    }

    Gui_00453a50* gui = (Gui_00453a50*)LoadGuiLayer(g_game->menu,
                                                    "TIMEOUT.GUI", 0x800);
    void* entries = gui->entries;
    gui->callback = (void (__stdcall *)(void *))&HandleTimeoutDialog;
    g_timeoutPlayerDpid = id;

    char* p = (char*)GameAllocIgnoreTag("LOUNGE CHATTER", 0xa00);
    g_loungeChatter = p;
    memset(p, 0, 0x780);

    OutEntry_00453a50* out = (OutEntry_00453a50*)FindGadgetChecked(entries, "OUTPUT");
    ConfigureListBoxByName(g_game->menu, "OUTPUT", g_loungeChatter,
                 (int)out->height / (GetFontLineHeight() + 2), 0);

    gui->onFrame = &UpdateTimeoutDialog;
    BeginTextEdit((Gui *)g_game->menu, FindGadgetIndex(entries, "TALK", 3));

    SetTranslatedTextByName(g_game->menu, "NAME",
                 g_game->players[i].name, 0);
    SetKeyboardInput(g_game->menu, 1);
    RenderLayer((Gui *)g_game->menu, 0x40);
}

static inline int IsTimedOut(Player* p, unsigned int now)
{
    if (p->active != 0 && p->type == 3) {
        unsigned int last = p->lastHeard;
        if (g_timeoutTimerStart > last)
            last = g_timeoutTimerStart;
        if (now - last > (unsigned int)(g_game->lobbyTimeoutSecs * 30))
            return 1;
    }
    return 0;
}

// FUNCTION: 0x453c20
void CheckPlayerTimeouts()
{
    if (g_game->uiOptionFlags & 1)
        return;
    if (g_game->pauseFlags & 1) {
        g_timeoutTimerStart = GetTicks();
        return;
    }
    unsigned int now = GetTicks();
    int mixed = 0;
    int team = -1;
    int i;
    Player* p = g_game->players;
    for (i = 0; i < 10; i++, p++) {
        if (IsTimedOut(p, now)) {
            if (team < 0)
                team = p->lobbyDataSynced;
            else if (team != p->lobbyDataSynced)
                mixed = 1;
        }
    }
    for (i = 0; i < 10; i++) {
        Player* q = &g_game->players[i];
        if (IsTimedOut(q, now) && !mixed) {
            OpenTimeoutDialog(q->id);
            return;
        }
    }
    OpenTimeoutDialog(-1);
}

static inline int FirstJoinedId()
{
    for (int i = 0; i < 10; i++) {
        if (g_game->players[i].type == 1)
            return g_game->players[i].id;
    }
    return -1;
}

static inline unsigned char* InfoPacket(unsigned char* buf, Player* p)
{
    SideDataPacket* pkt = (SideDataPacket*)buf;
    memcpy(&pkt->data, p->info, 0xb9);
    pkt->data.id = p->id;
    pkt->type = 0x20;
    return buf;
}

static inline int IsValid(Player* p)
{
    return p->active && (p->type == 1 || p->type == 2 || p->type == 3);
}

static inline int InGame(Player* p)
{
    return IsValid(p) && p->index != 10;
}

static inline Player* LocalPlayer()
{
    return &g_game->players[g_game->localPlayer];
}

static inline Unit* UnitAt(unsigned short index)
{
    if (index == 0)
        return 0;
    return &g_game->units[index];
}

static inline unsigned char FreeTeam()
{
    for (int team = 1; team <= 10; team++) {
        int used = 0;
        for (int i = 0; i < 10; i++) {
            Player* p = &g_game->players[i];
            if (IsValid(p) && p->index != 10 && p->lobbyDataSynced == team)
                used = 1;
        }
        if (!used)
            return team;
    }
    return 0;
}

static inline void DropPlayer(int id)
{
    if (FindSlotByDpid(id) == 10)
        return;
    PlayerIdPacket* out = (PlayerIdPacket*)g_game->recvPacketPtr;
    out->type = 0x1c;
    out->id = id;
    Player* p = &g_game->players[FindSlotByDpid(id)];
    if ((p->active && p->type == 3) || !((*(volatile union LoadFlags_00497f40*)&g_game->loadingFlags).value & 1) || ((*(volatile union LoadFlags_00497f40*)&g_game->loadingFlags).value & 2))
        RemovePlayer(id);
    BroadcastPacket(LocalPlayer()->id, out, 5);
}

static inline int CommandAllowed(unsigned char* bytes)
{
    int mode = g_game->mode_391f1;
    if (mode == 5 && (g_packetModes[bytes[0]] & 2))
        return 1;
    if (mode == 6 && (g_packetModes[bytes[0]] & 4))
        return 1;
    if (mode != 5 && mode != 6 && (g_packetModes[bytes[0]] & 1))
        return 1;
    return 0;
}

// FUNCTION: 0x453d40
int HandleNetPackets()
{
    if (!(g_game->netGameFlags & 1))
        return 0;
    int messages = 0;
    for (unsigned char i = 0; i < 10; i++)
        g_game->players[i].messages = 0;
    unsigned char* packet = g_game->recvPacketPtr;
    int more = 1;
    while (more) {
        more = ReceiveNetPacket();
        if (!more)
            continue;
        int sender = g_game->from_id;
        unsigned char from = FindSlotByDpid(sender);
        unsigned char to = FindSlotByDpid(g_game->lobby1);
        Player* player = &g_game->players[from];
        Player* recipient = &g_game->players[to];
        messages++;
        if (sender == 0) {
            if (!IsConnected(recipient))
                continue;
            if (recipient->type != 1)
                continue;
            Packet_00453d40* msg = (Packet_00453d40*)packet;
            switch (msg->type) {
            case 5: {
                if (msg->playerType != 1)
                    break;
                Player* p = FindPlayerByDpid(msg->id);
                if (!p)
                    break;
                if (!InGame(p))
                    break;
                if (!(g_game->netGameFlags & 4) && (p->info->flags_97 & 1) && p->type == 3) {
                    RejectPlayer(p->id, 1);
                    RejectPlayer(LocalPlayer()->id, 10);
                    p->SetType(0);
                    LocalPlayer()->SetType(0);
                } else {
                    RejectPlayer(p->id, 1);
                    p->SetType(0);
                }
                g_game->bits_2bee_1.dirty = 1;
                if (LocalPlayer()->info->flags_97 & 1) {
                    char name[32];
                    int a, b, c, d;
                    BuildGameInfo(name, &d, &c, &b, &a);
                    if (LocalPlayer()->info->started)
                        g_game->settings.flags_475 |= 0x20;
                    host_net_update_game_info((host_net*)g_game->session, name, DAT_005119b8, d, c, b, a);
                }
                break;
            }
            case 3: {
                if (!AddNetPlayer(msg->id))
                    break;
                int target = FindSlotByDpid(msg->id);
                if (!g_game->players[FindLobbyHostPlayer()].IsPlayableSlot())
                    break;
                PlayerInfo* info = LocalPlayer()->info;
                JoinRequestData* payload = msg->createData;
                if (info->closed) {
                    RejectPlayer(GetSlotDpid(target), 3);
                    break;
                }
                if (msg->dataSize != 0x15) {
                    RejectPlayer(GetSlotDpid(target), 8);
                    break;
                }
                if (payload->versionMajor != 0 || payload->versionMinor != 0x50) {
                    RejectPlayer(GetSlotDpid(target), 8);
                    break;
                }
                if (!(info->flags_9d & 1))
                    break;
                if (payload && !_strcmpi(g_game->password, payload->name))
                    break;
                RejectPlayer(GetSlotDpid(target), 4);
                break;
            }
            case 0x102: {
                if (msg->playerType != 1)
                    break;
                Player temp;
                Player* p = FindPlayerByDpid(msg->id);
                if (p) {
                    int target = FindSlotByDpid(msg->id);
                    if (target == 10) {
                        temp.FreeSideDataAndFogSightCounts();
                        continue;
                    }
                    char* payload = msg->data;
                    memcpy(g_game->players[target].info, payload, 0xb9);
                    if (FindLobbyHostPlayer() == g_game->localPlayer && !(LocalPlayer()->info->flags_9b & 0x80)
                        && (payload[0x9b] & 0x40))
                        RejectPlayer(p->id, 9);
                }
                temp.FreeSideDataAndFogSightCounts();
                break;
            }
            case 0x104:
                if (FindLobbyHostPlayer() != g_game->localPlayer)
                    memcpy(&g_game->settings, &msg->playerType, sizeof(Settings));
                break;
            case 0x103: {
                if (msg->playerType != 1)
                    break;
                Player* p = FindPlayerByDpid(msg->id);
                if (p) {
                    strncpy(p->name, msg->longName, 0x1e);
                    strncpy(p->fullName, (char*)msg->dataSize, 0x1e);
                }
                break;
            }
            }
            continue;
        }
        if (!CommandAllowed(packet))
            continue;
        if (IsConnected(player))
            continue;
        if (!InGame(player)) {
            RejectPlayer(sender, 6);
            continue;
        }
        if (packet[0] <= 1 && packet[0] >= 0x2d) {
            RejectPlayer(sender, 6);
            continue;
        }
        if (player->type != 1 && player->type != 2 && player->type != 3)
            continue;
        if (!InGame(recipient))
            continue;
        player->lastHeard = GetTicks();
        player->messages++;
        switch (packet[0]) {
        case 32: {
            SideDataPacket* infoPacket = (SideDataPacket*)packet;
            int target = FindSlotByDpid(infoPacket->data.id);
            if (target == 10)
                break;
            if (g_game->players[target].active && g_game->players[target].type == 3) {
                memcpy(g_game->players[target].info, &infoPacket->data, 0xb9);
                CheckDuplicatePlayerIds();
            }
            break;
        }
        case 23:
            if (LocalPlayer()->info->flags_97 & 1) {
                if (!IsColorFree(g_game->from_id, (signed char)packet[1])) {
                    AssignPlayerColor(FirstJoinedId(), g_game->from_id, (signed char)packet[1]);
                } else {
                    unsigned char reply[2];
                    reply[0] = 0x18;
                    reply[1] = packet[1];
                    SendPacketToPlayer(FirstJoinedId(), g_game->from_id, reply, 2);
                    if (g_usePacketManager)
                        g_packetManager.SendAllQueued(1);
                }
            }
            break;
        case 24:
            g_game->players[to].info->color = packet[1];
            if (to == g_game->localPlayer && (g_game->netGameFlags & 1)) {
                for (int i = 0; i < 10; i++) {
                    Player* p = &g_game->players[i];
                    unsigned char reply[0xba];
                    if (IsConnected(p)) {
                        BroadcastPacket(p->id, InfoPacket(reply, p), 0xba);
                        BroadcastAllyTeam(p);
                    }
                }
                SendLobbySyncRequests();
                g_packetManager.SendAllQueued(1);
            }
            g_game->bits_2bee_1.dirty = 1;
            break;
        case 2:
            HandlePing((LatencyPacket *)packet);
            break;
        case 38:
            memcpy(g_game->playerIds, packet + 1, 40);
            g_game->bits_2bee_1.dirty = 1;
            break;
        case 35: {
            AllyFlagsPacket* ally = (AllyFlagsPacket*)packet;
            Player* a = FindPlayerByDpid(ally->fromNetId);
            Player* b = FindPlayerByDpid(ally->toNetId);
            if (!a || !b)
                break;
            if (ally->allied)
                PlaySoundByName(g_allySoundName, 0);
            if (IsConnected(b)) {
                SetAlliance(ally->fromNetId, ally->toNetId, ally->allied,
                             ally->force);
                if (!(g_game->netGameFlags & 4))
                    g_game->bits_2bee_1.dirty = 1;
                else
                    RebuildAllyList();
            }
            g_game->players[a->index].allied[b->index] = ally->allied;
            break;
        }
        case 36: {
            PlayerIdPacket* allyTeam = (PlayerIdPacket*)packet;
            Player* p = FindPlayerByDpid(allyTeam->id);
            if (p)
                p->alliance = packet[5];
            if (!(g_game->netGameFlags & 4))
                g_game->bits_2bee_1.dirty = 1;
            break;
        }
        case 27: {
            PlayerIdPacket* reject = (PlayerIdPacket*)packet;
            Player* p = FindPlayerByDpid(reject->id);
            if (p)
                RejectPlayer(p->id, packet[5]);
            break;
        }
        case 28: {
            PlayerIdPacket* drop = (PlayerIdPacket*)packet;
            int id = drop->id;
            if (FindSlotByDpid(id) == 10)
                break;
            char text[200];
            sprintf(text, Translate(g_playerDisconnectedText), FindPlayerByDpid(id)->name);
            AddMessage(text, 4, 0, from);
            DropPlayer(drop->id);
            if (drop->id == FirstJoinedId()) {
                g_game->bits_3923b_2.bit2_3923b = 1;
                g_game->bits_3923b_2.bit4_3923b = 0;
            }
            break;
        }
        case 30: {
            unsigned char reply[5];
            PlayerIdPacket* replyPacket = (PlayerIdPacket*)reply;
            replyPacket->type = 0x1f;
            recipient->startPos = packet[1];
            replyPacket->id = recipient->id;
            SendPacketToPlayer(recipient->id, player->id, reply, 5);
            break;
        }
        case 31: {
            PlayerIdPacket* ack = (PlayerIdPacket*)packet;
            int target = FindSlotByDpid(ack->id);
            if (target != 10)
                g_game->startPosAssignAck[target] = 1;
            break;
        }
        case 5:
            if (recipient->active && recipient->type == 1)
                AddMessage((char*)(packet + 1), 8, 0, from);
            break;
        case 39: {
            PlayerIdPacket* breach = (PlayerIdPacket*)packet;
            Player* p = FindPlayerByDpid(breach->id);
            if (!p)
                break;
            from = p->index;
            char text[256];
            sprintf(text, g_playerMessageFormat, p->name, Translate(g_integrityBreachText));
            for (int i = 0; i < 12; i++)
                AddMessage(text, 8, 0, from);
            break;
        }
        case 6: {
            unsigned char reply = 7;
            SendPacketToPlayer(FindConnectedPlayerId(), g_game->from_id, &reply, 1);
            break;
        }
        case 7:
            player->keepaliveFlags |= 1;
            break;
        case 8:
            more = 0;
            g_game->netGameFlags |= 4;
            break;
        case 9:
            CreateUnitFromPacket(from, packet);
            break;
        case 44:
            ReceiveUnitStates(player, (unsigned int *)packet);
            break;
        case 10:
            ApplyAttachUnit((OrderView *)packet);
            break;
        case 11:
            ApplyUnitDamage((UnitDamagePacket *)packet);
            break;
        case 12:
            ApplyUnitDeath((UnitKilledPacket *)packet, 0);
            break;
        case 13:
            ApplyWeaponFirePacket(player, packet);
            break;
        case 14:
            ApplyProjectileHitPacket(player, packet);
            break;
        case 15: {
            FeatureDamagePacket* damage = (FeatureDamagePacket*)packet;
            switch (damage->sub) {
            case 0xfd:
                KillFeature(damage->x, damage->z, 0);
                break;
            case 0xfe:
                StartFeatureBurning(damage->x, damage->z, 1);
                break;
            case 0xff:
                KillFeature(damage->x, damage->z, 1);
                break;
            default:
                DamageFeature(GetMapCell(damage->x, damage->z),
                             damage->x, damage->z,
                             &g_game->weapons[damage->sub]);
                break;
            }
            break;
        }
        case 16: {
            UnitScriptCallRecv* call = (UnitScriptCallRecv*)packet;
            Unit* unit = UnitAt(call->id);
            if (unit->flags & 0x10000000)
                ((CobScript*)unit->script.get())->StartScriptWithArgsByIndex(call->index, 0, 0, call->argCount,
                                             call->arg0, call->arg1, call->arg2, call->arg3);
            break;
        }
        case 17: {
            Unit* unit = UnitAt(*(unsigned short*)(packet + 1));
            if (unit->flags & 0x10000000) {
                unit->SetStateBits(packet[3], 1);
                unit->SetStateBits((unsigned char)~packet[3], 0);
            }
            break;
        }
        case 18: {
            BuilderLinkPacket* link = (BuilderLinkPacket*)packet;
            Unit* a = UnitAt(link->constructedUnitId);
            FinishConstruction(UnitAt(link->builderUnitId), a);
            break;
        }
        case 19: {
            PlaySoundPacket* sound = (PlaySoundPacket*)packet;
            if (sound->flag)
                PlaySoundByIndex(sound->index, 0);
            else
                PlaySoundAt(sound->index, packet + 6, 0);
            break;
        }
        case 20: {
            OwnershipTransferPacket* transfer = (OwnershipTransferPacket*)packet;
            Unit* unit = UnitAt(transfer->unitId);
            if (!unit || !(unit->flags & 0x10000000))
                break;
            int id = transfer->newOwnerNetId;
            Player* p;
            if (FindSlotByDpid(id) == 10)
                p = 0;
            else
                p = &g_game->players[FindSlotByDpid(id)];
            if (IsConnected(p))
                GiveUnitToPlayer(unit, p, (OwnershipTransferPacket *)packet);
            break;
        }
        case 21:
            if (g_game->net_bits.net_bit2)
                g_game->shareVisionReady[from] = 1;
            break;
        case 22: {
            ResourceSharePacket* share = (ResourceSharePacket*)packet;
            unsigned char a = FindSlotByDpid(share->fromNetId);
            unsigned char b = FindSlotByDpid(share->toNetId);
            if (a == 10 || b == 10)
                break;
            switch (share->subtype) {
            case 1:
                TransferMetal(a, b, share->amount, 0);
                break;
            case 2:
                TransferEnergy(a, b, share->amount, 0);
                break;
            case 3:
                ShareMapInfo(a, b);
                break;
            }
            break;
        }
        case 40:
            HandlePlayerEconomy((PlayerViewStatePacket *)packet, player);
            break;
        case 41:
            if (packet[1]) {
                recipient->shareLos[from] = 1;
                if (packet[2])
                    recipient->shareMapping[from] = 1;
            }
            break;
        case 25:
            if (packet[1])
                SetGameSpeed(packet[2], 0);
            else
                g_game->bits_38a51.bit_38a51 = packet[2];
            break;
        case 26:
            if (g_game->sync && recipient->active && recipient->type == 1)
                ((UnitSync*)g_game->sync)->ReceiveSyncPacket(packet, from);
            break;
        case 29: {
            PlayerIdIntPacket* integrity = (PlayerIdIntPacket*)packet;
            if (g_usePacketManager)
                g_packetManager.HandleIntegrityNop(g_game->from_id, integrity->id,
                                                   integrity->value);
            break;
        }
        case 33: {
            LobbySyncMessage* request = (LobbySyncMessage*)packet;
            Player* a = FindPlayerByDpid(request->id);
            Player* b = FindPlayerByDpid(request->arg);
            if (!IsConnected(&g_game->players[FindLobbyHostPlayer()]))
                break;
            if (!a)
                break;
            unsigned char reply[6];
            PlayerIdPacket* replyPacket = (PlayerIdPacket*)reply;
            replyPacket->type = 0x22;
            replyPacket->id = -1;
            reply[5] = 0;
            if (!request->flag) {
                replyPacket->id = a->id;
                reply[5] = a->lobbyDataSynced;
                if (!reply[5])
                    break;
                BroadcastPacket(GetHostDpid(), reply, 6);
            } else {
                if (b) {
                    replyPacket->id = a->id;
                    if (InGame(b))
                        reply[5] = b->lobbyDataSynced;
                } else {
                    replyPacket->id = a->id;
                    if (!a->lobbyDataSynced)
                        reply[5] = FreeTeam();
                }
                if (!reply[5])
                    break;
                a->lobbyDataSynced = reply[5];
                BroadcastPacket(GetHostDpid(), reply, 6);
            }
            break;
        }
        case 34: {
            PlayerIdPacket* team = (PlayerIdPacket*)packet;
            Player* p = FindPlayerByDpid(team->id);
            if (p)
                p->lobbyDataSynced = packet[5];
            break;
        }
        case 42:
            player->progress = packet[1];
            break;
        }
    }
    CheckDuplicatePlayerIds();
    CheckPlayerTimeouts();
    return messages;
}

#pragma auto_inline(off)
// FUNCTION: 0x456030
int Player::IsPlayableSlot()
{
    if (active != 0 && (type == 1 || type == 2)) {
        return 1;
    }
    return 0;
}

#pragma auto_inline(on)

// FUNCTION: 0x456050
void __stdcall SendNewUnit(Unit* obj)
{
    UnitCreatePacket packet;
    packet.type = 9;
    packet.defIndex = ((short)obj->unitDefIndex);
    packet.unitId = ((short)obj->id);
    packet.pos = obj->pos;
    packet.angles = obj->angles;
    BroadcastPacket(obj->player->id, &packet, 0x17);
}

// FUNCTION: 0x4560c0
void __stdcall BroadcastBuilderLink(Unit* obj, Unit* target)
{
    BuilderLinkPacket packet;
    packet.type = 0x12;
    packet.constructedUnitId = ((short)target->id);
    packet.builderUnitId = ((short)obj->id);
    BroadcastPacket(obj->player->id, &packet, 5);
}

#include <algorithm>
#include <time.h>

#pragma pack(push, 1)

void Unused_004565a0(int a, int b, int c, int d, int e, int f);

struct UnitScriptCallPacket {
    unsigned char type;
    short id;
    short index;
    char argCount;
    int arg0;
    int arg1;
    int arg2;
    int arg3;
};

struct LatencyPacket {
    unsigned char type;
    int sendTick;
    int echoTick;
    int targetPlayerNetId;
};

struct Msg26_00456310 {
    unsigned char type;
    int table[10];
};

struct Msg20_00456310 {
    unsigned char type;
    PlayerInfo info;
};

struct Packet_00456de0 {
    unsigned char type;
    unsigned char progress;
};

void Unused_00457050(int a, int b, int c, int d, int e, int f);

void Unused_004571c0(int a, int b, int c, int d, int e, int f);

struct PlayerViewStatePacket {
    unsigned char type;
    unsigned char shareFlag;
    int unitsKilled;
    int unitsLost;
    int commandersKilled;
    int commandersLost;
    int metalAmount;
    int energyAmount;
    int metalStorage;
    int energyStorage;
    float energyProduced;
    float energyConsumed;
    float energyWasted;
    float metalProduced;
    float metalConsumed;
    float metalWasted;
};

void Unused_00457540(int a, int b, int c, int d, int e, int f);

struct ShareLosAckPacket {
    unsigned char type;
    unsigned char ackLos;
    unsigned char ackMapping;
};

void Unused_00457d30(int a, int b, int c, int d, int e, int f);

#pragma pack(pop)

extern char g_cmdlineHostGameName[];
extern char DAT_00512c98[];

void __stdcall SendPlayerEconomy(Player* from, Player* to, unsigned char flag);
#include "../map/map_list.h"

// FUNCTION: 0x456110
int __stdcall SendScriptCallNoArgsByName(Unit* obj, char* name)
{
    UnitScriptCallPacket packet;
    int index = ((CobScript *)obj->script.get())->FindScript(name);
    if (!(g_game->netGameFlags & 1)) {
        return 0;
    }
    packet.type = 0x10;
    packet.id = ((short)obj->id);
    packet.index = index;
    packet.argCount = 0;
    packet.arg0 = 0;
    packet.arg1 = 0;
    packet.arg2 = 0;
    packet.arg3 = 0;
    return BroadcastPacket(obj->player->id, &packet, 0x16);
}

// FUNCTION: 0x456190
int __stdcall SendScriptCallNoArgs(Unit* obj, short index)
{
    UnitScriptCallPacket packet;
    if (!(g_game->netGameFlags & 1)) {
        return 0;
    }
    packet.type = 0x10;
    packet.id = ((short)obj->id);
    packet.index = index;
    packet.argCount = 0;
    packet.arg0 = 0;
    packet.arg1 = 0;
    packet.arg2 = 0;
    packet.arg3 = 0;
    return BroadcastPacket(obj->player->id, &packet, 0x16);
}

// FUNCTION: 0x456200
int __stdcall SendScriptCallByName(Unit* obj, char* name, char argCount,
                           int arg0, int arg1, int arg2, int arg3)
{
    UnitScriptCallPacket packet;
    short index = ((CobScript *)obj->script.get())->FindScript(name);
    if (!(g_game->netGameFlags & 1)) {
        return 0;
    }
    packet.type = 0x10;
    packet.id = ((short)obj->id);
    packet.index = index;
    packet.argCount = argCount;
    packet.arg0 = arg0;
    packet.arg1 = arg1;
    packet.arg2 = arg2;
    packet.arg3 = arg3;
    return BroadcastPacket(obj->player->id, &packet, 0x16);
}

// FUNCTION: 0x456290
int __stdcall SendScriptCall(Unit* obj, short index, char argCount,
                           int arg0, int arg1, int arg2, int arg3)
{
    UnitScriptCallPacket packet;
    if (!(g_game->netGameFlags & 1)) {
        return 0;
    }
    packet.type = 0x10;
    packet.id = ((short)obj->id);
    packet.index = index;
    packet.argCount = argCount;
    packet.arg0 = arg0;
    packet.arg1 = arg1;
    packet.arg2 = arg2;
    packet.arg3 = arg3;
    return BroadcastPacket(obj->player->id, &packet, 0x16);
}

// FUNCTION: 0x456310
void SendNetHeartbeat()
{
    unsigned int now = GetTicks();
    if ((int)(now - g_game->lobbySyncTick) <= 0x3c)
        return;
    g_game->lobbySyncTick += 0x3c;

    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (p->active != 0 && (p->type == 1 || p->type == 2)) {
            if (g_usePacketManager != 0)
                g_packetManager.SendAllQueued(1);

            LatencyPacket msg;
            msg.type = 2;
            msg.sendTick = host_time_ms();
            msg.echoTick = 0;
            msg.targetPlayerNetId = p->id;

            int was = host_net_guarantee_packets(0);
            BroadcastPacket(p->id, (unsigned char*)&msg, 0xd);
            if (g_usePacketManager != 0)
                g_packetManager.SendAllQueued(1);
            if (was != 0)
                host_net_guarantee_packets(1);

            int id = p->id;
            unsigned char* buf = g_game->recvPacketPtr;
            buf[0] = 6;
            BroadcastPacket(id, buf, 1);

            if (p->info->color == 0xff)
                RequestPlayerColor(0);

            if (p->info->host & 1) {
                for (int k = 0; k < 10; k++) {
                    unsigned char st = g_game->players[k].type;
                    if (st == 4)
                        g_game->table_2c28[k] = -1;
                    else if (st == 0)
                        g_game->table_2c28[k] = 0;
                    else
                        g_game->table_2c28[k] = g_game->players[k].id;
                }

                Msg26_00456310 msg26;
                memcpy(msg26.table, g_game->table_2c28, 0x28);
                msg26.type = 0x26;
                BroadcastPacket(p->id, (unsigned char*)&msg26, 0x29);
            }
        }
    }

    Msg20_00456310 msg;
    if (g_game->netGameFlags & 1) {
        for (int i = 0; i < 10; i++) {
            Player* p = &g_game->players[i];
            if (p->active != 0 && (p->type == 1 || p->type == 2)) {
                msg.info = *p->info;
                msg.info.id = p->id;
                msg.type = 0x20;
                BroadcastPacket(p->id, (unsigned char*)&msg, 0xba);

                if (p->active != 0 && (p->type == 1 || p->type == 2)) {
                    PlayerIdPacket* buf2 = (PlayerIdPacket*)g_game->recvPacketPtr;
                    buf2->type = 0x24;
                    buf2->id = p->id;
                    buf2->value = p->alliance;
                    BroadcastPacket(p->id, buf2, 6);
                    if (g_usePacketManager != 0)
                        g_packetManager.SendAllQueued(1);
                }
            }
        }
        SendLobbySyncRequests();
        g_packetManager.SendAllQueued(1);
    }

    g_game->bits_2bee_1.dirty = 1;
}

// FUNCTION: 0x4565a0
void __stdcall HandlePing(LatencyPacket* p)
{
    if (p->echoTick == 0) {
        if (g_usePacketManager != 0) {
            g_packetManager.SendAllQueued(1);
        }
        p->echoTick = host_time_ms();
        int packets_were_guaranteed = host_net_guarantee_packets(0);
        SendPacketToPlayer(g_game->lobby1, p->targetPlayerNetId, (unsigned char *)p, 0xd);
        if (g_usePacketManager != 0) {
            g_packetManager.SendAllQueued(1);
        }
        if (packets_were_guaranteed != 0) {
            host_net_guarantee_packets(1);
        }
        return;
    }

    unsigned char index = FindSlotByDpid(p->targetPlayerNetId);
    Player* pl = &g_game->players[index];
    if (pl->active == 0) {
        return;
    }
    if (pl->type != 1 && pl->type != 2) {
        return;
    }
    unsigned char other = FindSlotByDpid(g_game->from_id);
    g_game->players[other].ping = host_time_ms() - p->sendTick;
}

static inline int IsPlaying(unsigned char i)
{
    return g_game->players[i].active != 0 && g_game->players[i].type == 3;
}

// FUNCTION: 0x456760
int AreAllPlayersReady()
{
    int count = 0;
    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (p->active != 0 && p->type == 3)
            count++;
    }
    if (count == 0)
        return 0;

    int all = 1;
    for (unsigned char j = 0; j < 10; j++) {
        Player* p = &g_game->players[j];
        if (p->active != 0 && (p->type == 1 || p->type == 2 || IsPlaying(j))) {
            if (!(g_game->players[j].info->flags_9b & 0x20))
                return 0;
        }
        if (g_game->players[j].active == 0 || !(g_game->players[j].info->flags_9b & 0x40))
            all = 0;
    }
    if (g_game->numPlayers == 1)
        return 0;
    return all == 0;
}

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

// FUNCTION: 0x4568b0
void __stdcall ReportPacketGap(int, int, int)
{
}

static inline unsigned char FindOccupied_004568c0() {
    for (unsigned char i = 0; i < 10; i++) {
        if (g_game->players[i].type != 0 && g_game->players[i].info->host)
            return i;
    }
    return 10;
}

static inline int FirstJoinedId_004568c0() {
    for (int j = 0; j < 10; j++) {
        if (g_game->players[j].type == 1)
            return g_game->players[j].id;
    }
    return -1;
}

// FUNCTION: 0x4568c0
int AssignStartPositions() {
    unsigned char idx = FindOccupied_004568c0();
    int res = g_game->players[idx].IsPlayableSlot();
    if (res != 0 && g_game->startPosShuffleReady == 0) {
        int* out = g_game->startPosShuffle;
        if (g_game->players[g_game->localPlayer].info->fixedloc) {
            int n = 0;
            for (int k0 = 0; k0 < 10; k0++) {
                Player* q = &g_game->players[k0];
                if (q->active != 0 && (q->type == 1 || q->type == 2 || q->type == 3) &&
                    q->index != 10 && (q->info->bit6) == 0)
                    out[k0] = n++;
                else
                    out[k0] = -1;
            }
        } else {
            int cand[10];
            for (int z = 0; z < 10; z++)
                cand[z] = -1;
            int n = 0;
            for (int k1 = 0; k1 < 10; k1++) {
                Player* p = &g_game->players[k1];
                if (p->active != 0 && (p->type == 1 || p->type == 2 || p->type == 3) &&
                    p->index != 10 && (p->info->bit6) == 0) {
                    cand[n] = n;
                    n++;
                }
            }
            GameShuffleDraw draw(RAND_STREAM_MAIN);
            if (n > 2 || (int)((__int64)GameRand(RAND_STREAM_MAIN) * 2 / 0x8000) != 0)
                std::random_shuffle(cand, cand + n, draw);
            int j = 0;
            for (int k2 = 0; k2 < 10; k2++) {
                Player* q2 = &g_game->players[k2];
                if (q2->active != 0 && (q2->type == 1 || q2->type == 2 || q2->type == 3) &&
                    q2->index != 10) {
                    if (g_game->players[k2].active != 0 && (q2->info->bit6))
                        out[k2] = -1;
                    else
                        out[k2] = cand[j++];
                } else {
                    out[k2] = -1;
                }
            }
        }
        g_game->startPosShuffleReady = 1;
    }
    int ok = 1;
    for (int k3 = 0; k3 < 10; k3++) {
        Player* q = &g_game->players[k3];
        if (q->active != 0 && q->type == 3) {
            if (res != 0) {
                if (g_game->shareVisionReady[k3] == 0 || g_game->startPosAssignAck[k3] == 0) {
                    ok = 0;
                    break;
                }
            }
            if (res == 0) {
                if (g_game->shareVisionReady[k3] == 0) {
                    ok = 0;
                    break;
                }
            }
        }
    }
    if (res != 0) {
        for (int k4 = 0; k4 < 10; k4++) {
            if (g_game->startPosAssignAck[k4] != 0)
                continue;
            unsigned char packet[2];
            packet[0] = 0x1e;
            packet[1] = (unsigned char)g_game->startPosShuffle[k4];
            if (g_game->players[k4].active != 0) {
                if (g_game->players[k4].type == 3) {
                    SendPacketToPlayer(FirstJoinedId_004568c0(), GetSlotDpid(k4), packet, 2);
                } else if (IsConnected(&g_game->players[k4])) {
                    g_game->players[k4].startPos = packet[1];
                    g_game->startPosAssignAck[k4] = 1;
                }
            }
        }
    }
    unsigned char pkt = 0x15;
    if (res == 0) {
        for (int k6 = 0; k6 < 10; k6++) {
            if (IsConnected(&g_game->players[k6]))
                BroadcastPacket(GetSlotDpid(k6), &pkt, 1);
        }
    } else if (ok) {
        for (int k5 = 0; k5 < 10; k5++) {
            if (IsConnected(&g_game->players[k5]))
                BroadcastPacket(GetSlotDpid(k5), &pkt, 1);
        }
    }
    return ok;
}

// FUNCTION: 0x456de0
void __stdcall SendLoadProgress()
{
    Packet_00456de0 packet;
    packet.type = 0x2a;
    int total = (*(volatile unsigned char(*)[6])&g_game->progress_38d6f)[0];
    total += (*(volatile unsigned char(*)[6])&g_game->progress_38d6f)[5];
    total += (*(volatile unsigned char(*)[6])&g_game->progress_38d6f)[4];
    total += (*(volatile unsigned char(*)[6])&g_game->progress_38d6f)[3];
    total += (*(volatile unsigned char(*)[6])&g_game->progress_38d6f)[2];
    total += (*(volatile unsigned char(*)[6])&g_game->progress_38d6f)[1];

    packet.progress = total / 6;
    for (int i = 0; i < 10; i++) {
        if (g_game->players[i].active != 0
            && (g_game->players[i].type == 1 || g_game->players[i].type == 2)) {
            g_game->players[i].progress = packet.progress;
            BroadcastPacket(GetSlotDpid(i), &packet, 2);
        }
    }
}

static inline int PlayerReady(Player* p)
{
    return p->active != 0
        && (p->type == 1 || p->type == 2 || p->type == 3)
        && p->index != 10
        && (p->unitCount != 0 || p->unitsCreated == 0);
}

// FUNCTION: 0x456ee0
void __stdcall SendShareMetal(unsigned char from, unsigned char to, int amount)
{
    Player* a = &g_game->players[from];
    Player* b = &g_game->players[to];

    if (PlayerReady(a) && PlayerReady(b)) {
        ResourceSharePacket packet;
        packet.type = 0x16;
        packet.subtype = 1;
        packet.fromNetId = GetSlotDpid(from);
        packet.toNetId = GetSlotDpid(to);
        packet.amount = amount;
        SendPacketToPlayer(GetSlotDpid(from), GetSlotDpid(to), (unsigned char *)&packet, sizeof(packet));
    }
}

// FUNCTION: 0x457050
void __stdcall SendShareEnergy(unsigned char from, unsigned char to, int value)
{
    Player* first = &g_game->players[from];
    Player* second = &g_game->players[to];
    if (first->active != 0
        && (first->type == 1 || first->type == 2 || first->type == 3)
        && first->index != 10
        && (first->unitCount != 0 || first->unitsCreated == 0)
        && second->active != 0
        && (second->type == 1 || second->type == 2 || second->type == 3)
        && second->index != 10
        && (second->unitCount != 0 || second->unitsCreated == 0)) {
        ResourceSharePacket packet;
        packet.type = 0x16;
        packet.subtype = 2;
        packet.fromNetId = GetSlotDpid(from);
        packet.toNetId = GetSlotDpid(to);
        packet.amount = value;
        SendPacketToPlayer(GetSlotDpid(from), GetSlotDpid(to), (unsigned char *)&packet, sizeof(packet));
    }
}

static inline int PlayerDpid_00457d30(unsigned char i)
{
    if (g_game->players[i].type != 0)
        return g_game->players[i].id;
    return -1;
}

// FUNCTION: 0x4571c0
void __stdcall SendShareMapInfo(unsigned char from, unsigned char to)
{
    if (from != 10 && to != 10) {
        ResourceSharePacket packet;
        packet.type = 0x16;
        packet.subtype = 3;
        packet.fromNetId = PlayerDpid_00457d30(from);
        packet.toNetId = PlayerDpid_00457d30(to);
        packet.amount = 0;
        SendPacketToPlayer(PlayerDpid_00457d30(from), PlayerDpid_00457d30(to), (unsigned char *)&packet, sizeof(packet));
    }
}

// FUNCTION: 0x4572a0
int BroadcastPendingViewState()
{
    int result = 1;
    for (int i = 0; i < 10; i++) {
        Player* pi = &g_game->players[i];
        if (pi->active == 0)
            continue;
        if (pi->type != 1 && pi->type != 2)
            continue;
        if (pi->unitsCreated == 0)
            continue;
        if (pi->rejectReason != 0)
            continue;
        for (int j = 0; j < 10; j++) {
            Player* pj = &g_game->players[j];
            if (pj->active != 0 && pj->type == 3 || pj->unitsCreated == 0
                || pj->rejectReason != 0) {
                if (pj->active != 0 && pj->type == 3) {
                    if (pj->info->kind == 1
                        && (pi->shareLos[pj->index] == 0
                            || pi->shareMapping[pj->index] == 0
                            || pi->shareVision[pj->index] == 0))
                        goto send;
                }
                if (pj->active != 0 && pj->type == 3) {
                    if (pi->shareVision[pj->index] == 0) {
                        SendPlayerEconomy(pi, pj, 1);
                        result = 0;
                    }
                }
                continue;
            send:
                SendPlayerEconomy(pi, pj, 1);
                result = 0;
            }
        }
    }
    host_sleep_ms(0xfa);
    return result;
}

// FUNCTION: 0x4573d0
void __stdcall SendPlayerEconomy(Player* player, Player* target,
                            unsigned char flag)
{
    if (player->active == 0)
        return;
    if (player->type != 1 && player->type != 2)
        return;
    if (player->rejectReason != 0)
        return;

    PlayerViewStatePacket packet;
    packet.type = 0x28;
    packet.shareFlag = flag;
    packet.unitsKilled = player->kills;
    packet.unitsLost = player->losses;
    packet.commandersKilled = player->commanderKills;
    packet.commandersLost = player->commanderLosses;
    packet.metalAmount = player->metalBits;
    packet.energyAmount = player->energyBits;
    packet.metalStorage = player->metalCapacityBits;
    packet.energyStorage = player->energyCapacityBits;
    packet.energyProduced = (float)player->totalEnergyProduced;
    packet.energyConsumed = (float)player->totalEnergyConsumed;
    packet.energyWasted = (float)player->energyWasted;
    packet.metalProduced = (float)player->totalMetalProduced;
    packet.metalConsumed = (float)player->totalMetalConsumed;
    packet.metalWasted = (float)player->metalWasted;

    if (target != 0) {
        if (target->rejectReason == 0)
            SendPacketToPlayer(player->id, target->id, (unsigned char *)&packet, 0x3a);
        return;
    }

    for (int i = 0; i < 10; i++) {
        if (g_game->players[i].active == 0)
            continue;
        if (g_game->players[i].type != 3)
            continue;
        if (g_game->players[i].info->kind != 1)
            continue;
        if (g_game->players[i].rejectReason != 0)
            continue;
        SendPacketToPlayer(player->id, g_game->players[i].id, (unsigned char *)&packet, 0x3a);
    }
}

// FUNCTION: 0x457540
void __stdcall HandlePlayerEconomy(PlayerViewStatePacket* packet, Player* player)
{
    if (player == 0)
        return;
    if (player->rejectReason != 0)
        return;

    int found = 0;
    for (int i = 0; i < 10; i++) {
        if (g_game->players[i].active != 0
            && (g_game->players[i].type == 1 || g_game->players[i].type == 2)
            && g_game->players[i].shareVision[player->index] != 0)
            found = 1;
    }

    if (found == 0) {
        player->kills = *(short*)&packet->unitsKilled;
        player->losses = *(short*)&packet->unitsLost;
        player->commanderKills = *(short*)&packet->commandersKilled;
        player->commanderLosses = *(short*)&packet->commandersLost;
        player->metalBits = packet->metalAmount;
        player->energyBits = packet->energyAmount;
        player->metalCapacityBits = packet->metalStorage;
        player->energyCapacityBits = packet->energyStorage;
        player->totalEnergyProduced = packet->energyProduced;
        player->totalEnergyConsumed = packet->energyConsumed;
        player->energyWasted = packet->energyWasted;
        player->totalMetalProduced = packet->metalProduced;
        player->totalMetalConsumed = packet->metalConsumed;
        player->metalWasted = packet->metalWasted;
    }

    if (packet->shareFlag == 0)
        return;

    ShareLosAckPacket team;
    team.type = 0x29;
    team.ackLos = 1;

    {
    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (p->active == 0)
            continue;
        if (p->type != 1 && p->type != 2)
            continue;
        if (p->rejectReason != 0)
            continue;
        p->shareVision[player->index] = 1;
        if (p->shareLos[player->index] != 0)
            team.ackMapping = 1;
        else
            team.ackMapping = 0;
        SendPacketToPlayer(p->id, player->id, (unsigned char *)&team, 3);
        if (g_game->list_391ed->CheckVictory() != 0)
            continue;
        if (p->shareLos[player->index] != 0)
            continue;
        SendPlayerEconomy(p, player, 1);
    }
    }
}

// FUNCTION: 0x457710
int InitLobbiedConnection()
{
    if (g_game->dplConnection != 0)
        return 1;

    if (host_net_init_lobbied_connection((host_net*)g_game->net_14)) {
        host_net_init_defaults((host_net*)g_game->net_14);
        g_game->netMode = 10;
        if (InitPacketManager(2, 100)) {
            SetMissionType(3);
            char* name = g_game->dplConnection->desc->lpszSessionName;
            if (name != 0) {
                strncpy(g_game->net_14, name, 0x10);
                return 1;
            }

            char username[16];
            username[0] = 0;
            if (g_cmdlineHostGameName[0] != 0) {
                strncat(username, g_cmdlineHostGameName, 0x10);
            } else if (DAT_00512c98[0] != 0) {
                strncat(username, DAT_00512c98, 0x10);
            } else {
                unsigned int size = host_user_name(username, 0xf);
                if (size != 0)
                    username[size] = 0;
                else
                    strcpy(username, "TotalA");
            }

            time_t now = time(0);
            struct tm* local = localtime(&now);
            int len = strlen(username);
            if (15 - len >= 7)
                sprintf(username + len, "_%02d%02d%02d", local->tm_hour, local->tm_min, local->tm_sec);
            strcpy(g_game->net_14, username);
            return 1;
        }
    }
    return 0;
}

// FUNCTION: 0x4578d0
void __stdcall SetPacketRate(int rate)
{
    g_packetManager.SetDefaultSendPacing(rate);
}

// FUNCTION: 0x4578f0
void __cdecl LeaveNetGameCallback(int)
{
    if (g_game->bits_2a44_2a44.leaving) {
        for (int i = 0; i < 10; i++) {
            if (g_game->players[i].active != 0
                && (g_game->players[i].type == 1 || g_game->players[i].type == 2))
                RemovePlayer(g_game->players[i].id);
        }
        ShutdownScoreTables();
    }
    host_net_quit_game((host_net*)g_game->net_14);
    SetCloseHandler(0, 0);
    g_game->bits.flag2 = 1;
    int reason = g_game->players[g_game->localPlayer].rejectReason;
    char* text;
    if (reason) {
        switch (reason) {
        case 4:
            text = "You did not have the correct password";
            break;
        case 3:
            text = "The game is closed";
            break;
        case 5:
            text = "The game is full";
            break;
        case 6:
            text = "You have lost connection with the game";
            break;
        case 7:
            text = "You need a unit you don't have for this game";
            break;
        case 8:
            text = "You need a newer version of the game to enter";
            break;
        case 9:
            text = "No watching is allowed for this game";
            break;
        case 10:
            text = "The creator has left the game";
            break;
        default:
            text = "You were rejected from the game";
            break;
        }
    }
    else {
        text = 0;
    }
    QuitApp(text);
}

static inline unsigned char FindPlayer_00457a50()
{
    for (unsigned char i = 0; i < 10; i++) {
        if (g_game->players[i].type != 0 && (g_game->players[i].info->flags_97 & 1))
            return i;
    }
    return 10;
}

static inline int IsHuman(Player* p)
{
    if (p->active != 0 && (p->type == 1 || p->type == 2))
        return 1;
    return 0;
}

// FUNCTION: 0x457a50
int IsHostLocal()
{
    return IsHuman(&g_game->players[FindPlayer_00457a50()]);
}

// FUNCTION: 0x457af0
int CountHumanPlayers()
{
    int count = 0;
    for (int i = 0; i < 10; i++) {
        if ((g_game->players[i].active != 0 && g_game->players[i].type == 1)
            || (g_game->players[i].active != 0 && g_game->players[i].type == 3
                && g_game->players[i].info->kind == 1))
            count++;
    }
    return count;
}

// FUNCTION: 0x457b40
int CountComputerPlayers()
{
    int count = 0;
    for (int i = 0; i < 10; i++) {
        if ((g_game->players[i].active != 0 && g_game->players[i].type == 2)
            || (g_game->players[i].active != 0 && g_game->players[i].type == 3
                && g_game->players[i].info->kind == 2))
            count++;
    }
    return count;
}

// FUNCTION: 0x457b90
int CountLocalComputerPlayers()
{
    int count = 0;
    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (p->active != 0 && p->type == 2)
            count++;
    }
    return count;
}

// FUNCTION: 0x457bc0
int CountActiveAIPlayers()
{
    int count = 0;
    for (int i = 0; i < 10; i++) {
        if (g_game->players[i].active != 0
            && g_game->players[i].type == 2
            && g_game->players[i].index != 10
            && (g_game->players[i].unitCount != 0 || g_game->players[i].unitsCreated == 0))
            count++;
    }
    return count;
}

// FUNCTION: 0x457c10
int CountActiveHumanOrAiPlayers()
{
    int count = 0;
    for (int i = 0; i < 10; i++) {
        if (g_game->players[i].active != 0
            && (g_game->players[i].type == 1 || g_game->players[i].type == 2)
            && (g_game->players[i].type == 1 || g_game->players[i].type == 2 || g_game->players[i].type == 3)
            && g_game->players[i].index != 10
            && (g_game->players[i].unitCount != 0 || g_game->players[i].unitsCreated == 0))
            count++;
    }
    return count;
}

// FUNCTION: 0x457c80
int CountRemotePlayers()
{
    int count = 0;
    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (p->active != 0 && p->type == 3)
            count++;
    }
    return count;
}

// FUNCTION: 0x457cb0
int CountCombatPlayers()
{
    int count = 0;
    for (int i = 0; i < 10; i++) {
        if (g_game->players[i].active != 0
            && (g_game->players[i].type == 1 || g_game->players[i].type == 2 || g_game->players[i].type == 3)
            && g_game->players[i].index != 10
            && (g_game->players[i].unitCount != 0 || g_game->players[i].unitsCreated == 0)
            && (g_game->players[i].type == 1
                || (g_game->players[i].type == 3 && g_game->players[i].info->kind == 1))
            && !(g_game->players[i].info->flags_9b & 0x40))
            count++;
    }
    return count;
}

#include "../../interfaces/seams.h"

// FUNCTION: 0x457d30
void __stdcall UpdateResourceSharing(Player* player)
{
    if ((g_game->netGameFlags & 1) == 0)
        return;

    if ((unsigned int)g_game->ticks % 60 == 0) {
        Player* found = player;
        if (player->info->shareMetal && player->metal > player->shareMetal) {
            for (int i = 0; i < 10; i++) {
                Player* p = &g_game->players[i];
                if (p->active != 0
                    && (p->type == 1 || p->type == 2 || p->type == 3)
                    && p->index != 10
                    && (p->type == 1 || p->type == 2 || p->type == 3)
                    && (p->unitCount != 0 || p->unitsCreated == 0)
                    && p->type == 3
                    && p->info->kind == 1
                    && player->allied[i] != 0
                    && p->metal < player->metal)
                    found = p;
            }
        }
        if (found != player && player->metal > player->shareMetal) {
            float amount = (player->metal - player->shareMetal) * 0.33333334f;
            float limit = found->metalCapacity - found->metal;
            float result = amount < limit ? amount : limit;
            amount = result;
            g_seams.players->TransferEnergy(player->index, found->index, amount, 1);
        }

        found = player;
        if (player->info->shareEnergy && player->energy > player->shareEnergy) {
            for (int i = 0; i < 10; i++) {
                Player* p = &g_game->players[i];
                if (p->active != 0
                    && (p->type == 1 || p->type == 2 || p->type == 3)
                    && p->index != 10
                    && (p->type == 1 || p->type == 2 || p->type == 3)
                    && (p->unitCount != 0 || p->unitsCreated == 0)
                    && p->type == 3
                    && p->info->kind == 1
                    && player->allied[i] != 0
                    && p->energy < player->energy)
                    found = p;
            }
        }
        if (found != player && player->energy > player->shareEnergy) {
            float amount = (player->energy - player->shareEnergy) * 0.5f;
            float limit = found->energyCapacity - found->energy;
            float result = amount < limit ? amount : limit;
            amount = result;
            g_seams.players->TransferMetal(player->index, found->index, amount, 1);
        }
    }

    if ((unsigned int)g_game->ticks % 450 == 0) {
        if (player->info->shareMapping) {
        for (int i = 0; i < 10; i++) {
            Player* p = &g_game->players[i];
            if (p->active != 0
                && (p->type == 1 || p->type == 2 || p->type == 3)
                && p->index != 10
                && (p->type == 1 || p->type == 2 || p->type == 3)
                && (p->unitCount != 0 || p->unitsCreated == 0)
                && p->type == 3
                && p->info->kind == 1
                && player->allied[i] != 0) {
                unsigned char a = player->index;
                unsigned char b = p->index;
                if (a != 10 && b != 10) {
                    ResourceSharePacket packet;
                    packet.type = 0x16;
                    packet.subtype = 3;
                    packet.fromNetId = PlayerDpid_00457d30(a);
                    packet.toNetId = PlayerDpid_00457d30(b);
                    packet.amount = 0;
                    SendPacketToPlayer(PlayerDpid_00457d30(a), PlayerDpid_00457d30(b),
                                 (unsigned char *)&packet, sizeof(packet));
                }
            }
        }
    }
    }
}
