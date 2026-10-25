#include <stdio.h>
#include <utility>

#define _XUTILITY_
namespace std {
template <class _II, class _OI>
inline _OI __stdcall copy(_II _F, _II _L, _OI _X)
{
    for (; _F != _L; ++_X, ++_F)
        *_X = *_F;
    return (_X);
}
template <class _BI1, class _BI2>
inline _BI2 __stdcall copy_backward(_BI1 _F, _BI1 _L, _BI2 _X)
{
    while (_F != _L)
        *--_X = *--_L;
    return (_X);
}
template <class _II1, class _II2>
inline bool __stdcall equal(_II1 _F, _II1 _L, _II2 _X)
{
    return (mismatch(_F, _L, _X).first == _L);
}
template <class _II1, class _II2, class _Pr>
inline bool __stdcall equal(_II1 _F, _II1 _L, _II2 _X, _Pr _P)
{
    return (mismatch(_F, _L, _X, _P).first == _L);
}
template <class _FI, class _Ty>
inline void __stdcall fill(_FI _F, _FI _L, const _Ty& _X)
{
    for (; _F != _L; ++_F)
        *_F = _X;
}
template <class _OI, class _Sz, class _Ty>
inline void __stdcall fill_n(_OI _F, _Sz _N, const _Ty& _X)
{
    for (; 0 < _N; --_N, ++_F)
        *_F = _X;
}
template <class _II1, class _II2>
inline bool __stdcall lexicographical_compare(_II1 _F1, _II1 _L1, _II2 _F2, _II2 _L2)
{
    for (; _F1 != _L1 && _F2 != _L2; ++_F1, ++_F2)
        if (*_F1 < *_F2)
            return (true);
        else if (*_F2 < *_F1)
            return (false);
    return (_F1 == _L1 && _F2 != _L2);
}
template <class _II1, class _II2, class _Pr>
inline bool __stdcall lexicographical_compare(_II1 _F1, _II1 _L1, _II2 _F2, _II2 _L2, _Pr _P)
{
    for (; _F1 != _L1 && _F2 != _L2; ++_F1, ++_F2)
        if (_P(*_F1, *_F2))
            return (true);
        else if (_P(*_F2, *_F1))
            return (false);
    return (_F1 == _L1 && _F2 != _L2);
}
#define _MAX _cpp_max
#define _MIN _cpp_min
template <class _Ty>
inline const _Ty& __stdcall _cpp_max(const _Ty& _X, const _Ty& _Y)
{
    return (_X < _Y ? _Y : _X);
}
template <class _Ty, class _Pr>
inline const _Ty& __stdcall _cpp_max(const _Ty& _X, const _Ty& _Y, _Pr _P)
{
    return (_P(_X, _Y) ? _Y : _X);
}
template <class _Ty>
inline const _Ty& __stdcall _cpp_min(const _Ty& _X, const _Ty& _Y)
{
    return (_Y < _X ? _Y : _X);
}
template <class _Ty, class _Pr>
inline const _Ty& __stdcall _cpp_min(const _Ty& _X, const _Ty& _Y, _Pr _P)
{
    return (_P(_Y, _X) ? _Y : _X);
}
template <class _II1, class _II2>
inline pair<_II1, _II2> __stdcall mismatch(_II1 _F, _II1 _L, _II2 _X)
{
    for (; _F != _L && *_F == *_X; ++_F, ++_X)
        ;
    return (pair<_II1, _II2>(_F, _X));
}
template <class _II1, class _II2, class _Pr>
inline pair<_II1, _II2> __stdcall mismatch(_II1 _F, _II1 _L, _II2 _X, _Pr _P)
{
    for (; _F != _L && _P(*_F, *_X); ++_F, ++_X)
        ;
    return (pair<_II1, _II2>(_F, _X));
}
template <class _Ty>
inline void __stdcall swap(_Ty& _X, _Ty& _Y)
{
    _Ty _Tmp = _X;
    _X = _Y, _Y = _Tmp;
}
}

#include <vector>
#include <map>
#include <list>
#include <yvals.h>

struct Rect_0046c620 {
    int x0;
    int y0;
    int x1;
    int y1;
};

struct Name_0046c620 {
    char text[16];
    char flag;
};

struct Net_4c97b0 {
    Name_0046c620 name;
    char unknown_11[0x4c9 - 0x11];
    int created;
    char unknown_4cd[4];
};

#include "../map/mission.h"
#include "../units/unit_def.h"

#pragma pack(push, 1)
struct UnitDefFlags_0046e160 {
    unsigned int bits_0 : 23;
    unsigned int flag_23 : 1;
    unsigned int bits_24 : 8;
};
#pragma pack(pop)

struct UnitSync;

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

extern Game* g_game;

typedef int (__stdcall *SendFn_0046c620)(int, Rect_0046c620*, void*, int, Name_0046c620*,
                                         int, int, int, void*, void*);

extern int g_onlineReportScores;
extern Name_0046c620 g_reportPlayerName;
extern int* g_onlineReportPlayers;
extern void* g_onlineReportScoreBoards;
extern SendFn_0046c620 g_riReport;
extern int g_reporterDll;
extern int g_hapinetOnlineReport;
extern int g_reportFlags;
extern void (*g_riIntervalTimer)(void);
extern int (__stdcall* g_riReportGameChat)(int, int);

Rect_0046c620* __stdcall GetSessionGuidInstance(Net_4c97b0* p);
#include "online_report.h"
int __stdcall RIReport(int, Rect_0046c620*, void*, int, Name_0046c620*, int,
                           int, int, void*, void*);
#include "../game/statistics_46c2a0.h"

