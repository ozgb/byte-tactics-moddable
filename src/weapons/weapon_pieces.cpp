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

#include "../scripts/object_view.h"
#include "../units/unit.h"

#include "../../link/globals.h"

#include "../../interfaces/seams.h"
#include "weapon_pieces.h"

// FUNCTION: 0x43e1e0
int __stdcall QueryWeaponPiece(Object* obj, unsigned char weapon)
{
    char* names[3] = { "QueryPrimary", "QuerySecondary", "QueryTertiary" };
    int piece = 0;
    g_seams.scripts->QueryScript(obj->script, names[weapon], &piece, 0, 0, 0);
    return piece;
}

// FUNCTION: 0x43e240
void __stdcall GetWeaponPiecePosition(Object* obj, Vec3* out, unsigned char weapon, int piece)
{
    if (piece < 0)
        piece = QueryWeaponPiece(obj, weapon);
    *out = obj->pos + g_seams.scripts->GetPieceOffset((Unit*)obj, piece);
}

