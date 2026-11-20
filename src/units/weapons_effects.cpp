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

static int NoticeBit(int kind)
{
    switch (kind) {
    case WEAPONS_EFFECT_SHOT_FIRED:
        return 0x400;
    case WEAPONS_EFFECT_COMMAND_SHOT_FIRED:
        return 0x800;
    }
    return 0x1000;
}

void UnitsOnWeaponsAttacked(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsAttacked))
        return;
    const WeaponsAttacked* e = static_cast<const WeaponsAttacked*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    g_seams.units->NotifyUnitRefs(unit, 16);
}

void UnitsOnWeaponsAttackedSpeech(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsAttacked))
        return;
    const WeaponsAttacked* e = static_cast<const WeaponsAttacked*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    if (!(g_seams.orders->GetOrderFlags(unit) & ORDER_FLAG_NO_UNDERATTACK_LINE) &&
        (unit->lastAttackerSlot != unit->playerIndex || unit->lastDamageType == 1))
        RecordUnitSpeech(unit, 2, SIM_SPEECH_KIND_TEXT, SIM_EVENT_SPEECH_IF_NOT_VISIBLE);
}

void UnitsOnWeaponsMarkDamaged(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsEventHeader))
        return;
    const WeaponsEventHeader* e = static_cast<const WeaponsEventHeader*>(event);
    Unit* unit = UnitOfOffset(e->unit);
    if (unit == nullptr)
        return;
    g_seams.units->MarkRecentlyDamaged(unit);
}

void UnitsOnWeaponsDamageRecord(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsDamageRecord))
        return;
    const WeaponsDamageRecord* e = static_cast<const WeaponsDamageRecord*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    unit->lastDamageType = (unsigned char)e->damageKind;
    Unit* attacker = UnitOfOffset(e->attacker);
    if (attacker) {
        unit->lastAttackerSlot = attacker->playerIndex;
        unit->attacker = attacker;
    }
}

void UnitsOnWeaponsHealth(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsHealth))
        return;
    const WeaponsHealth* e = static_cast<const WeaponsHealth*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    unit->health = (short)e->health;
    if (e->dying == 1)
        unit->flags |= 0x4000;
}

void UnitsOnWeaponsNotice(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsEventHeader))
        return;
    const WeaponsEventHeader* e = static_cast<const WeaponsEventHeader*>(event);
    Unit* unit = UnitOfOffset(e->unit);
    if (unit == nullptr)
        return;
    unit->netDirtyFlags |= (unsigned short)NoticeBit(e->kind);
}

void UnitsOnWeaponsBusy(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsBusy))
        return;
    const WeaponsBusy* e = static_cast<const WeaponsBusy*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    unit->workTime = e->until;
}