// FUNCTION: 0x46c620
int __stdcall ReportGameEvent(int msg)
{
    if (!g_hapinetOnlineReport && !g_reporterDll)
        return 4;
    if (!g_onlineReportPlayers || !g_onlineReportScoreBoards || !g_onlineReportScores)
        return 1;

    Rect_0046c620 rect = *GetSessionGuidInstance(&(*(Net_4c97b0*)((char*)g_game + 0x14)));
    int thing = GetCreatedLobbyInterface(&(*(Net_4c97b0*)((char*)g_game + 0x14)));

    if (msg == 1) {
        g_reportPlayerName = (*(Net_4c97b0*)((char*)g_game + 0x14)).name;
        char* p = g_reportPlayerName.text + 15;
        while (p > g_reportPlayerName.text && *p == ' ')
            *p-- = 0;
    }

    int id = FillScoreTables();

    if (g_hapinetOnlineReport) {
        if (msg == 1 || msg == 6 || msg == 7)
            g_reportFlags = QueryOnlineFlags(msg == 1 ? 1 : 2 + (msg != 6));
        if (g_reportFlags & 3) {
            int team = (int)g_game->net->GetMissionName();
            if (RIReport(msg, &rect, (char*)&g_game->providerGuid, thing, &g_reportPlayerName,
                             team, g_game->localPlayer,
                             id, g_onlineReportPlayers, g_onlineReportScoreBoards))
                g_hapinetOnlineReport = 0;
        }
    }

    if (g_reporterDll) {
        int team = (int)g_game->net->GetMissionName();
        g_riReport(msg, &rect, (char*)&g_game->providerGuid, thing, &g_reportPlayerName,
                     team, g_game->localPlayer,
                     id, g_onlineReportPlayers, g_onlineReportScoreBoards);
    }

    return 0;
}

// FUNCTION: 0x46c810
int __stdcall ReportGameChat(int msg)
{
    if ((g_hapinetOnlineReport != 0 && (g_reportFlags & 8)) || g_reporterDll != 0) {
        FillScoreTables();
        if (g_hapinetOnlineReport != 0 && (g_reportFlags & 8)) {
            if (RIReportGameChat(g_onlineReportPlayers[g_game->localPlayer], msg))
                g_hapinetOnlineReport = 0;
        }
        if (g_reporterDll != 0)
            return g_riReportGameChat(g_onlineReportPlayers[g_game->localPlayer], msg);
        return 0;
    }
    return 4;
}

// FUNCTION: 0x46c8b0
void ReportIntervalTimer(void)
{
    if (g_reporterDll) {
        g_riIntervalTimer();
    }
}

// FUNCTION: 0x46c8c0
void __stdcall FUN_0046c8c0(int)
{
}

// FUNCTION: 0x46c8d0
void FUN_0046c8d0(void)
{
}

struct UnitSyncEntry {
    int x;
    int y;
    short w;
    short h;
    int limit;
};

struct UnitSyncNode {
    UnitSyncNode* left;
    UnitSyncNode* parent;
    UnitSyncNode* right;
    unsigned int key;
    UnitSyncEntry value;
    int color;
};

class UnitSyncIter {
public:
    UnitSyncNode* ptr;
    UnitSyncIter() {}
    UnitSyncIter(UnitSyncNode* p) : ptr(p) {}
    bool operator==(const UnitSyncIter& other) const { return ptr == other.ptr; }
};

#pragma pack(push, 1)
struct Unit_0046e330 {
    char unknown_0[0x13e];
    unsigned int key;
};

#pragma pack(pop)

#include "player.h"
#include "player_info.h"

#pragma pack(push, 1)
struct UnitSyncPacket {
    unsigned char type;
    unsigned char arg;
    unsigned int id;
    int key;
    union {
        int value;
        struct {
            unsigned char enabled;
            unsigned char match;
            short limit;
        };
    };
};

struct Target_0046d530 {
    unsigned int id;
    char unknown_4[0x24];
    int sent;
};

struct Source_0046d630 {
    int key;
    char unknown_4[4];
    unsigned char enabled;
    char unknown_9;
    unsigned char match;
    char unknown_b;
    short limit;
};

#pragma pack(pop)

struct Cmp_0046d040 {
    char x;
};

struct Alloc_0046d040 {
    char x;
};

class Vec_0046d6c0 {
public:
    int pad;
    int* first;
    int* last;
    int* cap;

    int* begin() { return first; }
    int* end() { return last; }
    unsigned int size() { return first == 0 ? 0 : last - first; }
    void insert(int* pos, int n, int const& val);
};

struct PlayerSync_0046e0b0 {
    unsigned int id;
    Vec_0046d6c0 ids;
    Vec_0046d6c0 pairs;
    int expected;
    int sent;
    unsigned int ackd;
    char unknown_30[0x5c - 0x30];
};

struct Kfn_0046d6c0 {};
struct Alloc_0046d6c0 {};

struct Less_0046d6c0 {
    bool operator()(const unsigned int& a, const unsigned int& b) const { return a < b; }
};

template<class _K, class _Ty, class _Kfn, class _Pr, class _A>
class _Tree {
public:
    typedef UnitSyncNode _Node;
    struct _Pairib {
        _Node* first;
        bool second;
        _Pairib() {}
    };

    char _Alloc;
    union {
        char _Pred;
        _Pr key_compare;
    };
    _Node* _Head;
    bool _Multi;
    int _Size;

    _Pairib insert(const _Ty& _V);
};

typedef std::pair<const unsigned int, UnitSyncEntry> Value_0046d6c0;

extern UnitSyncNode* DAT_0051e598;
extern int DAT_0051e59c;

class UnitSyncMap : public _Tree<unsigned int, Value_0046d6c0, Kfn_0046d6c0,
                                  Less_0046d6c0, Alloc_0046d6c0> {
public:
    UnitSyncMap(Cmp_0046d040 c, Alloc_0046d040 a)
    {
        _Alloc = c.x;
        _Pred = a.x;
        _Multi = 0;
        Init();
    }

    UnitSyncIter End() { return UnitSyncIter(_Head); }
    UnitSyncIter FindExact(const unsigned int& key);
    UnitSyncNode* LowerBound(const unsigned int* key);
    int* Begin(int* param_1);

    UnitSyncNode* Buynode(UnitSyncNode* parent, int color)
    {
        UnitSyncNode* s = (UnitSyncNode*)operator new(sizeof(UnitSyncNode));
        s->parent = parent;
        s->color = color;
        return s;
    }

    void Init();

    UnitSyncEntry& operator[](const unsigned int& k)
    {
        _Pairib p = insert(Value_0046d6c0(k, UnitSyncEntry()));
        return p.first->value;
    }
};

class Sub_0046d040 {
public:
    int seqSent;
    int seqCur;
    int seqMax;

    void SendUnsequenced(void* packet, int to);
};

class Sub2_0046d040 {
public:
    int direct;
    int* pendingPlayerCount;
    int* checksumProgress;
    int* disabled;
};

#pragma pack(push, 2)
struct Elem_0046faf0 {
    union {
        struct {
            int a;
            int b;
            int c;
            short d;
        };
        UnitSyncPacket packet;
    };
};
#pragma pack(pop)

