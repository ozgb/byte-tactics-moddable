#ifndef GAME_FEATURES_EVENTS_H
#define GAME_FEATURES_EVENTS_H

#include <string.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/features.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"

struct Unit;

inline FeaturesEvent FeaturesEventAt(int kind, int x, int z)
{
    FeaturesEvent event;
    memset(&event, 0, sizeof(event));
    event.kind = kind;
    event.x = x;
    event.z = z;
    event.owner = 10;
    return event;
}

inline FeaturesEvent FeaturesEventOf(int kind, int cellIndex, int width)
{
    return FeaturesEventAt(kind, cellIndex % width, cellIndex / width);
}

inline void FeaturesPublishPlace(int outbox, int cellIndex, int width, int feature)
{
    FeaturesEvent event = FeaturesEventOf(FEATURES_PLACE, cellIndex, width);
    event.feature = feature;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void FeaturesPublishRemove(int outbox, int cellIndex, int width)
{
    FeaturesEvent event = FeaturesEventOf(FEATURES_REMOVE, cellIndex, width);
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U, class V>
inline void FeaturesPublishReclaim(int outbox, U* unit, const V* pos)
{
    FeaturesEvent event = FeaturesEventAt(FEATURES_RECLAIM, 0, 0);
    event.unit = Ref<U>(unit).offset;
    event.pos[0] = pos->x;
    event.pos[1] = pos->y;
    event.pos[2] = pos->z;
    SchedPublish(outbox, &event, sizeof(event));
}

#endif
