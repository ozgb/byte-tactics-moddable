// FLAGS: /Gi
#include <vector>

#include "../util/vec3.h"

typedef std::vector<Point16> Vec_0044ec30;
typedef void (Vec_0044ec30::*InsertFn_0044ec30)(
    Vec_0044ec30::iterator, Vec_0044ec30::size_type, const Point16&);

void __stdcall Grow_0044ec30(Vec_0044ec30* v, int extra)
{
    v->reserve(extra + v->size());
}

// FUNCTION: 0x44ec30 ?insert@?$vector@UPoint16@@V?$allocator@UPoint16@@@std@@@std@@QAEXPAUPoint16@@IABU3@@Z
InsertFn_0044ec30 g_insert_0044ec30 = &Vec_0044ec30::insert;