class UnitSyncPlayer {
public:
    char unknown_0[0x5c];

    UnitSyncPlayer(const UnitSyncPlayer& other);
    ~UnitSyncPlayer();
    UnitSyncPlayer& operator=(const UnitSyncPlayer& rhs);
};

class SyncChecksumVector {
public:
    std::vector<int> vec;
    ~SyncChecksumVector();
};

#include "net_game.h"
unsigned int GetHostDpid();
#include "net_game.h"
int __stdcall ComputeUnitScriptChecksum(UnitDef* def);
#include "../ai/ai_profile.h"
extern char g_unitSyncStatusText[];

static inline void SendPacket(unsigned int to, UnitSyncPacket* packet)
{
    packet->id = 0;
    SendPacketToPlayer(GetLocalHumanDpid(), to, (unsigned char *)packet, 0xe);
}

class UnitSync {
public:
    UnitSyncMap map;
    std::vector<PlayerSync_0046e0b0> players;
    std::list<int> ids;
    Sub_0046d040 sub;
    std::vector<int> seqSentQueue;
    std::vector<int> seqHeldQueue;
    union {
        Sub2_0046d040 sub2;
        struct {
            int direct;
            union {
                int* first;
                int pendingPlayerCount;
            };
            int* checksumProgress;
            int disabled;
        };
    };

    UnitSync(int param);

    UnitSyncIter End() { return UnitSyncIter(map._Head); }
    UnitSyncIter Find(const unsigned int* key)
    {
        UnitSyncIter p = UnitSyncIter(map.LowerBound(key));
        return (p == End() || map.key_compare(*key, p.ptr->key)) ? End() : p;
    }
    void Send(Target_0046d530* target, UnitSyncPacket* packet)
    {
        if (direct != 0) {
            SendPacket(target->id, packet);
        } else {
            SendPacket(GetHostDpid(), packet);
        }
    }

    void SendSyncPacket(unsigned int* param_1, UnitSyncPacket* param_2, int unused);
    void SendSyncMessage(unsigned char arg, int a, int b, int unused);
    void SendSyncMessageTo(Target_0046d530* target, unsigned char arg, int a, int b, int unused);
    void SendEntryTo(Target_0046d530* target, unsigned char arg, Source_0046d630* src, int unused);
    void HandleSyncPacket(UnitSyncPacket* packet, unsigned char player);
    void ReceiveSyncPacket(void* param_1, int param_2);
    void NotifyEntryChanged(unsigned int param_1);
    void CheckUnitAvailable(unsigned int key, int y);
    char* GetSyncStatusText();
    int AllPlayersSynced();
    int IsPlayerSynced(int id);
    int PopChangedEntry(UnitSyncEntry* out);
    int GetUnitEntry(Unit_0046e330* unit, UnitSyncEntry* out);
    int ToggleUnitAllowed(Unit_0046e330* unit);
    int DisallowUnit(Unit_0046e330* unit);
    int AllowUnit(Unit_0046e330* unit);
    void SetUnitLimit(Unit_0046e330* unit, int value);
    void ApplyToUnitTypes();
    void ResetEntries();
};

static inline UnitDef* Defs_0046d040()
{
    return g_game->defs;
}

static inline bool FlagOf_0046d040(UnitDef* d)
{
    return (d->flags2 >> 16) & 1;
}

class UnitSyncDeleter {
public:
    std::map<unsigned int, UnitSyncEntry> map;
    std::vector<UnitSyncPlayer> elems;
    std::list<int> ids;
    Sub_0046d040 sub;
    SyncChecksumVector seqSentQueue;
    std::vector<int> seqHeldQueue;
    int direct;
    int pendingPlayerCount;
    int checksumProgress;
    int disabled;

    ~UnitSyncDeleter() {}
};

// FUNCTION: 0x46c8e0
void __stdcall CreateUnitSync(int param_1)
{
    (*(UnitSync **)&g_game->sync) = new UnitSync(param_1);
}

// FUNCTION: 0x46c920
void __fastcall DeleteUnitSync()
{
    if ((UnitSync *)g_game->sync)
        delete (UnitSyncDeleter*)(UnitSync *)g_game->sync;
    (*(UnitSync **)&g_game->sync) = 0;
}

class Class_0046eba0 {
public:
    char unknown_0[4];
    UnitSyncPacket* first;
    UnitSyncPacket* last;
    UnitSyncPacket* end;
    void InsertPacket(UnitSyncPacket* where, int count, UnitSyncPacket* val);
};

struct PacketSequencer {
    int lastSent;
    unsigned int cur;
    unsigned int max;
    std::vector<Elem_0046faf0> sentQueue;
    std::vector<Elem_0046faf0> heldQueue;
    PacketSequencer();
    PacketSequencer& operator=(const PacketSequencer& rhs);
    void SendUnsequenced(unsigned int param_1, void* param_2);
    void SendSequenced(Elem_0046faf0* param_1, Elem_0046faf0* param_2);
    void ReceiveSequenced(UnitSyncPacket* packet, int param_2, void* param_3, unsigned int target);
};

// FUNCTION: 0x46cbe0
PacketSequencer::PacketSequencer()
{
    lastSent = 0;
    cur = 0;
    max = 0;
}

// FUNCTION: 0x46cec0
void PacketSequencer::SendUnsequenced(unsigned int param_1, void* param_2)
{
    UnitSyncPacket* packet = (UnitSyncPacket*)param_2;
    packet->id = 0;
    SendPacketToPlayer(GetLocalHumanDpid(), param_1, (unsigned char *)packet, 0xe);
}

