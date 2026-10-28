#ifndef UNIT_SYNC_STATE_H
#define UNIT_SYNC_STATE_H

#include <list>
#include <map>
#include <vector>

class UnitDef;
struct UnitSyncTarget;
struct UnitSyncSource;

struct UnitSyncEntry {
    int fbiChecksum;
    int scriptChecksum;
    short enabled;
    short peerEnabled;
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
#pragma pack(pop)

#pragma pack(push, 2)
struct UnitSyncPacketElem {
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

struct SyncMapPredArg {
    char x;
};

struct SyncMapAllocArg {
    char x;
};

struct UnitSyncMapKeyFn {};
struct UnitSyncMapAlloc {};

struct UnitSyncMapLess {
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

typedef std::pair<const unsigned int, UnitSyncEntry> UnitSyncMapValue;

class UnitSyncMap : public _Tree<unsigned int, UnitSyncMapValue, UnitSyncMapKeyFn,
                                  UnitSyncMapLess, UnitSyncMapAlloc> {
public:
    UnitSyncMap(SyncMapPredArg c, SyncMapAllocArg a)
    {
        _Alloc = c.x;
        _Pred = a.x;
        _Multi = 0;
        Init();
    }

    ~UnitSyncMap();

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

    void Erase(UnitSyncNode* x);
    void Init();

    UnitSyncEntry& operator[](const unsigned int& k)
    {
        _Pairib p = insert(UnitSyncMapValue(k, UnitSyncEntry()));
        return p.first->value;
    }
};

class UnitSyncSeqView {
public:
    int seqSent;
    int seqCur;
    int seqMax;

    void SendUnsequenced(void* packet, int to);
};

class UnitSyncSharedView {
public:
    int direct;
    int* pendingPlayerCount;
    int* checksumProgress;
    int* disabled;
};

class SyncChecksumVector {
public:
    std::vector<int> vec;
    ~SyncChecksumVector();
};

struct PacketSequencer {
    int lastSent;
    unsigned int cur;
    unsigned int max;
    std::vector<UnitSyncPacketElem> sentQueue;
    std::vector<UnitSyncPacketElem> heldQueue;
    PacketSequencer();
    void Inl(unsigned int param_1, UnitSyncPacket* param_2);
    PacketSequencer& operator=(const PacketSequencer& rhs);
    void SendUnsequenced(unsigned int param_1, void* param_2);
    void SendSequenced(UnitSyncPacketElem* param_1, UnitSyncPacketElem* param_2);
    void ReceiveSequenced(UnitSyncPacket* packet, int param_2, void* param_3, unsigned int target);
};

class UnitSyncPlayer {
public:
    unsigned int id;
    std::vector<int> ids;
    std::vector<int> pairs;
    int expected;
    int sent;
    unsigned int ackd;
    PacketSequencer sub;

    UnitSyncPlayer() : id(0), expected(0), sent(0), ackd(0) {}
    UnitSyncPlayer(const UnitSyncPlayer& other);
    ~UnitSyncPlayer();
    UnitSyncPlayer& operator=(const UnitSyncPlayer& rhs);
};


class UnitSync {
public:
    UnitSyncMap map;
    std::vector<UnitSyncPlayer> players;
    std::list<int> ids;
    UnitSyncSeqView sub;
    SyncChecksumVector seqSentQueue;
    std::vector<int> seqHeldQueue;
    union {
        UnitSyncSharedView sub2;
        struct {
            int direct;
            union {
                int* first;
                int pendingPlayerCount;
            };
            int checksumProgress;
            int disabled;
        };
    };

    UnitSync(int param);
    ~UnitSync();

    UnitSyncIter End() { return UnitSyncIter(map._Head); }
    UnitSyncIter Find(const unsigned int* key)
    {
        UnitSyncIter p = UnitSyncIter(map.LowerBound(key));
        return (p == End() || map.key_compare(*key, p.ptr->key)) ? End() : p;
    }
    void Send(UnitSyncTarget* target, UnitSyncPacket* packet);

    void ProcessSync();
    void SendSyncPacket(unsigned int* param_1, UnitSyncPacket* param_2, int unused);
    void SendSyncMessage(unsigned char arg, int a, int b, int unused);
    void SendSyncMessageTo(UnitSyncTarget* target, unsigned char arg, int a, int b, int unused);
    void SendEntryTo(UnitSyncTarget* target, unsigned char arg, UnitSyncSource* src, int unused);
    void HandleSyncPacket(UnitSyncPacket* packet, unsigned char player);
    void ReceiveSyncPacket(void* param_1, int param_2);
    void NotifyEntryChanged(unsigned int param_1);
    void CheckUnitAvailable(unsigned int key, int y);
    char* GetSyncStatusText();
    int AllPlayersSynced();
    int IsPlayerSynced(int id);
    int PopChangedEntry(UnitSyncEntry* out);
    int GetUnitEntry(UnitDef* unit, UnitSyncEntry* out);
    int ToggleUnitAllowed(UnitDef* unit);
    int DisallowUnit(UnitDef* unit);
    int AllowUnit(UnitDef* unit);
    void SetUnitLimit(UnitDef* unit, int value);
    void ApplyToUnitTypes();
    void ResetEntries();
};

#endif
