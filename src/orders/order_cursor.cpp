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

#pragma pack(push, 1)
#include "../orders/order_cursor_views.h"


struct UnitDefCursorFlags {
    unsigned int b0 : 1;
    unsigned int b1 : 1;
    unsigned int b2 : 1;
    unsigned int b3 : 1;
    unsigned int b4 : 1;
    unsigned int b5 : 1;
    unsigned int b6 : 1;
    unsigned int b7 : 1;
    unsigned int b8 : 1;
    unsigned int b9 : 1;
    unsigned int b10 : 1;
    unsigned int b11 : 1;
    unsigned int b12 : 1;
    unsigned int b13 : 1;
    unsigned int b14 : 1;
    unsigned int b15_31 : 17;
};

struct UnitDefCursorFlagsView {
    char unknown_0[0x241];
    UnitDefCursorFlags f241b;
    UnitDefCursorFlags f245b;
};

#include "../network/player.h"

#include "../map/cell.h"

#pragma pack(pop)

static inline int Visible(UnitCursorView* unit, FixedPosView* pos) {
    Player* p = unit->player;
    int y, x;
    x = pos->x >> 5;
    y = (pos->z - (pos->y >> 1)) >> 5;
    return (unsigned int)x < p->exploredWidth && (unsigned int)y < p->exploredHeight &&
           0 != ((1 << g_game->playerIndex) & g_game->visibilityMask[x + y * unit->player->exploredWidth]);
}

static inline FeatureCursorView* GetFeature(Cell* cell) {
    if (cell == 0)
        return 0;
    unsigned short id = cell->feature;
    if (id < 0xfffb) {
        if (id >= g_game->featureCount)
            return 0;
        return &((FeatureCursorView *)g_game->features.get())[id];
    }
    if (id != 0xfffe)
        return 0;
    id = (cell - (cell->offsetY * g_game->width + cell->offsetX))->feature;
    if (id >= 0xfffb)
        return 0;
    return &((FeatureCursorView *)g_game->features.get())[id];
}

#define RECLAIM_CHECK(def, unit, pos, mask, result)                  \
    do {                                                             \
        if (((def)->flags2 & (mask)) && Visible((unit), (pos))) {      \
            FeatureCursorView* f = GetFeature(g_seams.los_radar->GetMapCellAtPosition((Vec3*)(pos)));     \
            if (f && (f->flags & 0x80))                              \
                return (result);                                     \
        }                                                            \
    } while (0)

static inline int Selectable(UnitCursorView* t) {
    return t && t->playerIndex == g_game->localPlayer && (t->flags & 0x20) && t->buildLeft == 0.0f && t->postTransferHoldoff == 0 &&
           (t->carrier == 0 || (t->carrier->flags & 0x40000000));
}

// FUNCTION: 0x43e490
int __stdcall GetOrderCursor(unsigned char mode, UnitCursorView* unit, UnitCursorView* target,
                           FixedPosView* pos) {
    UnitDef* def;
    WeaponDefCursorView* node;
    int friendly;
    int enemy;
    friendly = 0;
    enemy = 0;
    def = unit->def;
    if (target) {
        if (unit->player->allied[target->player->index])
            friendly = 1;
        else
            enemy = 1;
    }

    switch (mode) {
    case 3:
        if ((def->flags2 & 0x10) && ((WeaponDefCursorView*)def->weapons[0].get())->f111b.b8)
            return 2;
        if (((UnitDefCursorFlagsView*)def)->f245b.b4) {
            node = unit->f10;
            if (unit->motion)
                return 1;
            if (target)
                return g_seams.weapons->WeaponCanReachUnit((Unit*)unit, (Unit*)target, 0) ? 1 : 3;
            if (!g_seams.weapons->WeaponCanReachPos((Unit*)unit, (Vec3*)unit->f6a, (Vec3*)pos, 0) || (node->f111 & 0x20000))
                return 3;
            return 1;
        }
        break;
    case 9:
        return ((UnitDefCursorFlagsView*)def)->f245b.b6 ? 7 : 0x13;
    case 8:
        return g_seams.units->CanRepair(((Unit*)unit), 0, (Unit*)target) ? 6 : 0x13;
    case 7:
        if (!(def->flags2 & 0x20) || !friendly)
            break;
        if ((def->flags1 & 0x800) || !(target->def->flags1 & 0x800))
            return 5;
        break;
    case 12:
        RECLAIM_CHECK(def, unit, pos, 0x400, 0xb);
        if (target && g_seams.units->CanReclaim(((Unit*)unit), 0, (Unit*)target))
            return 0xb;
        break;
    case 13:
        if ((def->flags2 & 0x1000) && target && unit->player != target->player)
            return 4;
        break;
    case 6:
        if (!target || !g_seams.units->CanLoad(((Unit*)unit), 0, (Unit*)target))
            break;
        return ((UnitDefCursorFlagsView*)def)->f241b.b11 ? 8 : 0xc;
    case 5:
        return ((UnitDefCursorFlagsView*)def)->f245b.b8 ? 0xd : 0x13;
    case 14:
        if (def->ids == 0 || unit->motion == 0)
            break;
        return 0x10;
    case 4:
        if (((UnitDefCursorFlagsView*)def)->f245b.b14) {
            WeaponDefShotCostView* l = unit->f48;
            PlayerResourceView* s = unit->fec;
            if (s->f8c < l->fc0 || s->f98 < l->fc4)
                return 3;
            return 1;
        }
        break;
    case 11:
        return 9;
    case 2:
        if (!(def->flags2 & 0x80))
            break;
        RECLAIM_CHECK(def, unit, pos, 0x800, 0xa);
        if (!target || !unit->motion)
            return 0xe;
        if ((def->flags2 & 0x1000) && enemy)
            return 4;
        if (enemy && g_seams.units->CanReclaim(((Unit*)unit), 0, (Unit*)target))
            return 0xb;
        if (friendly && g_seams.units->CanRepair(((Unit*)unit), 0, (Unit*)target) && target->buildLeft != 0.0f)
            return 6;
        if (friendly && g_seams.units->CanRepair(((Unit*)unit), 0, (Unit*)target))
            return 6;
        if ((def->flags1 & 0x800) && (target->def->flags1 & 0x200))
            return 0xd;
        if (g_seams.units->CanLoad(((Unit*)unit), 0, (Unit*)target))
            return ((UnitDefCursorFlagsView*)def)->f241b.b11 ? 8 : 0xc;
        if ((def->flags2 & 0x20) && friendly)
            return 5;
        return 0xe;
    case 1:
        if (g_game->interfaceType == 1) {
            if (Selectable(target))
                return 0xf;
            if (enemy)
                return 0x11;
            if (friendly)
                return 0x12;
            RECLAIM_CHECK(def, unit, pos, 0x800, 0x12);
            RECLAIM_CHECK(def, unit, pos, 0x400, 0x12);
            break;
        }
        if ((def->flags2 & 0x10) && enemy)
            return GetOrderCursor(3, unit, target, pos);
        if ((def->flags2 & 0x400) && enemy)
            return GetOrderCursor(0xc, unit, target, pos);
        if (target && g_seams.units->CanRepair(((Unit*)unit), 0, (Unit*)target) && target->buildLeft != 0.0f)
            return 6;
        if (Selectable(target))
            return 0xf;
        RECLAIM_CHECK(def, unit, pos, 0x800, 0xa);
        RECLAIM_CHECK(def, unit, pos, 0x400, 0xb);
        return ((UnitDefCursorFlagsView*)def)->f245b.b7 ? 0xe : 0x13;
    }
    return 0x13;
}

