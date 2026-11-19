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
#include "sfx_occupy.h"

// FUNCTION: 0x43db50
void SfxOccupyTracker::UpdateSfxOccupy(Unit* unit)
{
    int seaLevel = g_game->seaLevel;
    int y = unit->yWhole;
    int newState = *(int*)unit->unknown_10a;
    if ((unit->flags & 3) == 1 || (unit->flags & 3) == 2) {
        if (y > seaLevel) {
            newState = 4;
        } else {
            if (y - seaLevel > -5)
                newState = 1;
            if (unit->def->draft + y == seaLevel)
                newState = 2;
            if (unit->def->modelMaxYHigh + y < seaLevel)
                newState = 3;
        }
    } else {
        newState = 0;
    }
    if (*(int*)unit->unknown_10a != newState) {
        g_seams.scripts->StartScriptWithArgs((CobScript*)unit->script.get(), "setSFXoccupy", 0, 1, 1, newState, 0, 0, 0);
        *(int*)unit->unknown_10a = newState;
    }
}
