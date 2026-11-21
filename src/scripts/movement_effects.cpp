#include <stddef.h>

#pragma pack(push, 1)
#include "../units/unit.h"
#pragma pack(pop)

#include "../../interfaces/movement.h"
#include "../../interfaces/ref.h"
#include "cob_script.h"
#include "unit_script.h"
#include "movement_effects.h"

static Unit* UnitOfOffset(unsigned int offset)
{
    Ref<Unit> ref;
    ref.offset = offset;
    return ref.get();
}

static const char* ScriptName(int script)
{
    switch (script) {
    case MOVEMENT_SCRIPT_START_MOVING:
        return "StartMoving";
    case MOVEMENT_SCRIPT_STOP_MOVING:
        return "StopMoving";
    case MOVEMENT_SCRIPT_MOVE_RATE_1:
        return "MoveRate1";
    case MOVEMENT_SCRIPT_MOVE_RATE_2:
        return "MoveRate2";
    case MOVEMENT_SCRIPT_MOVE_RATE_3:
        return "MoveRate3";
    }
    return nullptr;
}

void ScriptsOnMovementStartScript(const void* event, unsigned int size)
{
    if (size < sizeof(MovementStartScript))
        return;
    const MovementStartScript* e = static_cast<const MovementStartScript*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    const char* name = ScriptName(e->script);
    if (name == nullptr)
        return;
    CobScript* script = unit->script.get();
    script->StartScript(name, nullptr, 1);
}
