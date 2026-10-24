#include <vector>

#include "../units/unit.h"

typedef std::vector<Unit*> Vec_00406c10;
typedef Vec_00406c10::iterator (Vec_00406c10::*UcopyFn_00406c10)(
    Vec_00406c10::const_iterator, Vec_00406c10::const_iterator, Vec_00406c10::iterator);

struct Access_00406c10 : Vec_00406c10 {
    static UcopyFn_00406c10 fn;
};

// FUNCTION: 0x406c10 ?_Ucopy@?$vector@PAUUnit@@V?$allocator@PAUUnit@@@std@@@std@@IAEPAPAUUnit@@PBQAU3@0PAPAU3@@Z
UcopyFn_00406c10 Access_00406c10::fn = &Access_00406c10::_Ucopy;
