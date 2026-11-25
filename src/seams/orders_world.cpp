#include <stddef.h>
#include <string.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/ai.h"
#include "../../interfaces/weapons.h"
#include "../../interfaces/movement.h"
#include "../../interfaces/orders.h"
#include "../../interfaces/ref.h"
#include "../util/engine_services.h"
#include "../game/game.h"
#include "../units/unit_def.h"
#include "../game/orders_events.h"
#include "../harness/harness.h"
#include "orders_world.h"

#include "../../link/globals.h"

void OrdersInbox(const void* event, unsigned int size)
{
    const OrdersEventHeader* e = static_cast<const OrdersEventHeader*>(event);
    HarnessOrdersEvent(e, size, g_seams.orders->Apply(e, size));
}

void OrdersOnCommand(const void* event, unsigned int size)
{
    if (size >= sizeof(OrdersAdd)) {
        const OrdersAdd* p = static_cast<const OrdersAdd*>(event);
        Ref<Unit> unit;
        unit.offset = p->h.unit;
        Ref<Unit> target;
        target.offset = p->target;
        HarnessUiOrder((unsigned char)p->type, p->queue, unit.get(), target.get(), p->hasPos ? p->pos : nullptr, p->params[0], p->params[1]);
    }
    OrdersInbox(event, size);
}

void OrdersOnGroupCommand(const void* event, unsigned int size)
{
    const OrdersEventHeader* e = static_cast<const OrdersEventHeader*>(event);
    HarnessOrdersEvent(e, size, g_seams.orders->ExpandGroupCommand(static_cast<const OrdersGroupCommand*>(event), size));
}

void OrdersOnAiAddOrder(const void* event, unsigned int size)
{
    if (size < sizeof(AiAddOrder))
        return;
    const AiAddOrder* effect = static_cast<const AiAddOrder*>(event);
    OrdersAdd e;
    memset(&e, 0, sizeof(e));
    e.h = OrdersHeaderOf(ORDERS_ADD);
    e.h.unit = effect->h.unit;
    e.type = effect->type;
    e.queue = effect->queue;
    e.target = effect->target;
    e.hasPos = effect->hasPos;
    memcpy(e.pos, effect->pos, sizeof(e.pos));
    memcpy(e.params, effect->params, sizeof(e.params));
    OrdersInbox(&e, sizeof(e));
}

void OrdersOnAiQueueBuild(const void* event, unsigned int size)
{
    if (size < sizeof(AiQueueBuild))
        return;
    const AiQueueBuild* effect = static_cast<const AiQueueBuild*>(event);
    OrdersQueueBuild e;
    memset(&e, 0, sizeof(e));
    e.h = OrdersHeaderOf(ORDERS_QUEUE_BUILD);
    e.h.unit = effect->h.unit;
    e.count = effect->count;
    strncpy(e.name, (char*)&g_game->defs[effect->type].unitname[0], sizeof(e.name) - 1);
    OrdersInbox(&e, sizeof(e));
}

void OrdersOnAiStop(const void* event, unsigned int size)
{
    if (size < sizeof(AiStop))
        return;
    const AiStop* effect = static_cast<const AiStop*>(event);
    OrdersStop e;
    e.h = OrdersHeaderOf(ORDERS_STOP);
    e.h.unit = effect->h.unit;
    e.all = effect->all;
    OrdersInbox(&e, sizeof(e));
}

void OrdersOnWeaponsAttacked(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsAttacked))
        return;
    const WeaponsAttacked* effect = static_cast<const WeaponsAttacked*>(event);
    OrdersAttacked e;
    e.h = OrdersHeaderOf(ORDERS_ATTACKED);
    e.h.unit = effect->h.unit;
    e.attacker = effect->attacker;
    OrdersInbox(&e, sizeof(e));
}

void OrdersOnWeaponsParalyze(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsParalyze))
        return;
    const WeaponsParalyze* effect = static_cast<const WeaponsParalyze*>(event);
    OrdersParalyze e;
    e.h = OrdersHeaderOf(ORDERS_PARALYZE);
    e.h.unit = effect->h.unit;
    e.ticks = effect->ticks;
    OrdersInbox(&e, sizeof(e));
}

void OrdersOnMovementOrderFlags(const void* event, unsigned int size)
{
    if (size < sizeof(MovementOrderFlags))
        return;
    const MovementOrderFlags* effect = static_cast<const MovementOrderFlags*>(event);
    OrdersStatusFlags e;
    e.h = OrdersHeaderOf(ORDERS_STATUS_FLAGS);
    e.h.unit = effect->h.unit;
    e.order = effect->order;
    e.flags = effect->flags;
    OrdersInbox(&e, sizeof(e));
}
