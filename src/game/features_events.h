#ifndef GAME_FEATURES_EVENTS_H
#define GAME_FEATURES_EVENTS_H

#include <string.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/features.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"

struct Unit;

inline FeaturesEventHeader FeaturesHeaderOf(int kind)
{
    FeaturesEventHeader header;
    header.kind = kind;
    return header;
}

inline unsigned int FeaturesEventSize(int kind)
{
    switch (kind) {
    case FEATURES_PLACE:
        return sizeof(FeaturesPlace);
    case FEATURES_REMOVE:
        return sizeof(FeaturesRemove);
    case FEATURES_DAMAGE:
        return sizeof(FeaturesDamage);
    case FEATURES_RECLAIM:
        return sizeof(FeaturesReclaim);
    case FEATURES_EFFECT_FREE_STATE:
        return sizeof(FeaturesFreeState);
    case FEATURES_EFFECT_INIT_STATE:
        return sizeof(FeaturesInitState);
    case FEATURES_EFFECT_PASS_MAPS:
        return sizeof(FeaturesPassMaps);
    case FEATURES_EFFECT_AREA_DAMAGE:
        return sizeof(FeaturesAreaDamage);
    }
    return sizeof(FeaturesEventHeader);
}

inline FeaturesPlace FeaturesPlaceOf(int cellIndex, int width)
{
    FeaturesPlace event;
    memset(&event, 0, sizeof(event));
    event.h = FeaturesHeaderOf(FEATURES_PLACE);
    event.x = cellIndex % width;
    event.z = cellIndex / width;
    event.owner = 10;
    return event;
}

inline void FeaturesPublishPlace(int outbox, int cellIndex, int width, int feature)
{
    FeaturesPlace event = FeaturesPlaceOf(cellIndex, width);
    event.feature = feature;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void FeaturesPublishRemove(int outbox, int cellIndex, int width)
{
    FeaturesRemove event;
    event.h = FeaturesHeaderOf(FEATURES_REMOVE);
    event.x = cellIndex % width;
    event.z = cellIndex / width;
    event.force = 0;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void FeaturesPublishDamage(int outbox, int x, int z, int weapon)
{
    FeaturesDamage event;
    event.h = FeaturesHeaderOf(FEATURES_DAMAGE);
    event.x = x;
    event.z = z;
    event.weapon = weapon;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U, class V>
inline void FeaturesPublishReclaim(int outbox, U* unit, const V* pos)
{
    FeaturesReclaim event;
    event.h = FeaturesHeaderOf(FEATURES_RECLAIM);
    event.unit = Ref<U>(unit).offset;
    event.pos[0] = pos->x;
    event.pos[1] = pos->y;
    event.pos[2] = pos->z;
    SchedPublish(outbox, &event, sizeof(event));
}

#endif
