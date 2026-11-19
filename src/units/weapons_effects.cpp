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
    const WeaponsEvent* e = static_cast<const WeaponsEvent*>(event);
    Unit* unit = UnitOfOffset(e->unit);
    if (unit == nullptr)
        return;
    g_seams.units->NotifyUnitRefs(unit, 16);
}

void UnitsOnWeaponsAttackedSpeech(const void* event)
{
    const WeaponsEvent* e = static_cast<const WeaponsEvent*>(event);
    Unit* unit = UnitOfOffset(e->unit);
    if (unit == nullptr)
        return;
    if (!(g_seams.orders->GetOrderFlags(unit) & ORDER_FLAG_NO_UNDERATTACK_LINE) &&
        (unit->lastAttackerSlot != unit->playerIndex || unit->lastDamageType == 1))
        RecordUnitSpeech(unit, 2, SIM_SPEECH_KIND_TEXT, SIM_EVENT_SPEECH_IF_NOT_VISIBLE);
}

void UnitsOnWeaponsMarkDamaged(const void* event)
{
    const WeaponsEvent* e = static_cast<const WeaponsEvent*>(event);
    Unit* unit = UnitOfOffset(e->unit);
    if (unit == nullptr)
        return;
    g_seams.units->MarkRecentlyDamaged(unit);
}

void UnitsOnWeaponsDamageRecord(const void* event)
{
    const WeaponsEvent* e = static_cast<const WeaponsEvent*>(event);
    Unit* unit = UnitOfOffset(e->unit);
    if (unit == nullptr)
        return;
    unit->lastDamageType = (unsigned char)e->value;
    Unit* attacker = UnitOfOffset(e->target);
    if (attacker) {
        unit->lastAttackerSlot = attacker->playerIndex;
        unit->attacker = attacker;
    }
}

void UnitsOnWeaponsHealth(const void* event)
{
    const WeaponsEvent* e = static_cast<const WeaponsEvent*>(event);
    Unit* unit = UnitOfOffset(e->unit);
    if (unit == nullptr)
        return;
    unit->health = (short)e->value;
    if (e->extra == 1)
        unit->flags |= 0x4000;
}

void UnitsOnWeaponsNotice(const void* event)
{
    const WeaponsEvent* e = static_cast<const WeaponsEvent*>(event);
    Unit* unit = UnitOfOffset(e->unit);
    if (unit == nullptr)
        return;
    unit->netDirtyFlags |= (unsigned short)e->value;
}

void UnitsOnWeaponsBusy(const void* event)
{
    const WeaponsEvent* e = static_cast<const WeaponsEvent*>(event);
    Unit* unit = UnitOfOffset(e->unit);
    if (unit == nullptr)
        return;
    unit->workTime = e->value;
}
