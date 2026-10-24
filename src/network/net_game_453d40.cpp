#define WIN32_LEAN_AND_MEAN
#include "win_types.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../interfaces/host.h"
#include <memory.h>

#pragma pack(push, 1)

#include "player_info.h"

#include "player.h"

#include "../weapons/weapon_def.h"

#include "../units/cob_script.h"

#include "../units/unit.h"
#include "net_game_453d40.h"

class UnitSync {
public:
    void ReceiveSyncPacket(void*, unsigned char);
};

#include "settings.h"

#include "../game/game.h"

struct PlayerData_004ca6a0 {
    char name[17];
    short versionMajor;
    short versionMinor;
};

struct Packet_00453d40 {
    unsigned int type;
    int playerType;
    int id;
    char* data;
    PlayerData_004ca6a0* createData;
    int dataSize;
    char* longName;
};

struct SideDataPacket {
    unsigned char type;
    PlayerInfo data;
};

struct PlayerIdPacket {
    unsigned char type;
    int id;
    int field_5;
};

struct FeatureDamagePacket {
    unsigned char type;
    unsigned char sub;
    unsigned short x;
    unsigned short z;
};

struct UnitScriptCallPacket {
    unsigned char type;
    unsigned short id;
    short index;
    unsigned char argCount;
    int arg0;
    int arg1;
    int arg2;
    int arg3;
};

struct BuilderLinkPacket {
    unsigned char type;
    short constructedUnitId;
    short builderUnitId;
};

