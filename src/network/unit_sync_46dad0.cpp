#include <list>
#include <map>
#include <vector>

#include "../units/unit_def.h"

struct Data_0046dad0 {
    char unknown_0[0x94];
    unsigned char kind;
};

#include "../../interfaces/ref.h"
#pragma pack(push, 1)
struct PlayerInfo_0046dad0 {
    int active;
    int id;
    char unknown_8[0x27 - 0x8];
    Ref<Data_0046dad0> data;
    char unknown_2b[0x73 - 0x2b];
    unsigned char type;
    char unknown_74[0x14b - 0x74];
};

struct UnitSyncPacket {
    unsigned char type;
    unsigned char arg;
    int id;
    int key;
    int value;
};

#include "../game/game.h"
#pragma pack(pop)

inline int Pass(int v) { return v; }

struct UnitSyncEntry {
    int x, y;
    short w, h;
    int limit;
};

struct Alloc_0046e5c0 {};

struct SyncTaggedVector {
    char allocator;
    char unknown_1[3];
    int _First;
    int _Last;
    int _End;

    SyncTaggedVector* InitTaggedVector(char* param_1) {
        allocator = *param_1;
        _First = 0;
        _Last = 0;
        _End = 0;
        return this;
    }
};

class SyncTempTaggedVector : public SyncTaggedVector {
  public:
    SyncTempTaggedVector(const Alloc_0046e5c0& al = Alloc_0046e5c0()) {
        InitTaggedVector((char*)&al);
    }
    ~SyncTempTaggedVector();
};

class Wrap_0046e5e0 : public SyncTempTaggedVector {
  public:
    Wrap_0046e5e0(const Alloc_0046e5c0& al = Alloc_0046e5c0()) : SyncTempTaggedVector(al) {}
};

class SyncChecksumVector {
  public:
    std::vector<int> vec;
    ~SyncChecksumVector();
};

int __cdecl GetLocalHumanDpid();
int __cdecl GetHostDpid();
void __stdcall SendPacketToPlayer(int a, unsigned int b, void* c, int d);

struct PacketSequencer {
    int lastSent;
    int cur;
    int max;
    SyncChecksumVector sentQueue;
    SyncChecksumVector heldQueue;

    PacketSequencer();
    void SendUnsequenced(unsigned int param_1, void* param_2);
    void Inl(unsigned int param_1, UnitSyncPacket* param_2) {
        param_2->id = 0;
        SendPacketToPlayer(GetLocalHumanDpid(), param_1, param_2, 0xe);
    }
};

struct SyncPlayerRecord {
    int id;
    SyncTempTaggedVector ids;
    Wrap_0046e5e0 pairs;
    int expected;
    int sent;
    int ackd;
    PacketSequencer sub;

    SyncPlayerRecord& operator=(const SyncPlayerRecord& src);
};

#include "../../link/globals.h"
extern int DAT_00000000;

int __stdcall ComputeUnitScriptChecksum(UnitDef* def);

class Vec_0046d860 : public std::vector<SyncPlayerRecord> {
  public:
    void InsertPlayerRecord(iterator where, size_type n, const SyncPlayerRecord& x);
    void Push(const SyncPlayerRecord& x) { InsertPlayerRecord(end(), 1, x); }
};

class UnitSync {
  public:
    std::map<unsigned int, UnitSyncEntry> map;
    Vec_0046d860 players;
    std::list<unsigned int> queue;
    PacketSequencer sequencer;
    int direct;
    int pendingPlayerCount;
    int checksumProgress;
    int disabled;

    void ProcessSync();
    void CheckUnitAvailable(unsigned int key, int y);
    void SendSyncPacket(void* target, UnitSyncPacket* packet, int unused);
};

