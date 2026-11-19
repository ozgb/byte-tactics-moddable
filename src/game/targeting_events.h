#ifndef GAME_TARGETING_EVENTS_H
#define GAME_TARGETING_EVENTS_H

#include <string.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/targeting.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"

struct Unit;

template <class U>
inline TargetingEvent TargetingEventOf(int kind, U* unit, int weapon)
{
    TargetingEvent event;
    memset(&event, 0, sizeof(event));
    event.kind = kind;
    event.unit = Ref<U>(unit).offset;
    event.weapon = weapon;
    return event;
}

template <class U, class T>
inline void TargetingPublishSetUnit(int outbox, U* unit, T* target, int weapon)
{
    TargetingEvent event = TargetingEventOf(TARGETING_SET_UNIT, unit, weapon);
    event.target = Ref<T>(target).offset;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U, class T>
inline void TargetingPublishSetUnit(int outbox, U* unit, const Ref<T>& target, int weapon)
{
    TargetingEvent event = TargetingEventOf(TARGETING_SET_UNIT, unit, weapon);
    event.target = target.offset;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U, class P>
inline void TargetingPublishSetPos(int outbox, U* unit, const P* pos, int weapon)
{
    TargetingEvent event = TargetingEventOf(TARGETING_SET_POS, unit, weapon);
    event.pos[0] = pos->x;
    event.pos[1] = pos->y;
    event.pos[2] = pos->z;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void TargetingPublishClear(int outbox, U* unit, int weapon)
{
    TargetingEvent event = TargetingEventOf(TARGETING_CLEAR, unit, weapon);
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void TargetingPublishReset(int outbox, U* unit, int weapon)
{
    TargetingEvent event = TargetingEventOf(TARGETING_RESET, unit, weapon);
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void TargetingPublishLoad(int outbox, U* unit, int weapon, short first, short second)
{
    TargetingEvent event = TargetingEventOf(TARGETING_LOAD, unit, weapon);
    event.pos[0] = first;
    event.pos[2] = second;
    SchedPublish(outbox, &event, sizeof(event));
}

#endif
