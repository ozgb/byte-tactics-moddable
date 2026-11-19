#include "win_types.h"
#include "../../interfaces/ref.h"
#include <string.h>

#include "../util/vec3.h"
#include "../util/angles.h"

#include "../scripts/cob_script.h"

#include "../map/mission.h"
#include "../sound/sound.h"

struct UnitCursorView;
struct FeatureCursorView;

#include "../units/unit_def.h"

#pragma pack(push, 1)
#include "../game/game.h"

#pragma pack(pop)

#include "../units/unit.h"

#include "../../link/globals.h"

#include "../../interfaces/seams.h"
#include "order_cursor.h"

// FUNCTION: 0x43e470
int __stdcall OrderModeTakesTarget(unsigned char type)
{
    if (type != 5 && type != 10 && type != 14)
        return 1;
    return 0;
}
