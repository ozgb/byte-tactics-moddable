#include <stddef.h>
#include <string.h>

#pragma pack(push, 1)
#include "../units/unit_type_set.h"
#include "../weapons/weapon_def.h"
#include "../weapons/unit_weapon_slot.h"
#include "order.h"
#include "../units/unit_def.h"
#include "../network/player.h"
#include "../units/unit.h"
#pragma pack(pop)

#include "../util/vec3.h"
#include "../state/block.h"
#include "../../interfaces/seams.h"
#include "../../interfaces/weapons.h"
#include "../../interfaces/orders.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"
#include "../game/orders_events.h"
#include "../game/targeting_events.h"
#include "mission_type.h"
#include "order_list.h"
#include "order_queue.h"
#include "orders_seam.h"

#include "../../link/globals.h"

static const float kZeroFloat = 0.0f;

static Unit* UnitOfOffset(unsigned int offset)
{
    Ref<Unit> ref;
    ref.offset = offset;
    return ref.get();
}

static int KnownKind(int kind)
{
    return kind >= ORDERS_ADD && kind <= ORDERS_COMMAND;
}

static void Attacked(Unit* unit, unsigned int attackerOffset)
{
    Unit* attacker = UnitOfOffset(attackerOffset);
    if (attacker && !attacker->unitDefIndex) attacker=0;
    if (attacker && unit->player->active && (unit->player->type==1 || unit->player->type==2) &&
        (unit->def->flags1&0x10010000) && unit->buildLeft==kZeroFloat && !unit->player->allied[attacker->player->index]) {
        int ordered=0;
        if ((!unit->list || (unit->list->flags_42&ORDER_FLAG_IDLE)) &&
            !unit->def->categories->Contains(attacker->unitDefIndex) &&
            !unit->def->weaponCategories[0]->Contains(attacker->unitDefIndex) && g_seams.weapons->WeaponCanReachUnit(unit,attacker,0))
            ordered=IssueAttackOrder(unit,attacker,0);
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

static void Paralyze(Unit* unit, int ticks)
{
    MissionType kind = MissionType(g_paralyzeMissionName);
    Order* first = unit->list;
    if (first && first->kind == kind) {
        first->ticks += ticks;
        return;
    }
    AppendOrder(unit, new (kSim) Order(kind, nullptr, nullptr, ticks, 0, 0));
}

int __stdcall OrdersApply(const OrdersEventHeader* e, unsigned int size)
{
    if (size < sizeof(OrdersEventHeader))
        return 0;
    if (!KnownKind(e->kind))
        return 0;
    if (size < OrdersEventSize(e->kind))
        return 0;
    Unit* unit = UnitOfOffset(e->unit);
    if (e->kind == ORDERS_STATUS_FLAGS) {
        const OrdersStatusFlags* p = reinterpret_cast<const OrdersStatusFlags*>(e);
        Ref<OrderBase> order;
        order.offset = p->order;
        if (order.get() == nullptr)
            return 0;
        order.get()->OrStatusFlags(p->flags);
        return 1;
    }
    if (unit == nullptr)
        return 0;
    switch (e->kind) {
    case ORDERS_ADD: {
        const OrdersAdd* p = reinterpret_cast<const OrdersAdd*>(e);
        Vec3 pos;
        pos.x = p->pos[0];
        pos.y = p->pos[1];
        pos.z = p->pos[2];
        AddOrder(MissionType((unsigned char)p->type), p->queue, unit, UnitOfOffset(p->target), p->hasPos ? &pos : nullptr, p->params[0], p->params[1]);
        break;
    }
    case ORDERS_COMMAND: {
        const OrdersAdd* p = reinterpret_cast<const OrdersAdd*>(e);
        int pos[3];
        pos[0] = p->pos[0];
        pos[1] = p->pos[1];
        pos[2] = p->pos[2];
        IssueOrCancelOrder((unsigned char)p->type, p->queue, unit, (int)UnitOfOffset(p->target), p->hasPos ? pos : 0, p->params[0], p->params[1]);
        break;
    }
    case ORDERS_QUEUE_BUILD: {
        const OrdersQueueBuild* p = reinterpret_cast<const OrdersQueueBuild*>(e);
        char name[32];
        memcpy(name, p->name, sizeof(name));
        name[sizeof(name) - 1] = 0;
        QueueBuildOrder(name, unit, p->count);
        break;
    }
    case ORDERS_STOP:
        DeleteOrders(unit, reinterpret_cast<const OrdersStop*>(e)->all);
        break;
    case ORDERS_BE_CARRIED:
        AddBeCarriedOrder(unit);
        break;
    case ORDERS_ATTACKED:
        Attacked(unit, reinterpret_cast<const OrdersAttacked*>(e)->attacker);
        break;
    case ORDERS_PARALYZE:
        Paralyze(unit, reinterpret_cast<const OrdersParalyze*>(e)->ticks);
        break;
    }
    return 1;
}
