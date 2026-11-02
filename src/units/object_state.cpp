#include "win_types.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"
#include "../state/block.h"
#include <stdlib.h>
#include <string.h>

#include "../../link/globals.h"
#include "../util/vec3.h"
#include "../graphics/gaf_frame.h"
#include "../graphics/model_types.h"
#include "unit_def.h"
#include "../network/player_info.h"
#include "../graphics/memory_cache.h"
#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)
#include "../util/math.h"
#include "object_state.h"

// FUNCTION: 0x45a8d0
ModelState* __stdcall CreateObjectState(Object3do* obj)
{
    int count = 1;
    if (obj->child != 0) {
        count = CountObjects(obj->child);
        count++;
    }
    if (obj->sibling != 0) {
        count += CountObjects(obj->sibling);
    }
    int size = count * 0x36 + 0x22;
    ModelState* state = (ModelState*)SimAllocTagged("Object State", size);
    memset(state, 0, size);
    state->root = AddStateEntries(state, obj, 0);
    state->animDirty = 1;
    return state;
}

struct ModelBuildList {
    char unknown_0[8];
    int count;
    char unknown_c[0x20 - 0xc];
    const char** names;
};

// FUNCTION: 0x45a950
ModelState* __stdcall CreatePlayerObjectState(Object3do* obj, ModelBuildList* list, Unit* owner)
{
    int count = 1;
    if (obj->child != 0) {
        count = CountObjects(obj->child);
        count++;
    }
    if (obj->sibling != 0) {
        count += CountObjects(obj->sibling);
    }
    int size = count * 0x36 + 0x22;
    ModelState* state = (ModelState*)SimAllocTagged("Object State", size);
    memset(state, 0, size);
    state->root = AddStateEntries(state, obj, 0);
    state->animDirty = 1;
    state->owner = owner;
    for (int i = 0; i < list->count; i++) {
        for (int j = i; j < state->count; j++) {
            if (_strcmpi(list->names[i], state->pieces[j].object->name) == 0) {
                if (i != j) {
                    ModelPiece temp = state->pieces[i];
                    state->pieces[i] = state->pieces[j];
                    state->pieces[j] = temp;
                }
                break;
            }
        }
    }
    state->root = LinkStateEntries(state, obj, 0);
    return state;
}

// FUNCTION: 0x45aaa0
void __stdcall FreeObjectState(ModelState* state)
{
    for (int i = 0; i < state->count; i++) {
        GameFreeThunk(state->pieces[i].points);
    }
    ReleaseUnitPictures(state);
    GameFreeThunk(state);
}

// FUNCTION: 0x45b030
int __fastcall RestorePieceVertices(ModelPiece* piece, int force)
{
    int result = force;
    if (piece->modified == 0 || force) {
        memcpy(piece->points, piece->object->vertices, piece->object->vertexCount * 12);
        piece->offset.x = 0;
        piece->offset.y = 0;
        piece->offset.z = 0;
        piece->modified = 0;
        result = 1;
    }
    if (piece->child)
        result = RestorePieceVertices(piece->child, result);
    if (piece->sibling)
        return RestorePieceVertices(piece->sibling, force);
    return result;
}

static inline int FarFrom(ModelState* state, const Vector3s* pos)
{
    return abs((short)(state->pos.z - pos->z)) >= 8
        || abs((short)(state->pos.y - pos->y)) >= 8
        || abs((short)(state->pos.x - pos->x)) >= 8;
}

// FUNCTION: 0x45ab10
void __stdcall UpdateObjectState(UnitDrawView* unit)
{
    ModelState* state = unit->state;
    if (FarFrom(state, &unit->angles)) {
        state->pos = unit->angles;
        state->animDirty = 1;
        state->root->modified = 0;
        if (state->root->flags.bits.colored) {
            state->cacheDrawCount = 0;
        }
    }
    if (unit->state->animDirty != 0) {
        RestorePieceVertices(unit->state->root, 0);
        PoseModel(unit->state, unit->state->root, 0);
        unit->state->animDirty = 0;
    }
}

#pragma auto_inline(off)

