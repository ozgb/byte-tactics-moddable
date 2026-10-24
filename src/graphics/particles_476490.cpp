// FLAGS: /Gi
#include <vector>

struct Elem_00476490 { int dwords[8]; };

typedef std::vector<Elem_00476490> Vec_00476490;
typedef void (Vec_00476490::*InsertFn_00476490)(
    Vec_00476490::iterator, Vec_00476490::size_type, const Elem_00476490&);

void __stdcall Assign_00476490(Vec_00476490* to, const Vec_00476490* from)
{
    *to = *from;
}

// FUNCTION: 0x476490 ?insert@?$vector@UElem_00476490@@V?$allocator@UElem_00476490@@@std@@@std@@QAEXPAUElem_00476490@@IABU3@@Z
InsertFn_00476490 g_insert_00476490 = &Vec_00476490::insert;
