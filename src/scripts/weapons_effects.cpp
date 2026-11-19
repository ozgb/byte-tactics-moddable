#include <stddef.h>
#include <string.h>

#pragma pack(push, 1)
#include "../units/unit.h"
#pragma pack(pop)

#include "../../interfaces/weapons.h"
#include "../../interfaces/ref.h"
#include "cob_script.h"
#include "cob_types.h"
#include "unit_script.h"
#include "piece_queries.h"
#include "weapons_effects.h"

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

void ScriptsOnWeaponsStartScript(const void* event)
{
    const WeaponsEvent* e = static_cast<const WeaponsEvent*>(event);
    Unit* unit = UnitOfOffset(e->unit);
    if (unit == nullptr)
        return;
    char name[32];
    memcpy(name, e->name, sizeof(name));
    Ref<Callback> callbackRef;
    callbackRef.offset = e->callback;
    Callback* callback = callbackRef.get();
    CobScript* script = unit->script.get();
    if (e->extra == 1)
        script->StartScriptWithArgs(name, callback, e->args[4], e->value, e->args[0], e->args[1], e->args[2], e->args[3]);
    else
        script->StartScript(name, callback, e->args[4]);
}

void ScriptsOnWeaponsDetachPiece(const void* event)
{
    const WeaponsEvent* e = static_cast<const WeaponsEvent*>(event);
    Unit* unit = UnitOfOffset(e->unit);
    if (unit == nullptr)
        return;
    if (e->extra == 1)
        PoseUnitModel(unit);
    PieceFlagsView* rec = (PieceFlagsView*)((char*)unit->state.get() + 0x22 + e->value * 0x36);
    rec->flags &= 0xfffe;
}
