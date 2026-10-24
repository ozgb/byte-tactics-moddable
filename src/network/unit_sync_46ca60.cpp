#include <list>
#include <map>
#include <vector>
#include <xmemory>

#pragma pack(push, 2)
struct Elem_0046faf0 {
    int a;
    int b;
    int c;
    short d;
};
#pragma pack(pop)

typedef std::list<int> List_0046ca60;
typedef List_0046ca60::iterator (List_0046ca60::*EraseFn_0046ca60)(List_0046ca60::iterator);
typedef List_0046ca60::iterator (List_0046ca60::iterator::*PostIncFn_0046ca60)(int);

EraseFn_0046ca60 g_erase_0046ca60 = &List_0046ca60::erase;
PostIncFn_0046ca60 g_postinc_0046ca60 = &List_0046ca60::iterator::operator++;

struct UnitSyncEntry {
    int x;
    int y;
    short w;
    short h;
    int limit;
};

class VecInt_0046ca60 {
public:
    std::allocator<int> alloc;
    int* _First;
    int* _Last;
    int* _End;

    ~VecInt_0046ca60()
    {
        operator delete(_First);
        _First = 0, _Last = 0, _End = 0;
    }
};

class UnitSyncPlayer {
public:
    int id;
    VecInt_0046ca60 ids;
    VecInt_0046ca60 pairs;
    char unknown_24[0x18];
    VecInt_0046ca60 sentQueue;
    VecInt_0046ca60 heldQueue;
};

static inline void DestroyR0_0046ca60(UnitSyncPlayer* _F, UnitSyncPlayer* _L);
static inline void DestroyR1_0046ca60(UnitSyncPlayer* _F, UnitSyncPlayer* _L);
static inline void DestroyR2_0046ca60(UnitSyncPlayer* _F, UnitSyncPlayer* _L);
class VecElems_0046ca60 {
public:
    std::allocator<UnitSyncPlayer> alloc;
    UnitSyncPlayer* _First;
    UnitSyncPlayer* _Last;
    UnitSyncPlayer* _End;

    void _Destroy(UnitSyncPlayer* _F, UnitSyncPlayer* _L)
    {
        for (; _F != _L; ++_F)
            alloc.destroy(_F);
    }

    ~VecElems_0046ca60()
    {
        DestroyR0_0046ca60(_First, _Last);

        operator delete(_First);
        _First = 0, _Last = 0, _End = 0;
    }
};

static inline void DestroyR0_0046ca60(UnitSyncPlayer* _F, UnitSyncPlayer* _L)
{
    DestroyR1_0046ca60(_F, _L);
}

static inline void DestroyR1_0046ca60(UnitSyncPlayer* _F, UnitSyncPlayer* _L)
{
    DestroyR2_0046ca60(_F, _L);
}

static inline void DestroyR2_0046ca60(UnitSyncPlayer* _F, UnitSyncPlayer* _L)
{
    std::allocator<UnitSyncPlayer> alloc;
    for (UnitSyncPlayer* p = _F; p != _L; ++p)
        alloc.destroy(p);
}

class UnitSync {
public:
    std::map<unsigned int, UnitSyncEntry> rects;
    VecElems_0046ca60 elems;
    std::list<int> ids;
    int seqSent;
    int seqCur;
    int seqMax;
    std::vector<Elem_0046faf0> seqSentQueue;
    std::vector<Elem_0046faf0> seqHeldQueue;
    int direct;
    int pendingPlayerCount;
    int checksumProgress;
    int disabled;

    ~UnitSync() {}
    void ApplyToUnitTypes();
};

#include "../game/game.h"

extern Game* g_game;

// FUNCTION: 0x46ca60
void __fastcall FinishUnitSync()
{
    ((UnitSync *)g_game->sync)->ApplyToUnitTypes();
    delete (UnitSync *)g_game->sync;
    (*(UnitSync **)&g_game->sync) = 0;
}