// FUNCTION: 0x46cef0
void PacketSequencer::ReceiveSequenced(UnitSyncPacket* packet, int param_2, void* param_3, unsigned int target)
{
    if (packet->arg == 0x65) {
        for (Elem_0046faf0* p = sentQueue.begin(); p != sentQueue.end(); p++) {
            if (p->packet.id == packet->id) {
                SendPacketToPlayer(GetLocalHumanDpid(), target, (unsigned char *)p, 0xe);
                break;
            }
        }
        return;
    }
    if (packet->id > max) {
        max = packet->id;
    }
    if (packet->id == cur + 1) {
        cur = packet->id;
        ((UnitSync*)param_3)->HandleSyncPacket(packet, param_2);
        for (unsigned int i = cur + 1; i <= max; i++) {
            Elem_0046faf0* p;
            for (p = heldQueue.begin(); p != heldQueue.end(); p++) {
                if (p->packet.id == i) break;
            }
            if (p == heldQueue.end()) break;
            cur = i;
            ((UnitSync*)param_3)->HandleSyncPacket(&p->packet, param_2);
        }
        return;
    }
    if (packet->id <= cur) {
        return;
    }
    Class_0046eba0& v = *(Class_0046eba0*)&heldQueue;
    v.InsertPacket(v.last, 1, packet);
    for (unsigned int i = cur + 1; i <= max; i++) {
        Elem_0046faf0* p;
        for (p = heldQueue.begin(); p != heldQueue.end(); p++) {
            if (p->packet.id == i) break;
        }
        if (p == heldQueue.end()) {
            UnitSyncPacket msg;
            msg.type = 0x1a;
            msg.arg = 0x65;
            msg.id = i;
            SendPacketToPlayer(GetLocalHumanDpid(), target, (unsigned char *)&msg, 0xe);
        }
    }
}

#pragma auto_inline(off)
// FUNCTION: 0x470560 ??4PacketSequencer@@QAEAAU0@ABU0@@Z
PacketSequencer& PacketSequencer::operator=(const PacketSequencer& rhs)
{
    lastSent = rhs.lastSent;
    cur = rhs.cur;
    max = rhs.max;
    sentQueue = rhs.sentQueue;
    heldQueue = rhs.heldQueue;
    return *this;
}
#pragma auto_inline(on)

// FUNCTION: 0x46d040
UnitSync::UnitSync(int param)
    : map(Cmp_0046d040(), Alloc_0046d040())
{
    sub.seqSent = 0;
    sub.seqCur = 0;
    sub.seqMax = 0;
    sub2.disabled = 0;
    sub2.direct = param;
    sub2.pendingPlayerCount = 0;
    sub2.checksumProgress = 0;
    {
        UnitSyncEntry v;
        for (unsigned short i = 1; i < g_game->unitDefCount; i++) {
            unsigned int key = g_game->defs[i].fbiChecksum;
            v.x = key;
            v.y = 0;
            v.w = 1;
            v.h = (short)sub2.direct;
            v.limit = FlagOf_0046d040(&g_game->defs[i]) ? 0 : -1;
            map[key] = v;
        }
    }
}

// FUNCTION: 0x46d2e0
void UnitSync::ResetEntries()
{
    UnitSyncEntry v;
    for (unsigned short i = 1; i < g_game->unitDefCount; i++) {
        v.x = g_game->defs[i].fbiChecksum;
        v.y = 0;
        v.w = 1;
        v.h = (short)direct;
        v.limit = FlagOf_0046d040(&g_game->defs[i]) ? 0 : -1;
        ((std::map<unsigned int, UnitSyncEntry>&)map)[v.x] = v;
    }
}

#pragma auto_inline(off)
// FUNCTION: 0x46d4c0
void UnitSync::SendSyncPacket(unsigned int* param_1, UnitSyncPacket* param_2, int unused)
{
    unsigned int val;
    if (direct != 0)
        val = *param_1;
    else
        val = GetHostDpid();

    param_2->id = 0;
    SendPacketToPlayer(GetLocalHumanDpid(), val, (unsigned char *)param_2, 14);
}
#pragma auto_inline(on)

// FUNCTION: 0x46d500
void UnitSync::ReceiveSyncPacket(void* param_1, int param_2)
{
    UnitSyncPacket* packet = (UnitSyncPacket*)param_1;
    unsigned char b = packet->arg;
    packet->id = 0;
    if (b < 0x64) {
        HandleSyncPacket(packet, param_2);
    }
}

// FUNCTION: 0x46d530
void UnitSync::SendSyncMessage(unsigned char arg, int a, int b, int unused)
{
    if (disabled == 0) {
        UnitSyncPacket packet;
        packet.type = 0x1a;
        packet.arg = arg;
        packet.key = a;
        packet.value = b;
        Send(0, &packet);
    }
}

// FUNCTION: 0x46d5b0
void UnitSync::SendSyncMessageTo(Target_0046d530* target, unsigned char arg, int a, int b, int unused)
{
    if (disabled == 0) {
        UnitSyncPacket packet;
        packet.type = 0x1a;
        packet.arg = arg;
        packet.key = a;
        packet.value = b;
        if (direct != 0) {
            SendPacket(target->id, &packet);
        } else {
            SendPacket(GetHostDpid(), &packet);
        }
        target->sent++;
    }
}

// FUNCTION: 0x46d630
void UnitSync::SendEntryTo(Target_0046d530* target, unsigned char arg, Source_0046d630* src, int unused)
{
    if (disabled == 0) {
        UnitSyncPacket packet;
        packet.type = 0x1a;
        packet.arg = arg;
        packet.key = src->key;
        packet.enabled = src->enabled;
        packet.match = src->match;
        packet.limit = src->limit;
        if (direct != 0) {
            SendPacket(target->id, &packet);
        } else {
            SendPacket(GetHostDpid(), &packet);
        }
        target->sent++;
    }
}

// FUNCTION: 0x46d6c0
void UnitSync::HandleSyncPacket(UnitSyncPacket* packet, unsigned char player)
{
    if (disabled != 0) {
        return;
    }
    pendingPlayerCount++;
    if (direct != 0) {
        std::vector<PlayerSync_0046e0b0>::iterator i = players.begin();
        if (i != players.end()) {
            unsigned int id = g_game->players_1b67[player].id;
            for (; i != players.end(); i++) {
                if (i->id == id) {
                    break;
                }
            }
        }

        switch (packet->arg) {
        case 0:
            break;

        case 1:
            i->expected = packet->value;
            break;

        case 2:
            {
                int* j = i->ids.begin();
                while (j != i->ids.end()) {
                    if (*j == packet->key) {
                        break;
                    }
                    j++;
                }
                if (j != i->ids.end()) {
                    return;
                }
            }
            {
                Vec_0046d6c0& v = i->ids;
                v.insert(v.end(), 1, packet->key);
            }
            {
                Vec_0046d6c0& w = i->pairs;
                w.insert(w.end(), 1, packet->value);
            }
            this->CheckUnitAvailable(packet->key, packet->value);
            break;

        case 3:
            break;

        case 4:
            if (i->ackd < (unsigned int)packet->value) {
                i->ackd = packet->value;
            }
            break;
        }
    } else {
        if (packet->arg != 0 && packet->arg == 3) {
            UnitSyncEntry r;
            r.x = packet->key;
            r.y = 0;
            r.w = packet->enabled;
            r.h = packet->match;
            r.limit = packet->limit;
            map[packet->key] = r;
            this->NotifyEntryChanged(packet->key);
        }
    }
}

