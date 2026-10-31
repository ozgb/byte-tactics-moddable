#include "../state/block.h"
#include "unit_script.h"

UnitScript* NewUnitScript()
{
    return new (kSim) UnitScript;
}

// FUNCTION: 0x485e30 ??_GUnitScript@@UAEPAXI@Z
UnitScript* EmitUnitScriptDeletingDtor() { return new UnitScript; }
