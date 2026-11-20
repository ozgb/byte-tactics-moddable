#ifndef GAME_AI_EVENTS_H
#define GAME_AI_EVENTS_H

#include <string.h>

#include "../../interfaces/ai.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"

struct Unit;

inline unsigned int AiEventSize(int kind)
{
    switch (kind) {
    case AI_REACT_TO_ATTACK:
        return sizeof(AiReactToAttack);
    case AI_SET_SQUAD:
        return sizeof(AiSetSquad);
    case AI_PLAN:
        return sizeof(AiPlan);
    case AI_WEIGHT:
        return sizeof(AiWeight);
    case AI_LIMIT:
        return sizeof(AiLimit);
    case AI_EFFECT_ADD_ORDER:
        return sizeof(AiAddOrder);
    case AI_EFFECT_QUEUE_BUILD:
        return sizeof(AiQueueBuild);
    case AI_EFFECT_STOP:
        return sizeof(AiStop);
    case AI_EFFECT_ACTIVATE:
        return sizeof(AiActivate);
    case AI_EFFECT_SET_STATES:
        return sizeof(AiSetStates);
    default:
        return sizeof(AiEventHeader);
    }
}

template <class E>
inline void AiFillHeader(E* event, int kind)
{
    memset(event, 0, sizeof(*event));
    event->h.kind = kind;
}

template <class E, class U>
inline void AiFillHeader(E* event, int kind, U* unit)
{
    AiFillHeader(event, kind);
    event->h.unit = Ref<U>(unit).offset;
}

template <class U>
inline void AiPublishSetSquad(int outbox, U* unit, int squad)
{
    AiSetSquad event;
    AiFillHeader(&event, AI_SET_SQUAD, unit);
    event.squad = squad;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void AiPublishPlan(int outbox, int bits)
{
    AiPlan event;
    AiFillHeader(&event, AI_PLAN);
    event.difficulties = bits;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void AiPublishWeight(int outbox, const unsigned int* types, float scale, int lock)
{
    AiWeight event;
    AiFillHeader(&event, AI_WEIGHT);
    memcpy(event.types, types, sizeof(event.types));
    event.scale = scale;
    event.fix = lock;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void AiPublishLimit(int outbox, const unsigned int* types, int limit, int lock)
{
    AiLimit event;
    AiFillHeader(&event, AI_LIMIT);
    memcpy(event.types, types, sizeof(event.types));
    event.limit = limit;
    event.fix = lock;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void AiPublishAddOrder(U* unit, int kind, int remove, Unit* target, const void* pos, int p6, int p7)
{
    AiAddOrder event;
    AiFillHeader(&event, AI_EFFECT_ADD_ORDER, unit);
    event.type = kind;
    event.queue = remove;
    event.target = Ref<Unit>(target).offset;
    if (pos != nullptr) {
        event.hasPos = 1;
        memcpy(event.pos, pos, sizeof(event.pos));
    }
    event.params[0] = p6;
    event.params[1] = p7;
    HarnessAiEvent(&event.h, sizeof(event), 1);
    SchedPublish(SCHED_AI, &event, sizeof(event));
}

template <class U>
inline void AiPublishQueueBuild(U* unit, int type, int count)
{
    AiQueueBuild event;
    AiFillHeader(&event, AI_EFFECT_QUEUE_BUILD, unit);
    event.type = type;
    event.count = count;
    HarnessAiEvent(&event.h, sizeof(event), 1);
    SchedPublish(SCHED_AI, &event, sizeof(event));
}

template <class U>
inline void AiPublishStop(U* unit, int all)
{
    AiStop event;
    AiFillHeader(&event, AI_EFFECT_STOP, unit);
    event.all = all;
    HarnessAiEvent(&event.h, sizeof(event), 1);
    SchedPublish(SCHED_AI, &event, sizeof(event));
}

template <class U>
inline void AiPublishActivate(U* unit, int bits, int on)
{
    AiActivate event;
    AiFillHeader(&event, AI_EFFECT_ACTIVATE, unit);
    event.bits = bits;
    event.on = on;
    HarnessAiEvent(&event.h, sizeof(event), 1);
    SchedPublish(SCHED_AI, &event, sizeof(event));
}

template <class U>
inline void AiPublishSetStates(U* unit, int movement, int firing)
{
    AiSetStates event;
    AiFillHeader(&event, AI_EFFECT_SET_STATES, unit);
    event.movement = movement;
    event.firing = firing;
    HarnessAiEvent(&event.h, sizeof(event), 1);
    SchedPublish(SCHED_AI, &event, sizeof(event));
}

#endif