// FUNCTION: 0x46d860
void UnitSync::NotifyEntryChanged(unsigned int param_1)
{
    std::vector<PlayerSync_0046e0b0>& ps = players;
    if (disabled != 0) {
        return;
    }
    if (direct != 0) {
        UnitSyncIter it = map.FindExact(param_1);
        for (std::vector<PlayerSync_0046e0b0>::iterator i = ps.begin(); i != ps.end(); ++i) {
            UnitSyncEntry* v = &it.ptr->value;
            if (disabled == 0) {
                UnitSyncPacket packet;
                packet.type = 0x1a;
                packet.arg = 3;
                packet.key = v->x;
                packet.enabled = v->w;
                packet.match = v->h;
                packet.limit = v->limit;
                this->SendSyncPacket(&i->id, &packet, 1);
                i->sent++;
            }
        }
    }
    for (std::list<int>::iterator it = ids.begin(); it != ids.end(); ++it) {
        if (*it == param_1) {
            return;
        }
    }
    ids.push_back((int&)param_1);
}

// FUNCTION: 0x46d970
void UnitSync::CheckUnitAvailable(unsigned int key, int y)
{
    if (disabled != 0)
        return;

    UnitSyncIter it = map.FindExact(key);
    if (it == map.End())
        return;

    int h = 1;
    if (y != 0) {
        if (it.ptr->value.y == 0) {
            int n = g_game->unitDefCount;
            for (int i = 1; i < n; i++) {
                UnitDef* def = &g_game->defs[i];
                if (def->fbiChecksum == key) {
                    ComputeUnitScriptChecksum(def);
                    it.ptr->value.y = def->scriptChecksum;
                    break;
                }
            }
        }
    }
    if (y != 0) {
        if (y != it.ptr->value.y)
            h = 0;
    }

    {
        for (PlayerSync_0046e0b0* e = players.begin(); e != players.end(); e++) {
            int flag;
            if (y != 0) {
                Player* pl = FindPlayerByDpid(e->id);
                if (pl == 0)
                    break;
                flag = pl->info->versionMajor >= 2 ? 1 : (pl->info->versionMajor == 1 && pl->info->versionMinor >= 2 ? 1 : 0);
            } else {
                flag = 0;
            }
            int* p2 = e->pairs.first;
            int* p1 = e->ids.first;
            int* p3 = e->ids.last;
            while (p1 != p3) {
                if (*p1 == key) {
                    if (flag && *p2 != y)
                        break;
                    goto next_entry;
                }
                p1++;
                p2++;
            }
            h = 0;
            break;
        next_entry:
            ;
        }
    }

    it.ptr->value.h = h;
    NotifyEntryChanged(key);
}

// FUNCTION: 0x46df40
char* UnitSync::GetSyncStatusText()
{
    if (direct == 0) {
        return 0;
    }
    for (std::vector<PlayerSync_0046e0b0>::iterator it = players.begin(); it != players.end(); ++it) {
        if (it->expected == 0) {
            return "No units_expected sent from player";
        }
        if (it->ids.size() != it->expected) {
            sprintf(g_unitSyncStatusText, "expected %d units, got %d", it->expected, it->ids.size());
            return g_unitSyncStatusText;
        }
        if (it->sent != it->ackd) {
            sprintf(g_unitSyncStatusText, "packets sent=%d  ackd=%d", it->sent, it->ackd);
            return g_unitSyncStatusText;
        }
    }
    return "OK";
}

// FUNCTION: 0x46e000
int UnitSync::AllPlayersSynced()
{
    if (disabled != 0)
        return 1;
    if (direct == 0)
        return 1;
    PlayerSync_0046e0b0* p = players.begin();
    if (p == players.end())
        return 1;
    for (; p != players.end(); p++) {
        Player* pl = FindPlayerByDpid(p->id);
        if (pl != 0) {
            if (pl->active != 0 && pl->type == 3 && pl->info->kind == 2)
                continue;
            if (pl->active != 0 && pl->type == 2)
                continue;
            if (p->expected == 0)
                return 0;
            if (p->ids.size() != p->expected)
                return 0;
            if (p->sent != p->ackd)
                return 0;
        }
    }
    return 1;
}

// FUNCTION: 0x46e0b0
int UnitSync::IsPlayerSynced(int id)
{
    if (direct == 0)
        return 0;
    Player* player;
    if (disabled != 0
        || (player = FindPlayerByDpid(id)) == 0
        || (player->active != 0 && player->type == 3 && player->info->kind == 2)
        || (player->active != 0 && player->type == 2))
        return 1;
    for (std::vector<PlayerSync_0046e0b0>::iterator it = players.begin(); it != players.end(); ++it) {
        if (it->id != id)
            continue;
        if (it->expected == 0 || it->ids.size() != it->expected)
            return 0;
        if (it->sent == it->ackd)
            return 1;
        break;
    }
    return 0;
}

// FUNCTION: 0x46e160
void UnitSync::ApplyToUnitTypes()
{
    if (disabled != 0)
        return;
    for (unsigned short i = 1; i < g_game->unitDefCount; i++) {
        UnitDef* def = &g_game->defs[i];
        UnitSyncIter it = Find(&def->fbiChecksum);
        if (it == End()) {
            ProtectUnitDefsReadWrite();
            def->maxThisUnit = 0;
            ((UnitDefFlags_0046e160*)&def->flags1)->flag_23 = 0;
        } else {
            ProtectUnitDefsReadWrite();
            ((UnitDefFlags_0046e160*)&def->flags1)->flag_23 = (it.ptr->value.w != 0 && it.ptr->value.h != 0);
            def->maxThisUnit = it.ptr->value.limit;
        }
        ProtectUnitDefsReadOnly();
    }
}

// FUNCTION: 0x46e280
int UnitSync::PopChangedEntry(UnitSyncEntry* out)
{
    if (ids.empty())
        return 0;
    UnitSyncIter p = Find((unsigned int*)&ids.front());
    *out = p.ptr->value;
    ids.pop_front();
    return 1;
}

