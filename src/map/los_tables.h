#ifndef LOS_TABLES_H
#define LOS_TABLES_H

#include "../util/string_ref.h"

struct Elem_00434020;
#ifdef __clang__
namespace std {
    template<class T> class allocator;
    template<class T, class A> class vector;
}
#define LOS_ELEM_VECTOR std::vector<Elem_00434020, std::allocator<Elem_00434020> >
#else
namespace std {
    template<class T> class allocator;
    template<class T, class A = allocator<T> > class vector;
}
#define LOS_ELEM_VECTOR std::vector<Elem_00434020>
#endif

class LosTable;
class TdfFile;

struct Entry_00432cf0 {
    StringRef name;
    int value;

    Entry_00432cf0(const StringRef& n, int v) : name(n), value(v) {}
    void* AssignPair(Entry_00432cf0* param);
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

#endif
