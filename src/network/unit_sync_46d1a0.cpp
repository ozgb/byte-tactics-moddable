#include <list>
#include <map>
#include <vector>

#pragma pack(push, 2)
struct Elem_0046faf0 {
    int a;
    int b;
    int c;
    short d;
};
#pragma pack(pop)

struct UnitSyncEntry {
    int x;
    int y;
    short w;
    short h;
    int limit;
};

class UnitSyncPlayer {
public:
    char unknown_0[0x5c];
    ~UnitSyncPlayer();
};

class Vec_0046d1a0 {
public:
    std::allocator<int> alloc;
    int* _First;
    int* _Last;
    int* _End;

    void _Destroy(int* _F, int* _L)
    {
        for (; _F != _L; ++_F)
            alloc.destroy(_F);
    }

    ~Vec_0046d1a0()
    {
        _Destroy(_First, _Last);
        alloc.deallocate(_First, _End - _First);
        _First = 0, _Last = 0, _End = 0;
    }
};

class VecElems_0046d1a0 {
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

    ~VecElems_0046d1a0()
    {
        _Destroy(_First, _Last);
        alloc.deallocate(_First, _End - _First);
        _First = 0, _Last = 0, _End = 0;
    }
};

typedef std::list<int> List_0046d1a0;
typedef List_0046d1a0::iterator (List_0046d1a0::*EraseFn_0046d1a0)(List_0046d1a0::iterator);
typedef List_0046d1a0::iterator (List_0046d1a0::iterator::*PostIncFn_0046d1a0)(int);

EraseFn_0046d1a0 g_erase_0046d1a0 = &List_0046d1a0::erase;
PostIncFn_0046d1a0 g_postinc_0046d1a0 = &List_0046d1a0::iterator::operator++;

class ListWrap_0046d1a0 : public List_0046d1a0 {
};

class UnitSync {
public:
    std::map<unsigned int, UnitSyncEntry> rects;
    VecElems_0046d1a0 elems;
    ListWrap_0046d1a0 ids;
    int seqSent;
    int seqCur;
    int seqMax;
    std::vector<Elem_0046faf0> seqSentQueue;
    Vec_0046d1a0 seqHeldQueue;
    int direct;
    int pendingPlayerCount;
    int checksumProgress;
    int disabled;

    ~UnitSync();
};

// FUNCTION: 0x46d1a0
UnitSync::~UnitSync()
{
}