// FUNCTION: 0x46e330
int UnitSync::GetUnitEntry(Unit_0046e330* unit, UnitSyncEntry* out)
{
    *out = Find(&unit->key).ptr->value;
    return out->w != 0 && out->h != 0;
}

// FUNCTION: 0x46e3c0
int UnitSync::ToggleUnitAllowed(Unit_0046e330* unit)
{
    UnitSyncNode* n = Find(&unit->key).ptr;
    n->value.w = (n->value.w == 0);
    NotifyEntryChanged(unit->key);
    return n->value.w != 0 && n->value.h != 0;
}

// FUNCTION: 0x46e450
int UnitSync::DisallowUnit(Unit_0046e330* unit)
{
    UnitSyncNode* n = Find(&unit->key).ptr;
    n->value.w = 0;
    NotifyEntryChanged(unit->key);
    return n->value.w != 0 && n->value.h != 0;
}

// FUNCTION: 0x46e4d0
int UnitSync::AllowUnit(Unit_0046e330* unit)
{
    UnitSyncNode* n = Find(&unit->key).ptr;
    n->value.w = 1;
    NotifyEntryChanged(unit->key);
    return n->value.w != 0 && n->value.h != 0;
}

// FUNCTION: 0x46e550
void UnitSync::SetUnitLimit(Unit_0046e330* unit, int value)
{
    UnitSyncIter it = Find(&unit->key);
    if (!(it == End())) {
        it.ptr->value.limit = value;
        NotifyEntryChanged(unit->key);
    }
}

struct SyncTaggedVector {
    char allocator;
    char unknown_1[3];
    int _First;
    int _Last;
    int _End;

    SyncTaggedVector* InitTaggedVector(char* param_1);
};

// FUNCTION: 0x46e5c0
SyncTaggedVector* SyncTaggedVector::InitTaggedVector(char* param_1)
{
    allocator = *param_1;
    _First = 0;
    _Last = 0;
    _End = 0;
    return this;
}

struct Elem_0046e5e0 {
    int value;
};

class SyncTempTaggedVector {
public:
    std::vector<Elem_0046e5e0> vec;

    ~SyncTempTaggedVector();
};

// FUNCTION: 0x46e5e0
SyncTempTaggedVector::~SyncTempTaggedVector()
{
}

#pragma auto_inline(off)
// FUNCTION: 0x46e610
SyncChecksumVector::~SyncChecksumVector()
{
}
#pragma auto_inline(on)

// FUNCTION: 0x46e880
int* UnitSyncMap::Begin(int* param_1)
{
    *param_1 = (int)_Head->left;
    return param_1;
}

typedef std::map<unsigned int, UnitSyncEntry>::_Imp Tree_0046e890;
typedef Tree_0046e890::iterator (Tree_0046e890::*EraseFn_0046e890)(
    Tree_0046e890::iterator, Tree_0046e890::iterator);

// FUNCTION: 0x46e890 ?erase@?$_Tree@IU?$pair@IUUnitSyncEntry@@@std@@U_Kfn@?$map@IUUnitSyncEntry@@U?$less@I@std@@V?$allocator@UUnitSyncEntry@@@3@@2@U?$less@I@2@V?$allocator@UUnitSyncEntry@@@2@@std@@QAE?AViterator@12@V312@0@Z
EraseFn_0046e890 g_erase_0046e890 = &Tree_0046e890::erase;

#pragma auto_inline(off)
// FUNCTION: 0x46e9b0
UnitSyncIter UnitSyncMap::FindExact(const unsigned int& key)
{
    UnitSyncIter p = UnitSyncIter(LowerBound(&key));
    return (p == End() || key_compare(key, p.ptr->key)) ? End() : p;
}
#pragma auto_inline(on)

// FUNCTION: 0x46ea10 ?_Inc@iterator@?$_Tree@IU?$pair@IUUnitSyncEntry@@@std@@U_Kfn@?$map@IUUnitSyncEntry@@U?$less@I@std@@V?$allocator@UUnitSyncEntry@@@3@@2@U?$less@I@2@V?$allocator@UUnitSyncEntry@@@2@@std@@QAEXXZ
// FUNCTION: 0x46ef50 ?insert@?$_Tree@IU?$pair@IUUnitSyncEntry@@@std@@U_Kfn@?$map@IUUnitSyncEntry@@U?$less@I@std@@V?$allocator@UUnitSyncEntry@@@3@@2@U?$less@I@2@V?$allocator@UUnitSyncEntry@@@2@@std@@QAE?AU?$pair@Viterator@?$_Tree@IU?$pair@IUUnitSyncEntry@@@std@@U_Kfn@?$map@IUUnitSyncEntry@@U?$less@I@std@@V?$allocator@UUnitSyncEntry@@@3@@2@U?$less@I@2@V?$allocator@UUnitSyncEntry@@@2@@std@@_N@2@ABU?$pair@IUUnitSyncEntry@@@2@@Z
// FUNCTION: 0x46f1e0 ?erase@?$_Tree@IU?$pair@IUUnitSyncEntry@@@std@@U_Kfn@?$map@IUUnitSyncEntry@@U?$less@I@std@@V?$allocator@UUnitSyncEntry@@@3@@2@U?$less@I@2@V?$allocator@UUnitSyncEntry@@@2@@std@@QAE?AViterator@12@V312@@Z
// FUNCTION: 0x46f6d0 ?_Erase@?$_Tree@IU?$pair@IUUnitSyncEntry@@@std@@U_Kfn@?$map@IUUnitSyncEntry@@U?$less@I@std@@V?$allocator@UUnitSyncEntry@@@3@@2@U?$less@I@2@V?$allocator@UUnitSyncEntry@@@2@@std@@IAEXPAU_Node@12@@Z
// FUNCTION: 0x46fb80 ?_Insert@?$_Tree@IU?$pair@IUUnitSyncEntry@@@std@@U_Kfn@?$map@IUUnitSyncEntry@@U?$less@I@std@@V?$allocator@UUnitSyncEntry@@@3@@2@U?$less@I@2@V?$allocator@UUnitSyncEntry@@@2@@std@@IAE?AViterator@12@PAU_Node@12@0ABU?$pair@IUUnitSyncEntry@@@2@@Z
// FUNCTION: 0x46feb0 ?_Lrotate@?$_Tree@IU?$pair@IUUnitSyncEntry@@@std@@U_Kfn@?$map@IUUnitSyncEntry@@U?$less@I@std@@V?$allocator@UUnitSyncEntry@@@3@@2@U?$less@I@2@V?$allocator@UUnitSyncEntry@@@2@@std@@IAEXPAU_Node@12@@Z
// FUNCTION: 0x46ff10 ?_Rrotate@?$_Tree@IU?$pair@IUUnitSyncEntry@@@std@@U_Kfn@?$map@IUUnitSyncEntry@@U?$less@I@std@@V?$allocator@UUnitSyncEntry@@@3@@2@U?$less@I@2@V?$allocator@UUnitSyncEntry@@@2@@std@@IAEXPAU_Node@12@@Z
// FUNCTION: 0x46ff70 ?_Buynode@?$_Tree@IU?$pair@IUUnitSyncEntry@@@std@@U_Kfn@?$map@IUUnitSyncEntry@@U?$less@I@std@@V?$allocator@UUnitSyncEntry@@@3@@2@U?$less@I@2@V?$allocator@UUnitSyncEntry@@@2@@std@@IAEPAU_Node@12@PAU312@W4_Redbl@12@@Z
// FUNCTION: 0x46ff90 ?_Dec@iterator@?$_Tree@IU?$pair@IUUnitSyncEntry@@@std@@U_Kfn@?$map@IUUnitSyncEntry@@U?$less@I@std@@V?$allocator@UUnitSyncEntry@@@3@@2@U?$less@I@2@V?$allocator@UUnitSyncEntry@@@2@@std@@QAEXXZ
template class std::_Tree<unsigned int, std::pair<const unsigned int, UnitSyncEntry>, std::map<unsigned int, UnitSyncEntry>::_Kfn, std::less<unsigned int>, std::allocator<UnitSyncEntry> >;

