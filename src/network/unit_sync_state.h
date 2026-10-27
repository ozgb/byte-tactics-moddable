#ifndef UNIT_SYNC_STATE_H
#define UNIT_SYNC_STATE_H

#include <list>
#include <map>
#include <vector>

struct UnitSyncDef;
struct Target_0046d530;
struct Source_0046d630;

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

struct Cmp_0046d040 {
    char x;
};

struct Alloc_0046d040 {
    char x;
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

class SyncChecksumVector {
public:
    std::vector<int> vec;
    ~SyncChecksumVector();
};

struct PacketSequencer {
    int lastSent;
    unsigned int cur;
    unsigned int max;
    std::vector<Elem_0046faf0> sentQueue;
    std::vector<Elem_0046faf0> heldQueue;
    PacketSequencer();
    void Inl(unsigned int param_1, UnitSyncPacket* param_2);
    PacketSequencer& operator=(const PacketSequencer& rhs);
    void SendUnsequenced(unsigned int param_1, void* param_2);
    void SendSequenced(Elem_0046faf0* param_1, Elem_0046faf0* param_2);
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
    Sub_0046d040 sub;
    SyncChecksumVector seqSentQueue;
    std::vector<int> seqHeldQueue;
    union {
        Sub2_0046d040 sub2;
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
    void Send(Target_0046d530* target, UnitSyncPacket* packet);

    void ProcessSync();
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
    int GetUnitEntry(UnitSyncDef* unit, UnitSyncEntry* out);
    int ToggleUnitAllowed(UnitSyncDef* unit);
    int DisallowUnit(UnitSyncDef* unit);
    int AllowUnit(UnitSyncDef* unit);
    void SetUnitLimit(UnitSyncDef* unit, int value);
    void ApplyToUnitTypes();
    void ResetEntries();
};

#endif
