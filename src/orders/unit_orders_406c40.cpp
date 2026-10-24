#include <vector>

#include "../units/unit.h"

typedef std::vector<Unit*> Vec_00406c40;
typedef void (Vec_00406c40::*UfillFn_00406c40)(
    Vec_00406c40::iterator, Vec_00406c40::size_type, Unit* const&);

struct Access_00406c40 : Vec_00406c40 {
    static UfillFn_00406c40 fn;
};

// FUNCTION: 0x406c40 ?_Ufill@?$vector@PAUUnit@@V?$allocator@PAUUnit@@@std@@@std@@IAEXPAPAUUnit@@IABQAU3@@Z
UfillFn_00406c40 Access_00406c40::fn = &Access_00406c40::_Ufill;
