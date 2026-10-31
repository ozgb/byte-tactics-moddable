#include "win_types.h"
#include "../../interfaces/ref.h"
#include <stdio.h>
#include <math.h>

#pragma pack(push, 1)

#include "order_cursor_views.h"

#include "../units/unit_def.h"

#include "../network/player.h"

#include "../map/cell.h"
#include "mission_type.h"

union UnitOrderTypeFlags {
    unsigned int raw;
    struct {
        unsigned int bits_0 : 31;
        unsigned int flag_31 : 1;
    };
};

union UnitDefFlags1Bits {
    unsigned int raw;
    struct {
        unsigned int bits_0 : 11;
        unsigned int flag_11 : 1;
        unsigned int bits_12 : 16;
        unsigned int flag_28 : 1;
        unsigned int bits_29 : 3;
    };
};

union WeaponDefFlagsBits {
    unsigned int raw;
    struct {
        unsigned int bits_0 : 8;
        unsigned int flag_8 : 1;
        unsigned int bits_9 : 8;
        unsigned int flag_17 : 1;
        unsigned int bits_18 : 14;
    };
};

union UnitDefFlags2Bits {
    unsigned int raw;
    struct {
        unsigned int bits_0 : 4;
        unsigned int flag_4 : 1;
        unsigned int flag_5 : 1;
        unsigned int flag_6 : 1;
        unsigned int flag_7 : 1;
        unsigned int flag_8 : 1;
        unsigned int flag_9 : 1;
        unsigned int flag_10 : 1;
        unsigned int flag_11 : 1;
        unsigned int flag_12 : 1;
        unsigned int bits_13 : 1;
        unsigned int flag_14 : 1;
        unsigned int bits_15 : 17;
    };
};

#include "../game/game.h"

struct WeaponDefOrderTypeView {
    char unknown_0[0x111];
    union {
        unsigned int f111;
        WeaponDefFlagsBits f111bits;
    };
};

struct UnitDefOrderTypeView {
    char unknown_0[0x146];
    char unknown_146[0x156 - 0x146];
    int f156;
    char unknown_15a[0x170 - 0x15a];
    short f170;
    char unknown_172[0x1ee - 0x172];
    Ref<WeaponDefOrderTypeView> f1ee;
    char unknown_1f2[0x1fa - 0x1f2];
    unsigned int f1fa;
    char unknown_1fe[0x241 - 0x1fe];
    union {
        unsigned int f241;
        UnitDefFlags1Bits f241bits;
    };
    union {
        unsigned int f245;
        UnitDefFlags2Bits f245bits;
    };
};

struct UnitOrderTypeView {
    int motion;
    char unknown_4[0x10 - 0x4];
    Ref<WeaponDefOrderTypeView> f10;
    char unknown_14[0x2c - 0x14];
    Ref<WeaponDefOrderTypeView> f2c;
    char unknown_30[0x3b - 0x30];
    unsigned char f3b;
    char unknown_3c[0x70 - 0x3c];
    short f70;
    char unknown_72[0x86 - 0x72];
    Ref<UnitOrderTypeView> carrier;
    char unknown_8a[0x92 - 0x8a];
    Ref<UnitDefOrderTypeView> def;
    Ref<Player> player;
    char unknown_9a[0xfb - 0x9a];
    int postTransferHoldoff;
    unsigned char unknown_ff[1];
    char unknown_100[0x104 - 0x100];
    float buildLeft;
    short health;
    char unknown_10a[0x110 - 0x10a];
    union {
        unsigned int flags;
        UnitOrderTypeFlags f110bits;
    };
};

void RegisterUnitOrders();

struct FeatureMarkView {
    char unknown_0[0xfe];
    unsigned char ffe;
};
#pragma pack(pop)

#include "../../link/globals.h"

#include "../../interfaces/seams.h"
#include "../units/unit.h"
void __stdcall DrawUnit(void*, Unit*);

#include "order_dispatch.h"

