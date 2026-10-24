// FLAGS: /Gi
#include <vector>
#include "../state/sim_allocator.h"

#include "../units/unit.h"

typedef SimVector<Unit*> Vec_00408f30;
typedef void (Vec_00408f30::*InsertFn_00408f30)(
    Vec_00408f30::iterator, Vec_00408f30::size_type, Unit* const&);

void __stdcall Grow_00408f30(Vec_00408f30* units, int extra)
{
    units->reserve(extra + units->size());
}

// FUNCTION: 0x408f30 ?insert@?$vector@PAUUnit@@V?$allocator@PAUUnit@@@std@@@std@@QAEXPAPAUUnit@@IABQAU3@@Z
InsertFn_00408f30 g_insert_00408f30 = &Vec_00408f30::insert;
