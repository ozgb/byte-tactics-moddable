#include <stddef.h>

#pragma pack(push, 1)
#include "../units/unit_type_set.h"
#include "../weapons/weapon_def.h"
#include "../weapons/unit_weapon_slot.h"
#include "order.h"
#include "../units/unit_def.h"
#include "../network/player.h"
#include "../units/unit.h"
#pragma pack(pop)

#include "../../interfaces/seams.h"
#include "../../interfaces/weapons.h"
#include "../../interfaces/orders.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"
#include "../game/targeting_events.h"
#include "mission_type.h"
#include "weapons_effects.h"

#include "../../link/globals.h"

static const float kZeroFloat = 0.0f;

static Unit* UnitOfOffset(unsigned int offset)
{
    Ref<Unit> ref;
    ref.offset = offset;
    return ref.get();
}

void OrdersOnWeaponsAttacked(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsAttacked))
        return;
    const WeaponsAttacked* e = static_cast<const WeaponsAttacked*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    Unit* attacker = UnitOfOffset(e->attacker);
    if (attacker && !attacker->unitDefIndex) attacker=0;
    if (attacker && unit->player->active && (unit->player->type==1 || unit->player->type==2) &&
        (unit->def->flags1&0x10010000) && unit->buildLeft==kZeroFloat && !unit->player->allied[attacker->player->index]) {
        int ordered=0;
        if ((!unit->list || (unit->list->flags_42&ORDER_FLAG_IDLE)) &&
            !unit->def->categories->Contains(attacker->unitDefIndex) &&
            !unit->def->weaponCategories[0]->Contains(attacker->unitDefIndex) && g_seams.weapons->WeaponCanReachUnit(unit,attacker,0))
            ordered=g_seams.orders->IssueAttackOrder(unit,attacker,0);
        if (!ordered && (unit->flags&0x300000)) {
            for (unsigned char i=0;i<3;++i) {
                UnitWeaponSlot* weapon=&unit->weapons[i];
                if ((weapon->flags&2) && (weapon->flags&0x10) && g_seams.weapons->WeaponCanReachUnit(unit,attacker,i) &&
                    !((unsigned char)(weapon->weapon->flags.value>>26)&1)) {
                    Unit* target=ScheduleTargetingTargetUnit(unit,i);
                    if (!target || !g_seams.weapons->WeaponCanReachUnit(unit,target,i) || unit->def->weaponCategories[i]->Contains(target->unitDefIndex))
                        TargetingPublishSetUnit(SCHED_ORDERS, unit,attacker,i);
                }
            }
        }
    }
}

void OrdersOnWeaponsHit(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsHit))
        return;
    const WeaponsHit* e = static_cast<const WeaponsHit*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    unit->netDirtyFlags |= (e->enemyDamage > e->friendlyDamage * 2) ? 0x4000 : 0x2000;
}

void OrdersOnWeaponsParalyze(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsParalyze))
        return;
    const WeaponsParalyze* e = static_cast<const WeaponsParalyze*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    MissionType kind = g_seams.orders->MissionTypeFromName(g_paralyzeMissionName);
    Order* first = unit->list;
    if (first && first->kind == kind) {
        first->ticks += e->ticks;
        return;
    }
    g_seams.orders->AppendOrder(unit, g_seams.orders->NewOrder(kind, nullptr, nullptr, e->ticks, 0, 0));
}