// FUNCTION: 0x46dad0
void UnitSync::ProcessSync() {
    if (disabled != 0)
        return;

    if (direct != 0) {
        int changed = 0;
        std::vector<SyncPlayerRecord>::iterator it = players.begin();
        if (it != players.end()) {
            do {
                int live = 0;
                for (int i = 0; i < 10; i++) {
                    PlayerInfo_0046dad0* p = &(*(PlayerInfo_0046dad0 (*)[10])&g_game->players)[i];
                    if (p->active != 0 && p->type == 3 && p->data->kind == 1 &&
                        it->id == p->id) {
                        live = 1;
                        break;
                    }
                }
                if (live == 0) {
                    it = players.erase(it);
                    --it;
                    changed = 1;
                }
                ++it;
            } while (it != players.end());
        }

        for (int i = 0; i < 10; i++) {
            PlayerInfo_0046dad0* p = &(*(PlayerInfo_0046dad0 (*)[10])&g_game->players)[i];
            if (p->active != 0 && p->type == 3 && p->data->kind == 1) {
                int found = 0;
                for (std::vector<SyncPlayerRecord>::iterator j = players.begin(); j != players.end();
                     ++j) {
                    if (j->id == p->id) {
                        found = 1;
                        break;
                    }
                }
                if (found == 0) {
                    SyncPlayerRecord entry;
                    entry.id = p->id;
                    entry.expected = 0;
                    entry.sent = 0;
                    entry.ackd = 0;
                    players.Push(entry);
                    SyncPlayerRecord* e = &players.back();
                    if (disabled == 0) {
                        UnitSyncPacket packet;
                        packet.type = 0x1a;
                        packet.arg = 0;
                        packet.key = 0;
                        packet.value = 0;
                        this->SendSyncPacket(e, &packet, 1);
                        e->sent++;
                    }
                    changed = 1;
                }
            }
        }

        if (changed != 0) {
            typedef std::map<unsigned int, UnitSyncEntry>::_Imp Tree;
            typedef void (Tree::iterator::*Increment)();
            Increment increment = &Tree::iterator::_Inc;
            for (std::map<unsigned int, UnitSyncEntry>::iterator k = map.begin(); k != map.end();
                 (k.*increment)()) {
                CheckUnitAvailable(Pass(Pass(Pass(Pass(Pass(Pass(Pass(Pass(Pass(Pass(k->second.x)))))))))), 0);
            }
        }
        return;
    }

    if (pendingPlayerCount > 0) {
        if (checksumProgress < g_game->unitDefCount) {
            if (checksumProgress == 0) {
                if (GetHostDpid() == -1)
                    return;
                int v = g_game->unitDefCount - 1;
                if (disabled == 0) {
                    UnitSyncPacket packet;
                    packet.type = 0x1a;
                    packet.arg = 1;
                    packet.key = 0;
                    packet.value = v;
                    if (direct != 0) {
                        (&this->sequencer)->SendUnsequenced(DAT_00000000, &packet);
                    } else {
                        unsigned int id = GetHostDpid();
                        (&this->sequencer)->SendUnsequenced(id, &packet);
                    }
                }
                checksumProgress = 1;
                return;
            }

            for (int n = 0; n < 4;) {
                Game* game = g_game;
                if (checksumProgress >= game->unitDefCount)
                    return;
                UnitDef* def = &game->defs[checksumProgress];
                ComputeUnitScriptChecksum(def);
                int y = def->scriptChecksum;
                unsigned int key = def->fbiChecksum;
                if (disabled == 0) {
                    UnitSyncPacket packet;
                    packet.type = 0x1a;
                    packet.arg = 2;
                    packet.key = key;
                    packet.value = y;
                    if (direct != 0) {
                        (&this->sequencer)->Inl(DAT_00000000, &packet);
                    } else {
                        unsigned int id = GetHostDpid();
                        (&this->sequencer)->SendUnsequenced(id, &packet);
                    }
                }
                n++;
                checksumProgress++;
            }
        } else {
            UnitSyncPacket packet;
            packet.type = 0x1a;
            packet.arg = 4;
            packet.key = 0;
            packet.value = pendingPlayerCount;
            unsigned int id = GetHostDpid();
            (&this->sequencer)->SendUnsequenced(id, &packet);
        }
    }
}