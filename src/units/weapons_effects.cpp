#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/weapons.h"
#include "../../interfaces/orders.h"
#include "../../interfaces/units.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"
#include "../state/sim_speech.h"
#include "unit.h"
#include "weapons_effects.h"

static Unit* UnitOfOffset(unsigned int offset)
{
    Ref<Unit> ref;
    ref.offset = offset;
    return ref.get();
}

void UnitsOnWeaponsAttacked(const void* event)
{
    const WeaponsEffect* e = static_cast<const WeaponsEffect*>(event);
    Unit* unit = UnitOfOffset(e->unit);
    if (unit == nullptr)
        return;
    g_seams.units->NotifyUnitRefs(unit, 16);
}

void UnitsOnWeaponsAttackedSpeech(const void* event)
{
    const WeaponsEffect* e = static_cast<const WeaponsEffect*>(event);
    Unit* unit = UnitOfOffset(e->unit);
    if (unit == nullptr)
        return;
    if (!(g_seams.orders->GetOrderFlags(unit) & ORDER_FLAG_NO_UNDERATTACK_LINE) &&
        (unit->lastAttackerSlot != unit->playerIndex || unit->lastDamageType == 1))
        RecordUnitSpeech(unit, 2, SIM_SPEECH_KIND_TEXT, SIM_EVENT_SPEECH_IF_NOT_VISIBLE);
}
