#include "../../interfaces/ref.h"
#include <string.h>

#include "../util/vec3.h"
#include "../util/angles.h"

#include "cob_script.h"

#include "../map/mission.h"

struct UnitCursorView;
struct FeatureCursorView;


#pragma pack(push, 1)
#include "../game/game.h"

#pragma pack(pop)

#include "object_view.h"
#include "../units/unit.h"


#include "piece_queries.h"

#include "../util/math.h"

struct PieceBase {
    char unknown_0[0x10];
    int f10;
    int f14;
    int f18;
};

#pragma pack(push, 2)
struct PieceRecord {
    PieceBase* p;
    int x;
    int y;
    int z;
    short f10;
    short f12;
    short f14;
    char unknown_16[0x32 - 0x16];
    Ref<PieceRecord> next;
};

struct PieceTable {
    int count;
    char unknown_4[0x22 - 4];
    PieceRecord items[1];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct PieceOffsetObjectView {
    char unknown_0[0x64];
    Angles16 angles;
    char unknown_6a[0x9e - 0x6a];
    Ref<PieceTable> recs;
};
#pragma pack(pop)

// FUNCTION: 0x43def0
Vec3 __stdcall GetPieceOffset(PieceOffsetObjectView* obj, int index)
{
    if (obj == 0 || obj->recs == 0) {
        Vec3 v;
        v.x = 0;
        v.y = 0;
        v.z = 0;
        return v;
    }
    PieceTable* block = obj->recs;
    Vec3 out;
    Vec3 result;
    if (index < 0 || index >= block->count) {
        out.x = 0;
        out.y = 0;
        out.z = 0;
    } else {
        PieceRecord* item = &block->items[index];
        result.x = item->p->f10 + item->x;
        result.y = item->p->f14 + item->y;
        result.z = item->p->f18 + item->z;
        for (PieceRecord* n = item->next; n != 0; n = n->next) {
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


// FUNCTION: 0x43e060
Vec3 __stdcall GetPiecePosition(Object* obj, int param)
{
    return obj->pos + GetPieceOffset((PieceOffsetObjectView*)obj, param);
}

struct PieceGeometry {
    char pad0[4];
    int count;
};

#pragma pack(push, 1)
struct PieceVertexView {
    PieceGeometry* geom;
    char pad4[0x22 - 4];
    Ref<Vec3> verts;
};

struct PieceCenterObjectView {
    char pad0[0x6a];
    Vec3 pos;
    char pad76[0x9a - 0x76];
    void* script;
    Ref<char> table;
};
#pragma pack(pop)

// FUNCTION: 0x43e0b0
void __stdcall GetPieceCenter(PieceCenterObjectView* obj, Vec3* out, int value)
{
    int minx = 0, miny = 0, minz = 0;
    int maxx = 0, maxy = 0, maxz = 0;
    PieceVertexView* p = (PieceVertexView*)(obj->table + value * 0x36 + 0x22);
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

struct PieceAngleRecord {
    short a;
    char pad[0x30];
    short b;
    short c;
};

#pragma pack(push, 2)
struct PieceAnglesObjectView {
    char pad0[0x64];
    Angles16 angles;
    char pad1[0x9e - 0x6a];
    Ref<PieceAngleRecord> recs;
};
#pragma pack(pop)

static inline short AddPieceHeading(PieceAnglesObjectView* obj, PieceAngleRecord* q)
{
    return obj->angles.heading + q->c;
}

static inline short AddPiecePitch(PieceAnglesObjectView* obj, PieceAngleRecord* q)
{
    return obj->angles.pitch + q->b;
}


// FUNCTION: 0x43e180
Angles16 __stdcall GetPieceAngles(PieceAnglesObjectView* obj, int index)
{
    PieceAngleRecord* r = obj->recs;
    Angles16 p;
    p.pitch = obj->angles.pitch;
    p.heading = obj->angles.heading;
    if (index >= 0) {
        p.pitch = AddPiecePitch(obj, &r[index]);
        p.heading = AddPieceHeading(obj, &r[index]);
    }
    p.bank = obj->angles.bank + r[index + 1].a;
    return p;
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
        *out = obj->pos + GetPieceOffset((PieceOffsetObjectView*)obj, q);
    } else {
        *out = obj->pos + GetPieceOffset((PieceOffsetObjectView*)obj, piece);
    }
}


#pragma pack(push, 2)
struct SweetSpotObjectView {
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
static_assert(offsetof(ModelPiece, modified) == 0x26, "modified");

#include "object_state.h"

void __stdcall PoseUnitModel(Unit* unit)
{
    SpotState* state = unit->state;
    if (state == NULL)
        return;
    memcpy(&state->pos, &unit->angles, 6);
    state->animDirty = 1;
    state->root->modified = 0;
    UpdateObjectState((UnitDrawView *)unit);
}

// FUNCTION: 0x43e3c0
void __stdcall GetSweetSpot(SweetSpotObjectView* obj, int out)
{
    int value = 0;
    obj->table->QueryScript("SweetSpot", &value, 0, 0, 0);
    if (!g_quirks.renderPoseReads)
        PoseUnitModel((Unit*)obj);
    GetPieceCenter((PieceCenterObjectView*)obj, (Vec3*)out, value);
}

// FUNCTION: 0x43e400
void __stdcall GetNanoPiecePosition(Object* obj, Vec3* out)
{
    int piece = 0;
    obj->script->QueryScript("QueryNanoPiece", &piece, 0, 0, 0);
    *out = obj->pos + GetPieceOffset((PieceOffsetObjectView*)obj, piece);
}

