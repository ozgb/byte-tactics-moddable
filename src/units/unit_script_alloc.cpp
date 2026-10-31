#include "../state/block.h"
#include "unit.h"
#include "unit_script.h"

void NewUnitScript(Unit* self)
{
    self->script = new (kSim) UnitScript;
}

// FUNCTION: 0x485e30 ??_GUnitScript@@UAEPAXI@Z
UnitScript* EmitUnitScriptDeletingDtor() { return new UnitScript; }
