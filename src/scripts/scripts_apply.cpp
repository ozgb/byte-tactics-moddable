#include <stddef.h>
#include <string.h>

#pragma pack(push, 1)
#include "../units/unit.h"
#pragma pack(pop)

#include "../../interfaces/seams.h"
#include "../../interfaces/scripts.h"
#include "../../interfaces/ref.h"
#pragma pack(push, 1)
#include "../graphics/model_types.h"
#include "../map/feature.h"
#include "../game/game.h"
#pragma pack(pop)
#include "../game/scripts_event_size.h"
#include "cob_script.h"
#include "cob_types.h"
#include "object_state.h"
#include "piece_queries.h"
#include "scripts_seam.h"

#include "../../link/globals.h"

#pragma pack(push, 1)
struct PieceFlagsView {
    char padBeforeFlags[0x28];
    unsigned short flags;
    char padAfterFlags[0x36 - 0x2a];
};
#pragma pack(pop)

static Unit* UnitOfOffset(unsigned int offset)
{
    Ref<Unit> ref;
    ref.offset = offset;
    return ref.get();
}

static int KnownKind(int kind)
{
    return kind >= SCRIPTS_START && kind <= SCRIPTS_INIT_STATE;
}

static int ScriptsApplyStart(Unit* unit, const ScriptsStart* p)
{
    CobScript* script = (CobScript*)unit->script.get();
    if (script == nullptr)
        return 0;
    char name[32];
    memset(name, 0, sizeof(name));
    strncpy(name, p->name, sizeof(name) - 1);
    Ref<Callback> callbackRef;
    callbackRef.offset = p->callback;
    Callback* callback = callbackRef.get();
    switch (p->form) {
    case SCRIPTS_START_BY_NAME:
        script->StartScript(name, callback, p->update);
        break;
    case SCRIPTS_START_BY_NAME_WITH_ARGS:
        script->StartScriptWithArgs(name, callback, p->update, p->count, p->args[0], p->args[1], p->args[2], p->args[3]);
        break;
    case SCRIPTS_START_BY_INDEX_WITH_ARGS:
        script->StartScriptWithArgsByIndex(p->index, callback, p->update, p->count, p->args[0], p->args[1], p->args[2], p->args[3]);
        break;
    default:
        return 0;
    }
    return 1;
}

static ModelState* ScriptsStateOfOffset(unsigned int offset)
{
    Ref<ModelState> ref;
    ref.offset = offset;
    return ref.get();
}

int __stdcall ScriptsApply(const ScriptsEventHeader* e, unsigned int size)
{
    if (size < sizeof(ScriptsEventHeader))
        return 0;
    if (!KnownKind(e->kind))
        return 0;
    if (size < ScriptsEventSize(e->kind))
        return 0;
    if (e->kind == SCRIPTS_FREE_STATE || e->kind == SCRIPTS_INIT_STATE) {
        const ScriptsState* p = reinterpret_cast<const ScriptsState*>(e);
        ModelState* state = ScriptsStateOfOffset(p->state);
        if (state == nullptr)
            return 0;
        if (e->kind == SCRIPTS_FREE_STATE) {
            FreeObjectState(state);
            return 1;
        }
        InitObjectState(state, (Object3do*)g_game->features[p->feature].object);
        return 1;
    }
    Unit* unit = UnitOfOffset(e->unit);
    if (unit == nullptr)
        return 0;
    if (e->kind == SCRIPTS_START)
        return ScriptsApplyStart(unit, reinterpret_cast<const ScriptsStart*>(e));
    const ScriptsDetachPiece* p = reinterpret_cast<const ScriptsDetachPiece*>(e);
    if (p->pose == 1)
        PoseUnitModel(unit);
    PieceFlagsView* rec = (PieceFlagsView*)((char*)unit->state.get() + 0x22 + p->piece * 0x36);
    rec->flags &= 0xfffe;
    return 1;
}
