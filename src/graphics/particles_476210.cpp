// FLAGS: /Gi
#include <vector>

struct Elem_00476210 {
    int dwords[8];
};

typedef std::vector<Elem_00476210> Vec_00476210;
typedef void (Vec_00476210::*InsertFn_00476210)(
    Vec_00476210::iterator, Vec_00476210::size_type, const Elem_00476210&);

void __stdcall Grow_00476210(Vec_00476210* records, int periods)
{
    records->reserve(periods + records->size());
}

// FUNCTION: 0x476210 ?insert@?$vector@UElem_00476210@@V?$allocator@UElem_00476210@@@std@@@std@@QAEXPAUElem_00476210@@IABU3@@Z
InsertFn_00476210 g_insert_00476210 = &Vec_00476210::insert;