// FUNCTION: 0x45ae80
int __stdcall CountObjects(Object3do* edi)
{
    int esi = 1;
    int eax;

    eax = (int)edi->child;
    if (eax != 0) {
        esi = CountObjects((Object3do*)eax);
        esi++;
    }

    eax = (int)edi->sibling;
    if (eax != 0) {
        eax = CountObjects((Object3do*)eax);
        esi = esi + eax;
    }

    return esi;
}

#pragma auto_inline(on)

// FUNCTION: 0x45aec0
ModelPiece* __stdcall AddStateEntries(ModelState* state, Object3do* obj,
                                       ModelPiece* parent)
{
    ModelPiece* e = &state->pieces[state->count];
    e->flags.word |= 2;
    e->object = obj;
    e->modified = 0;
    e->points = (Vec3*)SimAllocTagged("Point List", obj->vertexCount * 12);
    memcpy(e->points, obj->vertices, obj->vertexCount * 12);
    if (obj->vertexCount >= 3) {
        e->flags.word |= 1;
    } else {
        e->flags.word &= 0xfffe;
    }
    e->flags.word |= 4;
    state->count++;
    if (obj->child != 0) {
        e->child = AddStateEntries(state, obj->child, e);
    } else {
        e->child = 0;
    }
    if (obj->sibling != 0) {
        e->sibling = AddStateEntries(state, obj->sibling, parent);
    } else {
        e->sibling = 0;
    }
    e->parent = parent;
    return e;
}

// FUNCTION: 0x45af90
ModelPiece* __stdcall LinkStateEntries(ModelState* state, Object3do* obj, ModelPiece* parent)
{
    int i = state->count - 1;
    if (i >= 0) {
        while (1) {
            if (state->pieces[i].object == obj)
                break;
            i--;
            if (i < 0)
                return 0;
        }
        ModelPiece* e = &state->pieces[i];
        if (e->object->sibling != 0) {
            e->sibling = LinkStateEntries(state, e->object->sibling, parent);
        } else {
            e->sibling = 0;
        }
        if (e->object->child != 0) {
            e->child = LinkStateEntries(state, e->object->child, e);
        } else {
            e->child = 0;
        }
        e->parent = parent;
        return e;
    }
    return 0;
}

// FUNCTION: 0x45b150
void __fastcall TransformPieces(ModelState* owner, ModelPiece* piece,
                             Vector3s* angles, Vec3* delta, int deep)
{
    for (;;) {
        if (piece->modified == 0) {
            Vec3* offset = &piece->offset;
            RotateByAngles(offset, offset, (short*)angles);
            int count = piece->object->vertexCount;
            for (int i = count - 1; i >= 0; i--)
                RotateByAngles(&piece->points[i], &piece->points[i], (short*)angles);
            offset->x += delta->x;
            offset->y += delta->y;
            offset->z += delta->z;
            count = piece->object->vertexCount;
            for (int j = count - 1; j >= 0; j--) {
                Vec3* p = &piece->points[j];
                p->x += delta->x;
                p->y += delta->y;
                p->z += delta->z;
            }
        }
        if (piece->child) {
            TransformPieces(owner, piece->child, angles, delta, 1);
        }
        if (deep == 0)
            break;
        piece = piece->sibling;
        if (piece == 0)
            break;
        deep = 1;
    }
}

// FUNCTION: 0x45b0a0
void __fastcall PoseModel(ModelState* model, ModelPiece* piece, int force)
{
    if (piece->child)
        PoseModel(model, piece->child, 1);

    Vector3s pos;
    pos.x = piece->short_14;
    pos.z = piece->short_10;
    pos.y = piece->short_12;
    if (!force) {
        pos.x += model->pos.x;
        pos.z += model->pos.z;
        pos.y += model->pos.y;
    }

    Object3do* object = piece->object;
    Vec3 box;
    box.x = piece->rot_x + object->box_x;
    box.y = piece->rot_y + object->box_y;
    box.z = piece->rot_z + object->box_z;

    TransformPieces(model, piece, &pos, &box, 0);

    if (force && piece->sibling)
        PoseModel(model, piece->sibling, 1);
}