void __stdcall NopChecksumEntryDtor(int);

namespace std {
template<> inline void allocator<Elem_0046faf0>::destroy(Elem_0046faf0* p)
{
    NopChecksumEntryDtor((int)p);
}
}

struct Elem_004702a0 {
    int unknown_0;
};

int* __stdcall CopyDwordRangeOverwrite(int* first, int* last, int* dest);

class Class_00470250 : public std::vector<Elem_004702a0> {
public:
    unsigned int Capacity() const;

    unsigned int __inline count() const
    {
        if (!_First)
            return 0;
        return (int)((char *)_Last - (char *)_First) >> 2;
    }
};

class Class_00470270 : public Class_00470250 {
public:
    unsigned int Size() const;

    __inline Elem_004702a0* begin() { return _First; }
    __inline Elem_004702a0* end() { return _Last; }
    __inline const Elem_004702a0* begin() const { return _First; }
    __inline const Elem_004702a0* end() const { return _Last; }

    static __inline void burn(int a)
    {
        int t = a;
        t = t * 3 + 1;
        (void)t;
    }

    void __inline assign_first(Class_00470270* d, const Class_00470270* s)
    {
        if (d == s)
            ;
        else if (s->count() <= d->count()) {
            int* p = (int*)s->_First;
            int* e = (int*)s->_Last;
            int* q = (int*)d->_First;
            for (; p != e; ++p, ++q)
                *q = *p;
            d->_Last = d->_First + s->Size();
        } else {
            unsigned int room = d->Capacity();
            if (s->Size() <= room) {
                Elem_004702a0* mid = s->_First;
                mid += d->Size();
                CopyDwordRangeOverwrite((int*)s->begin(), (int*)mid, (int*)d->_First);
                d->_Ucopy(mid, s->_Last, d->_Last);

                d->_Last = d->_First + s->Size();
            } else {
                d->_Destroy(d->_First, d->_Last);
                operator delete(d->begin());
                int n = (int)s->Size();
                if (n < 0)
                    n = 0;
                Elem_004702a0* p = (Elem_004702a0*)operator new(n * 4);
                d->_First = p;
                Elem_004702a0* q = d->_Ucopy(s->begin(), s->end(), p);
                d->_Last = q;
                d->_End = q;
            }
        }
    }

    void __inline assign_second(Class_00470270* d, const Class_00470270* s)
    {
        if (d == s)
            ;
        else if (s->Size() <= d->Size()) {
            Elem_004702a0* r = (Elem_004702a0*)CopyDwordRangeOverwrite(
                (int*)s->_First, (int*)s->_Last, (int*)d->_First);
            d->_Destroy(r, d->_Last);
            d->_Last = d->_First + s->Size();
        } else {
            unsigned int room = d->Capacity();
            if (s->Size() <= room) {
                Elem_004702a0* mid = s->_First;
                mid += d->Size();
                CopyDwordRangeOverwrite((int*)s->_First, (int*)mid, (int*)d->_First);
                d->_Ucopy(mid, s->_Last, d->_Last);

                d->_Last = d->_First + s->Size();
            } else {
                d->_Destroy(d->_First, d->_Last);
                operator delete((void*)d->_First);
                int n = (int)s->Size();
                if (n < 0)
                    n = 0;
                Elem_004702a0* p = (Elem_004702a0*)operator new(n * 4);
                d->_First = p;
                Elem_004702a0* q = d->_Ucopy(s->_First, s->_Last, p);
                d->_Last = q;
                d->_End = q;
            }
        }
    }
};

struct SyncPlayerRecord {
    int id;
    std::vector<Elem_004702a0> ids;
    std::vector<Elem_004702a0> pairs;
    int expected;
    int sent;
    int ackd;
    PacketSequencer sub;

    SyncPlayerRecord& operator=(const SyncPlayerRecord& src);
};

typedef std::list<int> List_0046eb60;
typedef List_0046eb60::iterator (List_0046eb60::*EraseFn_0046eb60)(List_0046eb60::iterator);

// FUNCTION: 0x46eb60 ?erase@?$list@HV?$allocator@H@std@@@std@@QAE?AViterator@12@V312@@Z
EraseFn_0046eb60 g_erase_0046eb60 = &List_0046eb60::erase;