int __stdcall GetOrderCursor(unsigned char mode, UnitCursorView* unit, UnitCursorView* target,
                           FixedPosView* pos);

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
        return &((struct FeatureCursorView *)g_game->features.get())[id];
    }
    if (id != 0xfffe)
        return 0;
    id = (cell - (cell->offsetY * g_game->width + cell->offsetX))->feature;
    if (id >= 0xfffb)
        return 0;
    return &((struct FeatureCursorView *)g_game->features.get())[id];
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
        if (def->c04) {
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
        return def->c06 ? 7 : 0x13;
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
        return def->flying ? 8 : 0xc;
    case 5:
        return def->c08 ? 0xd : 0x13;
    case 14:
        if (def->ids == 0 || unit->motion == 0)
            break;
        return 0x10;
    case 4:
        if (def->c14) {
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
            return def->flying ? 8 : 0xc;
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
        return def->c07 ? 0xe : 0x13;
    }
    return 0x13;
}

static inline MissionType Pick(UnitDefOrderTypeView* def, const char* vtol, const char* ground) {
    return MissionType(def->f241bits.flag_11 ? vtol : ground);
}

static inline int Visible(UnitOrderTypeView* unit, FixedPosView* pos) {
    Player* p = unit->player;
    int x = pos->x >> 5;
    int y = (pos->z - (pos->y >> 1)) >> 5;
    return (unsigned int)x < p->exploredWidth && (unsigned int)y < p->exploredHeight &&
           0 != ((1 << g_game->playerIndex) & g_game->visibilityMask[x + y * unit->player->exploredWidth]);
}

#define FEATURE_CHECK(def, unit, pos, mask, result)                                            \
    do {                                                                                       \
        if (((def)->f245 & (mask)) && (pos) && Visible((unit), (pos)) && Marked(Lookup(pos)))  \
            return (result);                                                                   \
    } while (0)

static inline int Marked(FeatureMarkView* t) { return t && (t->ffe & 0x80); }

static inline FeatureMarkView* Lookup(FixedPosView* pos) {
    Cell* cell = g_seams.los_radar->GetMapCellAtPosition((Vec3*)pos);
    if (!cell)
        return 0;
    unsigned short id = cell->feature;
    if (id < 0xfffb) {
        if (id >= g_game->featureCount)
            return 0;
        return (FeatureMarkView*)((char*)(struct FeatureCursorView *)g_game->features.get() + (id << 8));
    }
    if (id != 0xfffe)
        return 0;
    id = (cell - (cell->offsetY * g_game->width + cell->offsetX))->feature;
    if (id >= 0xfffb)
        return 0;
    return (FeatureMarkView*)((char*)(struct FeatureCursorView *)g_game->features.get() + (id << 8));
}

