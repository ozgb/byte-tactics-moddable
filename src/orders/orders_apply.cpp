#include <stddef.h>
#include <string.h>

#pragma pack(push, 1)
#include "../units/unit_type_set.h"
#include "../weapons/weapon_def.h"
#include "../weapons/unit_weapon_slot.h"
#include "order.h"
#include "order_base.h"
#include "../units/unit_def.h"
#include "../network/player.h"
#include "../units/unit.h"
#include "../game/game.h"
#include "order_type_entry.h"
#pragma pack(pop)

#include "../util/vec3.h"
#include "../state/block.h"
#include "../../interfaces/seams.h"
#include "../../interfaces/weapons.h"
#include "../../interfaces/orders.h"
#include "../../interfaces/ref.h"
#include "../util/engine_services.h"
#include "../game/orders_events.h"
#include "../game/orders_event_size.h"
#include "../game/targeting_events.h"
#include "mission_type.h"
#include "order_list.h"
#include "order_queue.h"
#include "orders_seam.h"
#include "order_dispatch.h"
#include "order_cursor.h"
#include "../state/sim_globals.h"

#include "../../link/globals.h"

Order* __stdcall FindOrderByType(Unit* obj, unsigned char kind);

static const float kZeroFloat = 0.0f;

static Unit* UnitOfOffset(unsigned int offset)
{
    Ref<Unit> ref;
    ref.offset = offset;
    return ref.get();
}

static int KnownKind(int kind)
{
    return kind >= ORDERS_ADD && kind <= ORDERS_CANCEL;
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
        IssueOrCancelOrder((unsigned char)p->type, p->queue, unit, (int)UnitOfOffset(p->target), p->hasPos ? pos : nullptr, p->params[0], p->params[1]);
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
    case ORDERS_CANCEL: {
        Order* order = FindOrderByType(unit, (unsigned char)reinterpret_cast<const OrdersCancel*>(e)->type);
        if (order == nullptr)
            return 0;
        DeleteOrder(unit, order);
        return 1;
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

static int InGroup(const OrdersGroupCommand* c, int i)
{
    return (c->units[i >> 5] >> (i & 31)) & 1;
}

static int GroupSize(Unit* first)
{
    int n = (int)(g_game->unitsEnd.get() - first) + 1;
    return n < ORDERS_GROUP_UNITS ? n : ORDERS_GROUP_UNITS;
}

static int ExpandOrder(const OrdersGroupCommand* c, Unit* first)
{
    unsigned char kind = (unsigned char)c->type;
    int takesTarget;
    if (c->mode)
        takesTarget = OrderModeTakesTarget((unsigned char)c->mode);
    else
        takesTarget = (g_missionOrderTableVec[kind].flags >> 9) & 1;
    Unit* except = takesTarget ? UnitOfOffset(c->target) : nullptr;
    int size = GroupSize(first);
    int count = 0;
    int sum_x = 0;
    int sum_z = 0;
    int i;
    for (i = 0; i < size; i++) {
        Unit* u = first + i;
        if (InGroup(c, i) && u != except) {
            count++;
            sum_x += (short)(u->pos.x >> 16);
            sum_z += (short)(u->pos.z >> 16);
        }
    }
    if (!count)
        return 0;
    int avg[3];
    avg[0] = (sum_x / count) * 65536;
    avg[2] = (sum_z / count) * 65536;
    int range = count * 3000;
    Vec3 clicked;
    clicked.x = c->pos[0];
    clicked.y = c->pos[1];
    clicked.z = c->pos[2];
    const Vec3* pos = c->hasPos ? &clicked : nullptr;
    unsigned char fire = MissionType("Standing_FireOrder").index;
    unsigned char move = MissionType("Standing_MoveOrder").index;
    int published = 0;
    for (i = 0; i < size; i++) {
        Unit* u = first + i;
        if (!InGroup(c, i) || u == except)
            continue;
        if (c->mode)
            kind = GetOrderType((unsigned char)c->mode, u, except, &clicked).index;
        if (!kind)
            continue;
        if (kind == fire && !(u->def->flags2Byte & 2))
            continue;
        if (kind == move && !(u->def->flags2Byte & 1))
            continue;
        const Vec3* at = pos;
        Vec3 here;
        if (pos && (g_missionOrderTableVec[kind].flags & 2)) {
            int dz = u->pos.z;
            dz -= avg[2];
            int dx = u->pos.x;
            dx -= avg[0];
            if ((int)(((__int64)dx * dx) >> 32) + (int)(((__int64)dz * dz) >> 32) <= range) {
                here.x = pos->x + u->pos.x - avg[0];
                here.y = pos->y;
                here.z = pos->z + u->pos.z - avg[2];
                at = &here;
            }
        }
        OrdersPublishAdd(SCHED_ORDERS, ORDERS_COMMAND, u, kind, c->queue, except, at, c->params[0], c->params[1]);
        published++;
    }
    return published;
}

static int ExpandBuild(const OrdersGroupCommand* c, Unit* first)
{
    Vec3 site;
    site.x = c->pos[0];
    site.y = c->pos[1];
    site.z = c->pos[2];
    const Vec3* at = c->hasPos ? &site : nullptr;
    int size = GroupSize(first);
    int published = 0;
    for (int i = 0; i < size; i++) {
        Unit* u = first + i;
        if (!InGroup(c, i) || !(u->def->flags1 & 0x40))
            continue;
        if (!(u->def->flags1 & 0x800)) {
            OrdersPublishAdd(SCHED_ORDERS, ORDERS_COMMAND, u, MissionType("MOBILEBUILD").index, c->queue, (Unit*)nullptr, at, c->params[0], c->params[1]);
            published++;
        } else if (u->def->flying) {
            OrdersPublishAdd(SCHED_ORDERS, ORDERS_COMMAND, u, MissionType("VTOL_MOBILEBUILD").index, c->queue, (Unit*)nullptr, at, c->params[0], c->params[1]);
            published++;
        }
    }
    return published;
}

int __stdcall OrdersExpandGroupCommand(const OrdersGroupCommand* command, unsigned int size)
{
    if (size < sizeof(OrdersGroupCommand))
        return 0;
    Unit* first = UnitOfOffset(command->h.unit);
    if (first == nullptr)
        return 0;
    if (command->h.kind == ORDERS_GROUP_COMMAND)
        return ExpandOrder(command, first);
    if (command->h.kind == ORDERS_GROUP_BUILD)
        return ExpandBuild(command, first);
    return 0;
}
