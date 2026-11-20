#include <stddef.h>
#include <string.h>

#pragma pack(push, 1)
#include "../units/unit.h"
#pragma pack(pop)

#include "../../interfaces/weapons.h"
#include "../../interfaces/ref.h"
#include "../../link/globals.h"
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

static const char* ScriptName(int script)
{
    switch (script) {
    case WEAPONS_SCRIPT_FIRE_PRIMARY:
    case WEAPONS_SCRIPT_FIRE_SECONDARY:
    case WEAPONS_SCRIPT_FIRE_TERTIARY:
        return g_fireScriptNames[script - WEAPONS_SCRIPT_FIRE_PRIMARY];
    case WEAPONS_SCRIPT_AIM_PRIMARY:
    case WEAPONS_SCRIPT_AIM_SECONDARY:
    case WEAPONS_SCRIPT_AIM_TERTIARY:
        return g_aimScriptNames[script - WEAPONS_SCRIPT_AIM_PRIMARY];
    case WEAPONS_SCRIPT_ROCK_UNIT:
        return "RockUnit";
    case WEAPONS_SCRIPT_SET_MAX_RELOAD_TIME:
        return "SetMaxReloadTime";
    case WEAPONS_SCRIPT_HIT_BY_WEAPON:
        return g_hitByWeaponScriptName;
    case WEAPONS_SCRIPT_TAKE_DAMAGE:
        return g_takeDamageScriptName;
    }
    return "";
}

void ScriptsOnWeaponsStartScript(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsStartScript))
        return;
    const WeaponsStartScript* e = static_cast<const WeaponsStartScript*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    char name[32];
    memset(name, 0, sizeof(name));
    strncpy(name, ScriptName(e->script), sizeof(name) - 1);
    Ref<Callback> callbackRef;
    callbackRef.offset = e->callback;
    Callback* callback = callbackRef.get();
    CobScript* script = unit->script.get();
    if (e->withArgs == 1)
        script->StartScriptWithArgs(name, callback, e->update, e->count, e->args[0], e->args[1], e->args[2], e->args[3]);
    else
        script->StartScript(name, callback, e->update);
}

void ScriptsOnWeaponsDetachPiece(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsDetachPiece))
        return;
    const WeaponsDetachPiece* e = static_cast<const WeaponsDetachPiece*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    if (e->pose == 1)
        PoseUnitModel(unit);
    PieceFlagsView* rec = (PieceFlagsView*)((char*)unit->state.get() + 0x22 + e->piece * 0x36);
    rec->flags &= 0xfffe;
}
