#ifndef GAME_AI_EVENTS_H
#define GAME_AI_EVENTS_H

#include <string.h>

#include "../../interfaces/ai.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"

struct Unit;

inline AiEvent AiEventOf(int kind)
{
    AiEvent event;
    memset(&event, 0, sizeof(event));
    event.kind = kind;
    return event;
}

template <class U>
inline AiEvent AiEventOf(int kind, U* unit)
{
    AiEvent event = AiEventOf(kind);
    event.unit = Ref<U>(unit).offset;
    return event;
}

template <class U>
inline void AiPublishSetSquad(int outbox, U* unit, int squad)
{
    AiEvent event = AiEventOf(AI_SET_SQUAD, unit);
    event.value = squad;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void AiPublishPlan(int outbox, int bits)
{
    AiEvent event = AiEventOf(AI_PLAN);
    event.value = bits;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void AiPublishWeight(int outbox, const unsigned int* types, float scale, int lock)
{
    AiEvent event = AiEventOf(AI_WEIGHT);
    memcpy(event.types, types, sizeof(event.types));
    event.scale = scale;
    event.extra = lock;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void AiPublishLimit(int outbox, const unsigned int* types, int limit, int lock)
{
    AiEvent event = AiEventOf(AI_LIMIT);
    memcpy(event.types, types, sizeof(event.types));
    event.value = limit;
    event.extra = lock;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void AiPublishAddOrder(U* unit, int kind, int remove, Unit* target, const void* pos, int p6, int p7)
{
    AiEvent event = AiEventOf(AI_EFFECT_ADD_ORDER, unit);
    event.value = kind;
    event.extra = remove;
    event.target = Ref<Unit>(target).offset;
    if (pos != nullptr) {
        event.hasPos = 1;
        memcpy(event.pos, pos, sizeof(event.pos));
    }
    event.params[0] = p6;
    event.params[1] = p7;
    HarnessAiEvent(&event, 1);
    SchedPublish(SCHED_AI, &event, sizeof(event));
}

template <class U>
inline void AiPublishQueueBuild(U* unit, int type, int count)
{
    AiEvent event = AiEventOf(AI_EFFECT_QUEUE_BUILD, unit);
    event.value = type;
    event.extra = count;
    HarnessAiEvent(&event, 1);
    SchedPublish(SCHED_AI, &event, sizeof(event));
}

template <class U>
inline void AiPublishStop(U* unit, int all)
{
    AiEvent event = AiEventOf(AI_EFFECT_STOP, unit);
    event.value = all;
    HarnessAiEvent(&event, 1);
    SchedPublish(SCHED_AI, &event, sizeof(event));
}

template <class U>
inline void AiPublishActivate(U* unit, int bits, int on)
{
    AiEvent event = AiEventOf(AI_EFFECT_ACTIVATE, unit);
    event.value = bits;
    event.extra = on;
    HarnessAiEvent(&event, 1);
    SchedPublish(SCHED_AI, &event, sizeof(event));
}

template <class U>
inline void AiPublishSetStates(U* unit, int movement, int firing)
{
    AiEvent event = AiEventOf(AI_EFFECT_SET_STATES, unit);
    event.value = movement;
    event.extra = firing;
    HarnessAiEvent(&event, 1);
    SchedPublish(SCHED_AI, &event, sizeof(event));
}

#endif
