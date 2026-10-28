#ifndef LOS_TABLES_H
#define LOS_TABLES_H

#include "../util/string_ref.h"
struct StringValueElem;

struct LosStep;
#ifdef __clang__
namespace std {
    template<class T> class allocator;
    template<class T, class A> class vector;
}
#define LOS_ELEM_VECTOR std::vector<LosStep, std::allocator<LosStep> >
#else
namespace std {
    template<class T> class allocator;
    template<class T, class A = allocator<T> > class vector;
}
#define LOS_ELEM_VECTOR std::vector<LosStep>
#endif

class LosTable;
class TdfFile;

struct DamageTableEntry {
    StringRef name;
    int value;

    DamageTableEntry(const StringRef& n, int v) : name(n), value(v) {}
    void* AssignPair(DamageTableEntry* param);
};

class LosTables {
public:
    explicit LosTables(const std::allocator<int>& al = std::allocator<int>())
        : allocator(al), first(0), last(0), end(0) {}
    ~LosTables() { FreeTables(); }

    void FreeTables();
    void LoadLosTable(TdfFile* file, short table);
    LOS_ELEM_VECTOR* GetLosTable(int n);
    int GetLosTableCount();
    void LoadLosTables();
    void ResizeTables(short n);

    std::allocator<int> allocator;
    int* first;
    int* last;
    int* end;
};

void __stdcall AssignRange(StringValueElem* first, StringValueElem* last, StringValueElem* value);
StringValueElem* __stdcall AssignRangeBack(StringValueElem*, StringValueElem*, StringValueElem*);
void __stdcall DestroyPoint(int);

void __stdcall StoreDwordIfDst(int*, int*);

#endif
