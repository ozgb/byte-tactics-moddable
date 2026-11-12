#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/economy.h"
#include "../../interfaces/ref.h"
#include "cob_script.h"
#include "unit.h"
#include "economy_effects.h"

#include "../../link/globals.h"

static Unit* UnitOfHandle(EconUnit handle)
{
    Ref<Unit> ref;
    ref.offset = handle;
    return ref.get();
}

void UnitsOnEconomyCloak(const void* event)
{
    const EconomyEvent* effect = static_cast<const EconomyEvent*>(event);
    g_seams.units->SetStateBits(UnitOfHandle(effect->unit), 4, effect->value);
}

void UnitsOnEconomyExtractor(const void* event)
{
    const EconomyEvent* effect = static_cast<const EconomyEvent*>(event);
    Unit* unit = UnitOfHandle(effect->unit);
    if (unit->script != nullptr)
        g_seams.units->StartScriptWithArgs((CobScript*)unit->script.get(), "SetSpeed", nullptr, 0, 1, effect->value, 0, 0, 0);
}
