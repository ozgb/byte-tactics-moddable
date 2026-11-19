


#include "../map/mission.h"

struct UnitCursorView;
struct FeatureCursorView;

#include "../units/unit_def.h"

#pragma pack(push, 1)

#pragma pack(pop)



#include "order_cursor.h"

// FUNCTION: 0x43e470
int __stdcall OrderModeTakesTarget(unsigned char type)
{
    if (type != 5 && type != 10 && type != 14)
        return 1;
    return 0;
}
