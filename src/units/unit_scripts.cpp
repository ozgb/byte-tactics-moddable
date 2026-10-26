#include "win_types.h"
#include "../../interfaces/ref.h"
#include <string.h>

#include "../util/vec3.h"
#include "../util/angles.h"

#include "cob_script.h"

#include "../map/mission.h"
#include "../sound/sound.h"
#include "unit_scripts.h"


struct Unit_0043e490;
struct Feature_0043e490;

#include "../units/unit_def.h"
#include "sfx_occupy.h"

#pragma pack(push, 1)
#include "../game/game.h"

struct Object {
    char unknown_0[0x6a];
    Vec3 pos;
    char unknown_76[0x9a - 0x76];
    Ref<CobScript> script;
};

#pragma pack(pop)

#include "unit.h"

#include "../../link/globals.h"

// FUNCTION: 0x43db50
void Class_0043db50::UpdateSfxOccupy(Unit* unit)
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
            if (unit->def->field_170 + y < seaLevel)
                newState = 3;
        }
    } else {
        newState = 0;
    }
    if (*(int*)unit->unknown_10a != newState) {
        ((CobScript*)unit->script.get())->StartScriptWithArgs("setSFXoccupy", 0, 1, 1, newState, 0, 0, 0);
        *(int*)unit->unknown_10a = newState;
    }
}

void __stdcall RotateByAngles(Vec3* in, Vec3* out, short* angles);

struct Ptr_0043def0 {
    char unknown_0[0x10];
    int f10;
    int f14;
    int f18;
};

#pragma pack(push, 2)
struct Item_0043def0 {
    Ptr_0043def0* p;
    int x;
    int y;
    int z;
    short f10;
    short f12;
    short f14;
    char unknown_16[0x32 - 0x16];
    Item_0043def0* next;
};

