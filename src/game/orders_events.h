#ifndef GAME_ORDERS_EVENTS_H
#define GAME_ORDERS_EVENTS_H

#include <string.h>

#include "../../interfaces/orders.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"

struct Unit;

inline OrdersEventHeader OrdersHeaderOf(int kind)
{
    OrdersEventHeader header;
    header.kind = kind;
    header.unit = 0;
    return header;
}

template <class U>
inline OrdersEventHeader OrdersHeaderOf(int kind, U* unit)
{
    OrdersEventHeader header = OrdersHeaderOf(kind);
    header.unit = Ref<U>(unit).offset;
    return header;
}

inline unsigned int OrdersEventSize(int kind)
{
    switch (kind) {
    case ORDERS_ADD:
        return sizeof(OrdersAdd);
    case ORDERS_QUEUE_BUILD:
        return sizeof(OrdersQueueBuild);
    case ORDERS_STOP:
        return sizeof(OrdersStop);
    case ORDERS_STATUS_FLAGS:
        return sizeof(OrdersStatusFlags);
    case ORDERS_ATTACKED:
        return sizeof(OrdersAttacked);
    case ORDERS_PARALYZE:
        return sizeof(OrdersParalyze);
    case ORDERS_EFFECT_SET_STATE_BITS:
        return sizeof(OrdersSetStateBits);
    case ORDERS_EFFECT_SET_POSITION:
        return sizeof(OrdersSetPosition);
    case ORDERS_EFFECT_ATTACH_TO_PIECE:
        return sizeof(OrdersAttachToPiece);
    case ORDERS_EFFECT_FINISH_CONSTRUCTION:
        return sizeof(OrdersFinishConstruction);
    case ORDERS_EFFECT_BUILD_DECAY:
        return sizeof(OrdersBuildDecay);
    case ORDERS_EFFECT_REPAIR:
        return sizeof(OrdersRepair);
    case ORDERS_EFFECT_UNLINK:
        return sizeof(OrdersUnlink);
    case ORDERS_EFFECT_START_SCRIPT:
        return sizeof(OrdersStartScript);
    }
    return sizeof(OrdersEventHeader);
}

inline void OrdersPublishEffect(const void* effect, unsigned int size)
{
    HarnessOrdersEvent(static_cast<const OrdersEventHeader*>(effect), size, 1);
    SchedPublish(SCHED_ORDERS, effect, size);
}

template <class V>
inline void OrdersPublishAdd(int outbox, Unit* unit, int type, int queue, Unit* target, const V* pos, int param0, int param1)
{
    OrdersAdd event;
    event.h = OrdersHeaderOf(ORDERS_ADD, unit);
    event.type = type;
    event.queue = queue;
    event.target = Ref<Unit>(target).offset;
    event.hasPos = pos != 0;
    event.pos[0] = pos ? pos->x : 0;
    event.pos[1] = pos ? pos->y : 0;
    event.pos[2] = pos ? pos->z : 0;
    event.params[0] = param0;
    event.params[1] = param1;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void OrdersPublishQueueBuild(int outbox, Unit* unit, const char* name, int count)
{
    OrdersQueueBuild event;
    event.h = OrdersHeaderOf(ORDERS_QUEUE_BUILD, unit);
    event.count = count;
    memset(event.name, 0, sizeof(event.name));
    strncpy(event.name, name, sizeof(event.name) - 1);
    SchedPublish(outbox, &event, sizeof(event));
}

inline void OrdersPublishStop(int outbox, Unit* unit, int all)
{
    OrdersStop event;
    event.h = OrdersHeaderOf(ORDERS_STOP, unit);
    event.all = all;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void OrdersPublishBeCarried(int outbox, Unit* unit)
{
    OrdersEventHeader event = OrdersHeaderOf(ORDERS_BE_CARRIED, unit);
    SchedPublish(outbox, &event, sizeof(event));
}

template <class O>
inline void OrdersPublishStatusFlags(int outbox, Unit* unit, O* order, unsigned int flags)
{
    OrdersStatusFlags event;
    event.h = OrdersHeaderOf(ORDERS_STATUS_FLAGS, unit);
    event.order = Ref<O>(order).offset;
    event.flags = flags;
    SchedPublish(outbox, &event, sizeof(event));
}

#endif