struct Packet_0047f0c0 {
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

struct Msg_00450530 {
    unsigned char type;
    unsigned char flag;
    int id;
    int arg;
};

struct AllyFlagsPacket {
    unsigned char type;
    int fromNetId;
    int toNetId;
    char allied;
    int force;
};

#pragma pack(pop)

#include "../network/packet_manager.h"
#include "../state/sim_globals.h"

#include "../../link/globals.h"

#include "net_game.h"
int GetTicks();
#include "net_game_450530.h"
#include "../frontend/multi.h"
int __stdcall RejectPlayer(int, int);
void __stdcall AddMessage(void*, int, int, unsigned char);
int __stdcall BroadcastPacket(int, void*, int);
void __stdcall SetAlliance(int, int, unsigned char, int);
char* __stdcall Translate(char*);
void __stdcall PlaySoundByName(char*, int);
void __stdcall CreateUnitFromPacket(unsigned char, void*);
#include "../units/unit_position.h"
#include "../units/unit_targets.h"
#include "../units/units_485010.h"
void __stdcall ApplyWeaponFirePacket(Player*, void*);
void __stdcall ApplyProjectileHitPacket(Player*, void*);
#include "../map/features.h"
#include "../map/line_of_sight.h"
#include "../ingame/control_panel.h"
#include "../sound/sound.h"
void __stdcall PlaySoundAt(int, void*, int);
#include "../units/unit_commands.h"
void __stdcall TransferMetal(unsigned char, unsigned char, int, int);
void __stdcall TransferEnergy(unsigned char, unsigned char, int, int);
#include "../units/units_485010.h"
#include "../game/game_state.h"

static inline int GetPlayerId(unsigned char i)
{
    if (i != 10 && g_game->players[i].type)
        return g_game->players[i].id;
    return -1;
}

static inline unsigned char FindPlayerIndex(int id)
{
    if (id == -1)
        return 10;
    for (unsigned char i = 0; i < 10; i++) {
        if (GetPlayerId(i) == id)
            return i;
    }
    return 10;
}

static inline unsigned char FindPlayerSlot(int id)
{
    if (id == -1)
        return 10;
    for (unsigned char i = 0; i < 10; i++) {
        if (GetSlotDpid(i) == id)
            return i;
    }
    return 10;
}

static inline Player* PlayerById(int id)
{
    if (FindPlayerSlot(id) == 10)
        return 0;
    return &g_game->players[FindSlotByDpid(id)];
}

static inline Player* PlayerBySlot(int id)
{
    if (FindPlayerSlot(id) == 10)
        return 0;
    return &g_game->players[FindPlayerSlot(id)];
}

static inline Player* PlayerByIndex(int id)
{
    if (FindPlayerIndex(id) == 10)
        return 0;
    return &g_game->players[FindPlayerIndex(id)];
}

static inline unsigned char FindHost()
{
    for (unsigned char i = 0; i < 10; i++) {
        if (g_game->players[i].type != 0) {
            if (g_game->players[i].info->flags_97 & 1)
                return i;
        }
    }
    return 10;
}

int GetHostDpid()
{
    int i;
    for (i = 0; i < 10; i++) {
        if (g_game->players[i].info->flags_97 & 1)
            return GetPlayerId(i);
    }
    return -1;
}

static inline int FirstJoinedId()
{
    for (int i = 0; i < 10; i++) {
        if (g_game->players[i].type == 1)
            return g_game->players[i].id;
    }
    return -1;
}

static inline int FirstConnectedId()
{
    int i = 0;
    while (1) {
        if (g_game->players[i].active
            && (g_game->players[i].type == 1 || g_game->players[i].type == 2))
            return g_game->players[i].id;
        i++;
        if (i >= 10)
            return -1;
    }
}

static inline unsigned char* InfoPacket(unsigned char* buf, Player* p)
{
    SideDataPacket* pkt = (SideDataPacket*)buf;
    memcpy(&pkt->data, p->info, 0xb9);
    pkt->data.id = p->id;
    pkt->type = 0x20;
    return buf;
}

static inline int IsConnected(Player* p)
{
    return p->active && (p->type == 1 || p->type == 2);
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
    if (FindPlayerSlot(id) == 10)
        return;
    PlayerIdPacket* out = (PlayerIdPacket*)g_game->recvPacketPtr;
    out->type = 0x1c;
    out->id = id;
    Player* p = &g_game->players[FindPlayerSlot(id)];
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
        unsigned char from = FindPlayerIndex(sender);
        unsigned char to = FindPlayerIndex(g_game->lobby1);
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
                Player* p = PlayerById(msg->id);
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
                PlayerById(msg->id);
                int target = FindPlayerIndex(msg->id);
                if (!g_game->players[FindHost()].IsPlayableSlot())
                    break;
                PlayerInfo* info = LocalPlayer()->info;
                PlayerData_004ca6a0* payload = msg->createData;
                if (info->closed) {
                    RejectPlayer(GetPlayerId(target), 3);
                    break;
                }
                if (msg->dataSize != 0x15) {
                    RejectPlayer(GetPlayerId(target), 8);
                    break;
                }
                if (payload->versionMajor != 0 || payload->versionMinor != 0x50) {
                    RejectPlayer(GetPlayerId(target), 8);
                    break;
                }
                if (!(info->flags_9d & 1))
                    break;
                if (payload && !_strcmpi(g_game->password, payload->name))
                    break;
                RejectPlayer(GetPlayerId(target), 4);
                break;
            }
            case 0x102: {
                if (msg->playerType != 1)
                    break;
                Player temp;
                Player* p = PlayerById(msg->id);
                if (p) {
                    int target = FindPlayerIndex(msg->id);
                    if (target == 10) {
                        temp.FreeSideDataAndFogSightCounts();
                        continue;
                    }
                    char* payload = msg->data;
                    memcpy(g_game->players[target].info, payload, 0xb9);
                    if (FindHost() == g_game->localPlayer && !(LocalPlayer()->info->flags_9b & 0x80)
                        && (payload[0x9b] & 0x40))
                        RejectPlayer(p->id, 9);
                }
                temp.FreeSideDataAndFogSightCounts();
                break;
            }
            case 0x104:
                if (FindHost() != g_game->localPlayer)
                    memcpy(&g_game->settings, &msg->playerType, sizeof(Settings));
                break;
            case 0x103: {
                if (msg->playerType != 1)
                    break;
                Player* p = PlayerById(msg->id);
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
            int target = FindPlayerIndex(infoPacket->data.id);
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
            Player* a = PlayerBySlot(ally->fromNetId);
            Player* b = PlayerBySlot(ally->toNetId);
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
            Player* p = PlayerBySlot(allyTeam->id);
            if (p)
                p->alliance = packet[5];
            if (!(g_game->netGameFlags & 4))
                g_game->bits_2bee_1.dirty = 1;
            break;
        }
        case 27: {
            PlayerIdPacket* reject = (PlayerIdPacket*)packet;
            Player* p = PlayerBySlot(reject->id);
            if (p)
                RejectPlayer(p->id, packet[5]);
            break;
        }
        case 28: {
            PlayerIdPacket* drop = (PlayerIdPacket*)packet;
            int id = drop->id;
            if (FindPlayerIndex(id) == 10)
                break;
            char text[200];
            sprintf(text, Translate(g_playerDisconnectedText), PlayerBySlot(id)->name);
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
            int target = FindPlayerIndex(ack->id);
            if (target != 10)
                g_game->startPosAssignAck[target] = 1;
            break;
        }
        case 5:
            if (recipient->active && recipient->type == 1)
                AddMessage(packet + 1, 8, 0, from);
            break;
        case 39: {
            PlayerIdPacket* breach = (PlayerIdPacket*)packet;
            Player* p = PlayerBySlot(breach->id);
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
            SendPacketToPlayer(FirstConnectedId(), g_game->from_id, &reply, 1);
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
                             (Weapon *)&g_game->weapons[damage->sub]);
                break;
            }
            break;
        }
        case 16: {
            UnitScriptCallPacket* call = (UnitScriptCallPacket*)packet;
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
            Packet_0047f0c0* sound = (Packet_0047f0c0*)packet;
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
            if (FindPlayerIndex(id) == 10)
                p = 0;
            else
                p = &g_game->players[FindPlayerSlot(id)];
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
            unsigned char a = FindPlayerIndex(share->fromNetId);
            unsigned char b = FindPlayerIndex(share->toNetId);
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
            PlayerIdPacket* integrity = (PlayerIdPacket*)packet;
            if (g_usePacketManager)
                g_packetManager.HandleIntegrityNop(g_game->from_id, integrity->id,
                                                   integrity->field_5);
            break;
        }
        case 33: {
            Msg_00450530* request = (Msg_00450530*)packet;
            Player* a = PlayerByIndex(request->id);
            Player* b = PlayerByIndex(request->arg);
            if (!IsConnected(&g_game->players[FindHost()]))
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
            Player* p = PlayerByIndex(team->id);
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