// FUNCTION: 0x46f720
void UnitSyncMap::Init()
{
    std::_Lockit lock;
    if (DAT_0051e598 == 0) {
        DAT_0051e598 = Buynode(0, 1);
        DAT_0051e598->left = 0, DAT_0051e598->right = 0;
    }
    ++DAT_0051e59c;
    _Head = Buynode(DAT_0051e598, 0), _Size = 0;
    _Head->left = _Head, _Head->right = _Head;
}

typedef std::list<int> List_0046fac0;
typedef List_0046fac0::iterator (List_0046fac0::iterator::*PostIncFn_0046fac0)(int);

// FUNCTION: 0x46fac0 ??Eiterator@?$list@HV?$allocator@H@std@@@std@@QAE?AV012@H@Z
PostIncFn_0046fac0 g_postinc_0046fac0 = &List_0046fac0::iterator::operator++;

struct Node_0046fad0;

class InsertResult {
public:
    Node_0046fad0* first;
    bool second;

    InsertResult(Node_0046fad0* const& f, const bool& s);
};

// FUNCTION: 0x46fad0
InsertResult::InsertResult(Node_0046fad0* const& f, const bool& s)
    : first(f), second(s)
{
}

typedef std::vector<Elem_0046faf0> Vec_0046faf0;

#pragma auto_inline(off)
// FUNCTION: 0x46fe60
UnitSyncNode* UnitSyncMap::LowerBound(const unsigned int* key)
{
    std::_Lockit lock;
    UnitSyncNode* x = _Head->parent;
    UnitSyncNode* y = _Head;
    while (x != DAT_0051e598)
        if (key_compare(x->key, *key))
            x = x->right;
        else
            y = x, x = x->left;
    return y;
}
#pragma auto_inline(on)

// FUNCTION: 0x470030
void __stdcall NopChecksumEntryDtor(int)
{
}

// FUNCTION: 0x470040
SyncPlayerRecord& SyncPlayerRecord::operator=(const SyncPlayerRecord& src)
{
    id = src.id;

    ((Class_00470270*)&ids)->assign_first((Class_00470270*)&ids, (const Class_00470270*)&src.ids);
    ((Class_00470270*)&pairs)->assign_second((Class_00470270*)&pairs, (const Class_00470270*)&src.pairs);

    Class_00470270::burn(1);
    Class_00470270::burn(2);
    Class_00470270::burn(3);

    expected = src.expected;
    sent = src.sent;
    ackd = src.ackd;
    sub = src.sub;
    return *this;
}

// FUNCTION: 0x470250
unsigned int Class_00470250::Capacity() const
{
    if (!_First) {
        return 0;
    }
    return _End - _First;
}

// FUNCTION: 0x470270
unsigned int Class_00470270::Size() const
{
    if (_First == 0) {
        return 0;
    }
    return _Last - _First;
}

// FUNCTION: 0x4702d0
int* __stdcall CopyDwordRangeOverwrite(int* first, int* last, int* dest)
{
    for (; first != last; ++dest, ++first)
        *dest = *first;
    return dest;
}

typedef Vec_0046faf0::size_type (Vec_0046faf0::*SizeFn_00470770)() const;

// FUNCTION: 0x470770 ?size@?$vector@UElem_0046faf0@@V?$allocator@UElem_0046faf0@@@std@@@std@@QBEIXZ
SizeFn_00470770 g_size_00470770 = &Vec_0046faf0::size;

typedef Vec_0046faf0& (Vec_0046faf0::*AssignFn_0046faf0)(const Vec_0046faf0&);

// FUNCTION: 0x4707a0 ??4?$vector@UElem_0046faf0@@V?$allocator@UElem_0046faf0@@@std@@@std@@QAEAAV01@ABV01@@Z
AssignFn_0046faf0 g_assign_0046faf0 = &Vec_0046faf0::operator=;

// FUNCTION: 0x46ca60
void __fastcall FinishUnitSync()
{
    ((UnitSync *)g_game->sync)->ApplyToUnitTypes();
    delete (UnitSyncDeleter *)(UnitSync *)g_game->sync;
    (*(UnitSync **)&g_game->sync) = 0;
}

// FUNCTION: 0x46cc10
void PacketSequencer::SendSequenced(Elem_0046faf0* to, Elem_0046faf0* packet)
{
    packet->packet.id = ++lastSent;
    sentQueue.insert(sentQueue.end(), 1, *packet);
    SendPacketToPlayer(GetLocalHumanDpid(), (unsigned int)to, (unsigned char *)packet, 0xe);
}

typedef std::vector<int> IntVector;
typedef void (IntVector::*IntVectorInsertFn)(
    IntVector::iterator, IntVector::size_type, int const&);

void __stdcall GrowIntVector(IntVector* v, int extra)
{
    v->reserve(extra + v->size());
}

// FUNCTION: 0x46e640 ?insert@?$vector@HV?$allocator@H@std@@@std@@QAEXPAHIABH@Z
IntVectorInsertFn g_intVectorInsert = &IntVector::insert;

typedef std::vector<UnitSyncPacket> SyncPacketVector;
typedef void (SyncPacketVector::*SyncPacketInsertFn)(
    SyncPacketVector::iterator, SyncPacketVector::size_type,
    const UnitSyncPacket&);

void __stdcall PushSyncPacket(SyncPacketVector* v, const UnitSyncPacket& x)
{
    v->push_back(x);
}

// FUNCTION: 0x46eba0 ?insert@?$vector@UUnitSyncPacket@@V?$allocator@UUnitSyncPacket@@@std@@@std@@QAEXPAUUnitSyncPacket@@IABU3@@Z
SyncPacketInsertFn g_syncPacketInsert = &SyncPacketVector::insert;

inline UnitSyncPlayer& UnitSyncPlayer::operator=(const UnitSyncPlayer& rhs)
{
    return (UnitSyncPlayer&)((SyncPlayerRecord*)this)->operator=(*(const SyncPlayerRecord*)&rhs);
}

typedef std::vector<UnitSyncPlayer> SyncPlayerVector;
typedef void (SyncPlayerVector::*SyncPlayerInsertFn)(
    SyncPlayerVector::iterator, SyncPlayerVector::size_type,
    const UnitSyncPlayer&);

// FUNCTION: 0x46f7a0 ?insert@?$vector@VUnitSyncPlayer@@V?$allocator@VUnitSyncPlayer@@@std@@@std@@QAEXPAVUnitSyncPlayer@@IABV3@@Z
SyncPlayerInsertFn g_syncPlayerInsert = &SyncPlayerVector::insert;
