#include <vector>

struct Elem_00434020 {
    unsigned short a;
    unsigned short b;
};

#include "los_tables.h"
void __stdcall StoreDwordIfDst(int* p, const int* q);

namespace std {
inline void _Destroy(Elem_00434020* p)
{
    DestroyPoint((int)p);
}

inline void _Construct(Elem_00434020* p, const Elem_00434020& v)
{
    StoreDwordIfDst((int*)p, (const int*)&v);
}
}

struct Elem_00434360 {
    std::vector<Elem_00434020> v;
};

typedef std::vector<Elem_00434020> Inner_00433a80;

namespace std {
inline void _Destroy(Elem_00434360* p)
{
    p->v.~Inner_00433a80();
}
}

typedef std::vector<Elem_00434360> Middle_00433a80;
typedef std::vector<Middle_00433a80> Outer_00433a80;
typedef Outer_00433a80& (Outer_00433a80::*AssignFn_00433a80)(const Outer_00433a80&);

AssignFn_00433a80 g_assign_00433a80 = &Outer_00433a80::operator=;

// FUNCTION: 0x433a80 ??1?$vector@UElem_00434360@@V?$allocator@UElem_00434360@@@std@@@std@@QAE@XZ

typedef std::vector<Elem_00434360> Outer_00434020;
typedef Outer_00434020::iterator (Outer_00434020::*EraseFn_00434020)(
    Outer_00434020::iterator, Outer_00434020::iterator);

// FUNCTION: 0x434020 ?erase@?$vector@UElem_00434360@@V?$allocator@UElem_00434360@@@std@@@std@@QAEPAUElem_00434360@@PAU3@0@Z
EraseFn_00434020 g_erase_00434020 = &Outer_00434020::erase;

typedef std::vector<Elem_00434020> Inner_004344e0;
typedef std::vector<Elem_00434360> Middle_004344e0;
typedef std::vector<Middle_004344e0> Outer_004344e0;
typedef void (Outer_004344e0::*InsertFn_004344e0)(
    Outer_004344e0::iterator, Outer_004344e0::size_type, const Middle_004344e0&);

// FUNCTION: 0x4344e0 ??0?$vector@UElem_00434360@@V?$allocator@UElem_00434360@@@std@@@std@@QAE@ABV01@@Z
InsertFn_004344e0 g_insert_004344e0 = &Outer_004344e0::insert;