struct Block_0043def0 {
    int count;
    char unknown_4[0x22 - 4];
    Item_0043def0 items[1];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Object_0043def0 {
    char unknown_0[0x64];
    Angles16 angles;
    char unknown_6a[0x9e - 0x6a];
    Ref<Block_0043def0> recs;
};
#pragma pack(pop)

// FUNCTION: 0x43def0
Vec3 __stdcall GetPieceOffset(Object_0043def0* obj, int index)
{
    if (obj == 0 || obj->recs == 0) {
        Vec3 v;
        v.x = 0;
        v.y = 0;
        v.z = 0;
        return v;
    }
    Block_0043def0* block = obj->recs;
    Vec3 out;
    Vec3 result;
    if (index < 0 || index >= block->count) {
        out.x = 0;
        out.y = 0;
        out.z = 0;
    } else {
        Item_0043def0* item = &block->items[index];
        result.x = item->p->f10 + item->x;
        result.y = item->p->f14 + item->y;
        result.z = item->p->f18 + item->z;
        for (Item_0043def0* n = item->next; n != 0; n = n->next) {
            short angles[3];
            angles[0] = n->f14;
            angles[2] = n->f10;
            angles[1] = n->f12;
            if (n->next == 0) {
                angles[0] = angles[0] + obj->angles.bank;
                angles[2] = angles[2] + obj->angles.pitch;
                angles[1] = angles[1] + obj->angles.heading;
            }
            RotateByAngles(&result, &result, angles);
            result.x += n->p->f10 + n->x;
            result.y += n->p->f14 + n->y;
            result.z += n->p->f18 + n->z;
        }
        out.x = result.x;
        out.y = result.y;
        out.z = -result.z;
    }
    return out;
}

#include <io.h>

// FUNCTION: 0x43e060
Vec3 __stdcall GetPiecePosition(Object* obj, int param)
{
    return obj->pos + GetPieceOffset((Object_0043def0*)obj, param);
}

struct Geom_0043e0b0 {
    char pad0[4];
    int count;
};

#pragma pack(push, 1)
struct Piece_0043e0b0 {
    Geom_0043e0b0* geom;
    char pad4[0x22 - 4];
    Ref<Vec3> verts;
};

struct Obj_0043e0b0 {
    char pad0[0x6a];
    Vec3 pos;
    char pad76[0x9e - 0x76];
    Ref<char> table;
};
#pragma pack(pop)

// FUNCTION: 0x43e0b0
void __stdcall GetPieceCenter(Obj_0043e0b0* obj, Vec3* out, int value)
{
    int minx = 0, miny = 0, minz = 0;
    int maxx = 0, maxy = 0, maxz = 0;
    Piece_0043e0b0* p = (Piece_0043e0b0*)(obj->table + value * 0x36 + 0x22);
    Vec3* v = p->verts;
    int n = p->geom->count;
    while (n > 0) {
        int x = v->x;
        if (x < minx) minx = x;
        if (x > maxx) maxx = x;
        int y = v->y;
        if (y < miny) miny = y;
        if (y > maxy) maxy = y;
        int z = v->z;
        if (z < minz) minz = z;
        if (z > maxz) maxz = z;
        v++;
        n--;
    }
    out->x = (maxx + minx) / 2 + obj->pos.x;
    out->y = (maxy + miny) / 2 + obj->pos.y;
    out->z = (maxz + minz) / 2 + obj->pos.z;
}

struct Rec_0043e180 {
    short a;
    char pad[0x30];
    short b;
    short c;
};

#pragma pack(push, 2)
struct Obj_0043e180 {
    char pad0[0x64];
    Angles16 angles;
    char pad1[0x9e - 0x6a];
    Ref<Rec_0043e180> recs;
};
#pragma pack(pop)

static inline short SumY_0043e180(Obj_0043e180* obj, Rec_0043e180* q)
{
    return obj->angles.heading + q->c;
}

static inline short SumZ_0043e180(Obj_0043e180* obj, Rec_0043e180* q)
{
    return obj->angles.pitch + q->b;
}

// FUNCTION: 0x43e180
Angles16 __stdcall GetPieceAngles(Obj_0043e180* obj, int index)
{
    Rec_0043e180* r = obj->recs;
    Angles16 p;
    p.pitch = obj->angles.pitch;
    p.heading = obj->angles.heading;
    if (index >= 0) {
        p.pitch = SumZ_0043e180(obj, &r[index]);
        p.heading = SumY_0043e180(obj, &r[index]);
    }
    p.bank = obj->angles.bank + r[index + 1].a;
    return p;
}

#include "../../interfaces/seams.h"

// FUNCTION: 0x43e1e0
int __stdcall QueryWeaponPiece(Object* obj, unsigned char weapon)
{
    char* names[3] = { "QueryPrimary", "QuerySecondary", "QueryTertiary" };
    int piece = 0;
    g_seams.units->QueryScript(obj->script, names[weapon], &piece, 0, 0, 0);
    return piece;
}

// FUNCTION: 0x43e240
void __stdcall GetWeaponPiecePosition(Object* obj, Vec3* out, unsigned char weapon, int piece)
{
    if (piece < 0)
        piece = QueryWeaponPiece(obj, weapon);
    *out = obj->pos + g_seams.units->GetPieceOffset((Unit*)obj, piece);
}

// FUNCTION: 0x43e2e0
void __stdcall GetAimFromPosition(Object* obj, Vec3* out, unsigned char weapon)
{
    char* names[3] = { "AimFromPrimary", "AimFromSecondary", "AimFromTertiary" };
    int piece = -1;
    obj->script->QueryScript(names[weapon], &piece, 0, 0, 0);
    if (piece == -1) {
        char* qnames[3] = { "QueryPrimary", "QuerySecondary", "QueryTertiary" };
        int q = 0;
        obj->script->QueryScript(qnames[weapon], &q, 0, 0, 0);
        *out = obj->pos + GetPieceOffset((Object_0043def0*)obj, q);
    } else {
        *out = obj->pos + GetPieceOffset((Object_0043def0*)obj, piece);
    }
}

#pragma pack(push, 2)
struct Obj_0043e3c0 {
    char unknown_0[0x9a];
    Ref<CobScript> table;
};
#pragma pack(pop)

#include "../graphics/spot_state.h"
#include "../quirks/quirks.h"
#include <stddef.h>
#include "../state/sim_globals.h"

static_assert(offsetof(Unit, state) == 0x9e, "state");
static_assert(offsetof(SpotState, animDirty) == 0x8, "animDirty");
static_assert(offsetof(SpotState, pos) == 0x18, "pos");
static_assert(offsetof(SpotState, root) == 0x1e, "root");
static_assert(offsetof(Piece_459c70, modified) == 0x26, "modified");

#include "../graphics/model_render.h"

void __stdcall PoseUnitModel(Unit* unit)
{
    SpotState* state = unit->state;
    if (state == NULL)
        return;
    memcpy(&state->pos, &unit->angles, 6);
    state->animDirty = 1;
    state->root->modified = 0;
    UpdateObjectState((Unit_459200 *)unit);
}

// FUNCTION: 0x43e3c0
void __stdcall GetSweetSpot(Obj_0043e3c0* obj, int out)
{
    int value = 0;
    obj->table->QueryScript("SweetSpot", &value, 0, 0, 0);
    if (!g_quirks.renderPoseReads)
        PoseUnitModel((Unit*)obj);
    GetPieceCenter((Obj_0043e0b0*)obj, (Vec3*)out, value);
}

// FUNCTION: 0x43e400
void __stdcall GetNanoPiecePosition(Object* obj, Vec3* out)
{
    int piece = 0;
    obj->script->QueryScript("QueryNanoPiece", &piece, 0, 0, 0);
    *out = obj->pos + GetPieceOffset((Object_0043def0*)obj, piece);
}

// FUNCTION: 0x43e470
int __stdcall OrderModeTakesTarget(unsigned char type)
{
    if (type != 5 && type != 10 && type != 14)
        return 1;
    return 0;
}

#pragma pack(push, 1)
#include "../orders/node_0043e490.h"


struct Flags_0043e490 {
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

struct UnitDefFlags_0043e490 {
    char unknown_0[0x241];
    Flags_0043e490 f241b;
    Flags_0043e490 f245b;
};

struct Size_0043e490 {
    unsigned int width;
    unsigned int height;
};

#include "../network/player.h"

struct Stats_0043e490 {
    char unknown_0[0x8c];
    float f8c;
    char unknown_90[0x98 - 0x90];
    float f98;
};

struct Limits_0043e490 {
    char unknown_0[0xc0];
    float fc0;
    float fc4;
};

struct Unit_0043e490 {
    int motion;
    char unknown_4[0x10 - 0x4];
    Ref<Node_0043e490> f10;
    char unknown_14[0x48 - 0x14];
    Ref<Limits_0043e490> f48;
    char unknown_4c[0x6a - 0x4c];
    char f6a[0x86 - 0x6a];
    Ref<Unit_0043e490> carrier;
    char unknown_8a[0x92 - 0x8a];
    Ref<UnitDef> def;
    Ref<Player> player;
    char unknown_9a[0xec - 0x9a];
    Ref<Stats_0043e490> fec;
    char unknown_f0[0xfb - 0xf0];
    int postTransferHoldoff;
    unsigned char playerIndex;
    char unknown_100[0x104 - 0x100];
    float buildLeft;
    char unknown_108[0x110 - 0x108];
    unsigned int flags;
};

struct Pos_0043e490 {
    short xf, x;
    short yf, y;
    short zf, z;
};

#include "../map/cell.h"

struct Feature_0043e490 {
    char unknown_0[0xfe];
    unsigned char flags;
    char unknown_ff;
};
#pragma pack(pop)

static inline int Visible(Unit_0043e490* unit, Pos_0043e490* pos) {
    Player* p = unit->player;
    int y, x;
    x = pos->x >> 5;
    y = (pos->z - (pos->y >> 1)) >> 5;
    return (unsigned int)x < p->exploredWidth && (unsigned int)y < p->exploredHeight &&
           0 != ((1 << g_game->playerIndex) & g_game->visibilityMask[x + y * unit->player->exploredWidth]);
}

static inline Feature_0043e490* GetFeature(Cell* cell) {
    if (cell == 0)
        return 0;
    unsigned short id = cell->feature;
    if (id < 0xfffb) {
        if (id >= g_game->featureCount)
            return 0;
        return &((Feature_0043e490 *)g_game->features)[id];
    }
    if (id != 0xfffe)
        return 0;
    id = (cell - (cell->offsetY * g_game->width + cell->offsetX))->feature;
    if (id >= 0xfffb)
        return 0;
    return &((Feature_0043e490 *)g_game->features)[id];
}

#define RECLAIM_CHECK(def, unit, pos, mask, result)                  \
    do {                                                             \
        if (((def)->flags2 & (mask)) && Visible((unit), (pos))) {      \
            Feature_0043e490* f = GetFeature(g_seams.los_radar->GetMapCellAtPosition((Vec3*)(pos)));     \
            if (f && (f->flags & 0x80))                              \
                return (result);                                     \
        }                                                            \
    } while (0)

static inline int Selectable(Unit_0043e490* t) {
    return t && t->playerIndex == g_game->localPlayer && (t->flags & 0x20) && t->buildLeft == 0.0f && t->postTransferHoldoff == 0 &&
           (t->carrier == 0 || (t->carrier->flags & 0x40000000));
}

// FUNCTION: 0x43e490
int __stdcall GetOrderCursor(unsigned char mode, Unit_0043e490* unit, Unit_0043e490* target,
                           Pos_0043e490* pos) {
    UnitDef* def;
    Node_0043e490* node;
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
        if ((def->flags2 & 0x10) && ((Node_0043e490*)def->weapons[0].get())->f111b.b8)
            return 2;
        if (((UnitDefFlags_0043e490*)def)->f245b.b4) {
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
        return ((UnitDefFlags_0043e490*)def)->f245b.b6 ? 7 : 0x13;
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
        return ((UnitDefFlags_0043e490*)def)->f241b.b11 ? 8 : 0xc;
    case 5:
        return ((UnitDefFlags_0043e490*)def)->f245b.b8 ? 0xd : 0x13;
    case 14:
        if (def->ids == 0 || unit->motion == 0)
            break;
        return 0x10;
    case 4:
        if (((UnitDefFlags_0043e490*)def)->f245b.b14) {
            Limits_0043e490* l = unit->f48;
            Stats_0043e490* s = unit->fec;
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
            return ((UnitDefFlags_0043e490*)def)->f241b.b11 ? 8 : 0xc;
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
        return ((UnitDefFlags_0043e490*)def)->f245b.b7 ? 0xe : 0x13;
    }
    return 0x13;
}
