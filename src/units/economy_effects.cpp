#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/economy.h"
#include "../../interfaces/ref.h"
#include "../scripts/cob_script.h"
#include "unit.h"
#include "economy_effects.h"

static Unit* UnitOfHandle(EconUnit handle)
{
    Ref<Unit> ref;
    ref.offset = handle;
    return ref.get();
}

void UnitsOnEconomyCloak(const void* event)
{
    const EconomyCloak* effect = static_cast<const EconomyCloak*>(event);
    g_seams.units->SetStateBits(UnitOfHandle(effect->h.unit), 4, effect->keep);
}

void UnitsOnEconomyExtractor(const void* event)
{
    const EconomyExtractor* effect = static_cast<const EconomyExtractor*>(event);
    Unit* unit = UnitOfHandle(effect->h.unit);
    if (unit->script != nullptr)
        g_seams.scripts->StartScriptWithArgs((CobScript*)unit->script.get(), "SetSpeed", nullptr, 0, 1, effect->metal, 0, 0, 0);
}
