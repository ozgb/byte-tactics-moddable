#ifndef GAME_TARGETING_EVENTS_H
#define GAME_TARGETING_EVENTS_H

#include <string.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/targeting.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"

struct Unit;

template <class U>
inline TargetingEventHeader TargetingHeaderOf(int kind, U* unit, int weapon)
{
    TargetingEventHeader header;
    header.kind = kind;
    header.unit = Ref<U>(unit).offset;
    header.weapon = weapon;
    return header;
}

template <class U, class T>
inline void TargetingPublishSetUnit(int outbox, U* unit, T* target, int weapon)
{
    TargetingSetUnit event;
    event.h = TargetingHeaderOf(TARGETING_SET_UNIT, unit, weapon);
    event.target = Ref<T>(target).offset;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U, class T>
inline void TargetingPublishSetUnit(int outbox, U* unit, const Ref<T>& target, int weapon)
{
    TargetingSetUnit event;
    event.h = TargetingHeaderOf(TARGETING_SET_UNIT, unit, weapon);
    event.target = target.offset;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U, class P>
inline void TargetingPublishSetPos(int outbox, U* unit, const P* pos, int weapon)
{
    TargetingSetPos event;
    event.h = TargetingHeaderOf(TARGETING_SET_POS, unit, weapon);
    event.pos[0] = pos->x;
    event.pos[1] = pos->y;
    event.pos[2] = pos->z;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void TargetingPublishClear(int outbox, U* unit, int weapon)
{
    TargetingEventHeader event = TargetingHeaderOf(TARGETING_CLEAR, unit, weapon);
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void TargetingPublishReset(int outbox, U* unit, int weapon)
{
    TargetingEventHeader event = TargetingHeaderOf(TARGETING_RESET, unit, weapon);
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void TargetingPublishLoad(int outbox, U* unit, int weapon, short first, short second)
{
    TargetingLoad event;
    event.h = TargetingHeaderOf(TARGETING_LOAD, unit, weapon);
    event.xOrUnit = first;
    event.zOrSentinel = second;
    SchedPublish(outbox, &event, sizeof(event));
}

#endif
