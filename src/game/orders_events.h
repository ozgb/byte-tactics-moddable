#ifndef GAME_ORDERS_EVENTS_H
#define GAME_ORDERS_EVENTS_H

#include <string.h>

#include "../../interfaces/orders.h"
#include "orders_event_size.h"
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

inline void OrdersPublishEffect(const void* effect, unsigned int size)
{
    HarnessOrdersEvent(static_cast<const OrdersEventHeader*>(effect), size, 1);
    SchedPublish(SCHED_ORDERS, effect, size);
}

template <class V>
inline void OrdersPublishAdd(int outbox, int kind, Unit* unit, int type, int queue, Unit* target, const V* pos, int param0, int param1)
{
    OrdersAdd event;
    event.h = OrdersHeaderOf(kind, unit);
    event.type = type;
    event.queue = queue;
    event.target = Ref<Unit>(target).offset;
    event.hasPos = pos != nullptr;
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

inline void OrdersPublishSetStateBits(Unit* unit, int mask, int set)
{
    OrdersSetStateBits effect;
    effect.h = OrdersHeaderOf(ORDERS_EFFECT_SET_STATE_BITS, unit);
    effect.mask = mask;
    effect.set = set;
    OrdersPublishEffect(&effect, sizeof(effect));
}

inline void OrdersPublishAttachToPiece(Unit* unit, Unit* target, int piece, int mode)
{
    OrdersAttachToPiece effect;
    effect.h = OrdersHeaderOf(ORDERS_EFFECT_ATTACH_TO_PIECE, unit);
    effect.target = Ref<Unit>(target).offset;
    effect.piece = piece;
    effect.mode = mode;
    OrdersPublishEffect(&effect, sizeof(effect));
}

inline void OrdersPublishFinishConstruction(Unit* builder, Unit* unit)
{
    OrdersFinishConstruction effect;
    effect.h = OrdersHeaderOf(ORDERS_EFFECT_FINISH_CONSTRUCTION, unit);
    effect.builder = Ref<Unit>(builder).offset;
    OrdersPublishEffect(&effect, sizeof(effect));
}

inline void OrdersPublishBuildDecay(Unit* unit, int ticks)
{
    OrdersBuildDecay effect;
    effect.h = OrdersHeaderOf(ORDERS_EFFECT_BUILD_DECAY, unit);
    effect.ticks = ticks;
    OrdersPublishEffect(&effect, sizeof(effect));
}

template <class V>
inline void OrdersPublishSetPosition(Unit* unit, const V& pos, int mode)
{
    OrdersSetPosition effect;
    effect.h = OrdersHeaderOf(ORDERS_EFFECT_SET_POSITION, unit);
    effect.pos[0] = pos.x;
    effect.pos[1] = pos.y;
    effect.pos[2] = pos.z;
    effect.mode = mode;
    OrdersPublishEffect(&effect, sizeof(effect));
}

inline void OrdersPublishRepair(Unit* unit, Unit* builder, float amount)
{
    OrdersRepair effect;
    effect.h = OrdersHeaderOf(ORDERS_EFFECT_REPAIR, unit);
    effect.builder = Ref<Unit>(builder).offset;
    effect.amount = amount;
    OrdersPublishEffect(&effect, sizeof(effect));
}

inline void OrdersPublishBusy(Unit* unit, int until)
{
    OrdersBusy effect;
    effect.h = OrdersHeaderOf(ORDERS_EFFECT_BUSY, unit);
    effect.until = until;
    OrdersPublishEffect(&effect, sizeof(effect));
}

inline void OrdersPublishSetFlags(Unit* unit, unsigned int clear, unsigned int set)
{
    OrdersSetFlags effect;
    effect.h = OrdersHeaderOf(ORDERS_EFFECT_SET_FLAGS, unit);
    effect.clear = clear;
    effect.set = set;
    OrdersPublishEffect(&effect, sizeof(effect));
}

inline void OrdersPublishClearNetDirty(Unit* unit, unsigned int mask)
{
    OrdersClearNetDirty effect;
    effect.h = OrdersHeaderOf(ORDERS_EFFECT_CLEAR_NET_DIRTY, unit);
    effect.mask = mask;
    OrdersPublishEffect(&effect, sizeof(effect));
}

inline void OrdersPublishSetStance(Unit* unit, int fields, int moveOrder, int fireOrder, int group)
{
    OrdersSetStance effect;
    effect.h = OrdersHeaderOf(ORDERS_EFFECT_SET_STANCE, unit);
    effect.fields = fields;
    effect.moveOrder = moveOrder;
    effect.fireOrder = fireOrder;
    effect.group = group;
    OrdersPublishEffect(&effect, sizeof(effect));
}

template <class A>
inline void OrdersPublishSetAngles(Unit* unit, const A& angles)
{
    OrdersSetAngles effect;
    effect.h = OrdersHeaderOf(ORDERS_EFFECT_SET_ANGLES, unit);
    effect.bank = angles.bank;
    effect.heading = angles.heading;
    effect.pitch = angles.pitch;
    OrdersPublishEffect(&effect, sizeof(effect));
}

inline void OrdersPublishSetBuildState(Unit* unit, float buildLeft, int health)
{
    OrdersSetBuildState effect;
    effect.h = OrdersHeaderOf(ORDERS_EFFECT_SET_BUILD_STATE, unit);
    effect.buildLeft = buildLeft;
    effect.health = health;
    OrdersPublishEffect(&effect, sizeof(effect));
}

template <class O>
inline void OrdersPublishUnlink(Unit* unit, O* ref)
{
    OrdersUnlink effect;
    effect.h = OrdersHeaderOf(ORDERS_EFFECT_UNLINK, unit);
    effect.ref = Ref<O>(ref).offset;
    OrdersPublishEffect(&effect, sizeof(effect));
}

inline void OrdersPublishStartScript(Unit* unit, int form, int index, const char* name, unsigned int callback, int update, int count, int a, int b, int c, int d)
{
    OrdersStartScript effect;
    memset(&effect, 0, sizeof(effect));
    effect.h = OrdersHeaderOf(ORDERS_EFFECT_START_SCRIPT, unit);
    effect.form = form;
    effect.index = index;
    if (name)
        strncpy(effect.name, name, sizeof(effect.name) - 1);
    effect.callback = callback;
    effect.update = update;
    effect.count = count;
    effect.args[0] = a;
    effect.args[1] = b;
    effect.args[2] = c;
    effect.args[3] = d;
    OrdersPublishEffect(&effect, sizeof(effect));
}

#endif