// FUNCTION: 0x43f0e0
MissionType __stdcall GetOrderType(unsigned char mode, Unit* unitArg, Unit* targetArg, Vec3* posArg) {
    UnitOrderTypeView* unit = (UnitOrderTypeView*)unitArg;
    UnitOrderTypeView* target = (UnitOrderTypeView*)targetArg;
    FixedPosView* pos = (FixedPosView*)posArg;
    UnitDefOrderTypeView* def;
    int friendly = 0;
    int enemy = 0;
    if (target) {
        if (!(target->flags & 0x10000000))
            goto none;
        if (unit->player->allied[target->player->index] != 0)
            friendly = 1;
        else
            enemy = 1;
    }
    def = unit->def;
    switch (mode) {
    case 3: {
        if (!(def->f245 & 0x10))
            break;
        UnitOrderTypeFlags flags;
        flags.raw = unit->flags;
        if (flags.flag_31) {
            WeaponDefOrderTypeView* node = unit->f10;
            if (!enemy) {
                if (node->f111bits.flag_17)
                    break;
                if (!(def->f241 & 0x800))
                    return MissionType("SUPPRESS");
                if (def->f1ee->f111bits.flag_8)
                    return MissionType("AIRSTRIKE");
                return MissionType("AIRTOGROUND");
            }
            if ((target->flags & 3) != 2) {
                if (node->f111 & 0x20000)
                    break;
            }
            if (target->def->f170 + target->f70 < g_game->seaLevel) {
                if (!(node->f111 & 0x10000)) {
                    if (!(unit->f3b & 2))
                        break;
                    if (!(unit->f2c->f111 & 0x10000))
                        break;
                }
            }
            if (target->def->f170 + target->f70 >= g_game->seaLevel) {
                if (def->f241 & 0x1000) {
                    if (node->f111 & 0x10000)
                        break;
                    if ((unit->f3b & 2) && (unit->f2c->f111 & 0x10000))
                        return MissionType();
                }
            }
            unsigned int f = def->f241;
            if (def->f241bits.flag_11) {
                unsigned int air = def->f1ee->f111 & 0x100;
                if (air && !(target->def->f241 & 0x800))
                    return MissionType("AIRSTRIKE");
                if (!air && (target->def->f241 & 0x800))
                    return MissionType("AIRTOAIR");
                unsigned int tv = target->def->f241 & 0x800;
                if (!tv && !(f & 0x8000000))
                    return MissionType("AIRTOGROUND");
                if (!tv && (f & 0x8000000))
                    return MissionType("AIRTOGROUNDHOVER");
                break;
            }
            if (unit->motion != 0)
                return MissionType("ATTACK_CHASE");
            if (flags.raw & 0x20000000)
                return MissionType("ATTACK_NOMOVE");
        }
        if (def->f241bits.flag_28)
            return MissionType("ATTACK_KAMIKAZE");
        break;
    }
    case 9:
        if (def->f245 & 0x40) {
            if (unit->motion == 0)
                return MissionType("QPATROL");
            if (def->f245bits.flag_9) {
                if (def->f241bits.flag_11)
                    return MissionType("VTOL_REPAIRPATROL");
                return MissionType("REPAIRPATROL");
            }
            if (def->f241bits.flag_11)
                return MissionType("VTOL_PATROL");
            return MissionType("PATROL");
        }
        break;
    case 8:
        if (!g_seams.units->CanRepair(((Unit*)unit), 0, (Unit*)target))
            break;
        if (target->buildLeft != 0.0f)
            return Pick(def, "VTOL_HELPBUILD", "HELPBUILD");
        return Pick(def, "VTOL_REPAIRUNIT", "REPAIRUNIT");
    case 7:
        if (!(def->f245 & 0x20) || !friendly)
            break;
        return Pick(def, "VTOL_FOLLOW", "FOLLOW_GROUND");
    case 12: {
        if (!(def->f245 & 0x400))
            break;
        FeatureMarkView* t = Lookup(pos);
        if (pos && (def->f245 & 0x800)) {
            if (Visible(unit, pos) && t && (t->ffe & 0x80))
                return MissionType("RESURRECT");
        }
        if (pos && Visible(unit, pos) && t && (t->ffe & 0x80))
            return Pick(def, "VTOL_RECLAIM", "RECLAIM");
        if (!target)
            break;
        return Pick(def, "VTOL_RECLAIMUNIT", "RECLAIMUNIT");
    }
    case 13:
        if ((def->f245 & 0x1000) && target && unit->player != target->player)
            return MissionType("CAPTURE");
        break;
    case 6:
        if (!target || !g_seams.units->CanLoad(((Unit*)unit), 0, (Unit*)target))
            break;
        return Pick(def, "VTOL_PICKUP", "GROUND_PICKUP");
    case 5:
        if ((def->f245 & 0x100) && (def->f241 & 0x800) && target && (target->def->f241 & 0x200))
            return MissionType("VTOL_LANDING");
        if (def->f245bits.flag_8)
            return Pick(def, "VTOL_UNLOAD", "GROUND_UNLOAD");
        break;
    case 14:
        if (def->f156 == 0 || unit->motion == 0)
            break;
        return Pick(def, "VTOL_MOBILEBUILD", "MOBILEBUILD");
    case 4:
        if (def->f245bits.flag_14)
            return MissionType("ATTACKSPECIAL");
        break;
    case 11:
        return MissionType("TELEPORT");
    case 10:
        return MissionType("STOP");
    case 2:
        if (!(def->f245 & 0x80))
            break;
        if (unit->motion == 0)
            return MissionType("QMOVE");
        if (!target)
            return Pick(def, "VTOL_MOVE", "MOVE_GROUND");
        if ((def->f245 & 0x1000) && enemy)
            return MissionType("CAPTURE");
        if ((def->f245 & 0x400) && enemy)
            return Pick(def, "VTOL_RECLAIMUNIT", "RECLAIMUNIT");
        if (friendly && g_seams.units->CanRepair(((Unit*)unit), 0, (Unit*)target) && target->buildLeft != 0.0f)
            return Pick(def, "VTOL_HELPBUILD", "HELPBUILD");
        if (friendly && g_seams.units->CanRepair(((Unit*)unit), 0, (Unit*)target) &&
            (unsigned int)target->health < target->def->f1fa)
            return Pick(def, "VTOL_REPAIRUNIT", "REPAIRUNIT");
        if ((def->f241 & 0x800) && friendly && (target->def->f241 & 0x200))
            return MissionType("VTOL_LANDING");
        if (g_seams.units->CanLoad(((Unit*)unit), 0, (Unit*)target))
            return Pick(def, "VTOL_PICKUP", "GROUND_PICKUP");
        if ((def->f245 & 0x20) && friendly)
            return Pick(def, "VTOL_FOLLOW", "FOLLOW_GROUND");
        return Pick(def, "VTOL_MOVE", "MOVE_GROUND");
    case 1: {
        if (g_game->interfaceType == 1) {
            if ((def->f245 & 0x10) && enemy)
                return GetOrderType(3, unitArg, targetArg, posArg);
            if ((def->f245 & 0x400) && enemy)
                return Pick(def, "VTOL_RECLAIMUNIT", "RECLAIMUNIT");
            if (friendly && g_seams.units->CanRepair(((Unit*)unit), 0, (Unit*)target) && target->buildLeft != 0.0f)
                return Pick(def, "VTOL_HELPBUILD", "HELPBUILD");
            if (friendly && g_seams.units->CanRepair(((Unit*)unit), 0, (Unit*)target))
                return Pick(def, "VTOL_REPAIRUNIT", "REPAIRUNIT");
            if ((def->f241 & 0x800) && friendly && (target->def->f241 & 0x200))
                return MissionType("VTOL_LANDING");
            if (target && g_seams.units->CanLoad(((Unit*)unit), 0, (Unit*)target))
                return Pick(def, "VTOL_PICKUP", "GROUND_PICKUP");
            if ((def->f245 & 0x20) && friendly)
                return Pick(def, "VTOL_FOLLOW", "FOLLOW_GROUND");
            FEATURE_CHECK(def, unit, pos, 0x800, MissionType("RESURRECT"));
            FEATURE_CHECK(def, unit, pos, 0x400, Pick(def, "VTOL_RECLAIM", "RECLAIM"));
            if (!(def->f245 & 0x80) || unit->motion == 0)
                break;
            return Pick(def, "VTOL_MOVE", "MOVE_GROUND");
        } else {
            if ((def->f245 & 0x10) && enemy)
                return GetOrderType(3, unitArg, targetArg, posArg);
            if ((def->f245 & 0x400) && enemy)
                return GetOrderType(0xc, unitArg, targetArg, posArg);
            if (target && g_seams.units->CanRepair(((Unit*)unit), 0, (Unit*)target) && target->buildLeft != 0.0f)
                return GetOrderType(8, unitArg, targetArg, posArg);
            if (target && target->unknown_ff[0] == g_game->localPlayer && (target->flags & 0x20) &&
                target->buildLeft == 0.0f && target->postTransferHoldoff == 0 &&
                (!target->carrier || (target->carrier->flags & 0x40000000)))
                break;
            FEATURE_CHECK(def, unit, pos, 0x800, MissionType("RESURRECT"));
            FEATURE_CHECK(def, unit, pos, 0x400, Pick(def, "VTOL_RECLAIM", "RECLAIM"));
            if (!(def->f245 & 0x80) || unit->motion == 0)
                break;
            return Pick(def, "VTOL_MOVE", "MOVE_GROUND");
        }
    }
    }
none:
    return MissionType();
}