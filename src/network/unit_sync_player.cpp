#include <map>
#include <list>
#include <vector>

#pragma pack(push, 2)
struct Elem_0046faf0 {
    int a;
    int b;
    int c;
    short d;
};
#pragma pack(pop)

struct Rect_0046e330 {
    int x;
    int y;
    short w;
    short h;
    int limit;
};

struct Pair_0046faf0 {
    std::vector<Elem_0046faf0> sentQueue;
    std::vector<Elem_0046faf0> heldQueue;
    Pair_0046faf0(const Pair_0046faf0& other)
        : sentQueue(other.sentQueue), heldQueue(other.heldQueue)
    {
    }
};

class UnitSyncPlayer {
public:
    int id;
    std::vector<int> ids;
    std::vector<int> pairs;
    int expected;
    int sent;
    int ackd;
    int lastSent;
    int cur;
    int max;
    Pair_0046faf0 pair;

    UnitSyncPlayer(const UnitSyncPlayer& other);
    ~UnitSyncPlayer();
};

// FUNCTION: 0x46ded0
UnitSyncPlayer::~UnitSyncPlayer()
{
}

class SyncChecksumVector {
public:
    std::vector<int> vec;
};

class UnitSync {
public:
    std::map<unsigned int, Rect_0046e330> rects;
    std::vector<UnitSyncPlayer> elems;
    std::list<int> ids;
    int seqSent;
    int seqCur;
    int seqMax;
    SyncChecksumVector seqSentQueue;
    std::vector<int> seqHeldQueue;
    int direct;
    int pendingPlayerCount;
    int checksumProgress;
    int disabled;

    void ApplyToUnitTypes();
};

#include "../game/game.h"

extern Game* g_game;

// FUNCTION: 0x470300 ??_GUnitSyncPlayer@@QAEPAXI@Z
void FinishUnitSync()
{
    ((UnitSync *)g_game->sync)->ApplyToUnitTypes();
    if ((UnitSync *)g_game->sync)
        delete (UnitSync *)g_game->sync;
    (*(UnitSync **)&g_game->sync) = 0;
}

// FUNCTION: 0x470390
UnitSyncPlayer::UnitSyncPlayer(const UnitSyncPlayer& other)
    : id(other.id), ids(other.ids), pairs(other.pairs),
      expected(other.expected), sent(other.sent), ackd(other.ackd),
      lastSent(other.lastSent), cur(other.cur), max(other.max),
      pair(other.pair)
{
}
