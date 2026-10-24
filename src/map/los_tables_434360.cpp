#include <vector>

struct Elem_00434020 {
    unsigned short a;
    unsigned short b;
};

struct Elem_00434360 {
    std::vector<Elem_00434020> v;
};

void __stdcall DestroyLine(Elem_00434360* p);

namespace std {
inline void _Destroy(Elem_00434360* p)
{
    DestroyLine(p);
}
}

typedef std::vector<Elem_00434360> Inner_00434360;
typedef std::vector<Inner_00434360> Outer_00434360;
typedef Outer_00434360::iterator (Outer_00434360::*EraseFn_00434360)(
    Outer_00434360::iterator, Outer_00434360::iterator);

// FUNCTION: 0x434360 ?erase@?$vector@V?$vector@UElem_00434360@@V?$allocator@UElem_00434360@@@std@@@std@@V?$allocator@V?$vector@UElem_00434360@@V?$allocator@UElem_00434360@@@std@@@std@@@2@@std@@QAEPAV?$vector@UElem_00434360@@V?$allocator@UElem_00434360@@@std@@@2@PAV32@0@Z
EraseFn_00434360 g_erase_00434360 = &Outer_00434360::erase;
