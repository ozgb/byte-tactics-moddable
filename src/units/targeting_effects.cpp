#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/targeting.h"
#include "../../interfaces/ref.h"
#include "cob_script.h"
#include "unit.h"
#include "targeting_effects.h"

void UnitsOnTargetingCleared(const void* event)
{
    const TargetingEvent* effect = static_cast<const TargetingEvent*>(event);
    Ref<Unit> ref;
    ref.offset = effect->unit;
    Unit* unit = ref.get();
    g_seams.units->FindScript((CobScript*)unit->script.get(), "StartBuilding");
    g_seams.units->StartScriptWithArgs((CobScript*)unit->script.get(), "TargetCleared", 0, 0, 1, effect->weapon, 0, 0, 0);
}
