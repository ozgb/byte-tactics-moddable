#ifndef GAME_WIND_METEORS_EVENTS_H
#define GAME_WIND_METEORS_EVENTS_H

#include <string.h>

#include "../util/engine_services.h"
#include "../../interfaces/wind_meteors.h"

inline void PublishWindMeteors(int kind, const MeteorShowerParams* params)
{
    if (kind == WIND_METEORS_PARAMS) {
        WindMeteorsParams event;
        memset(&event, 0, sizeof event);
        event.h.kind = kind;
        if (params != nullptr)
            event.params = *params;
        SchedPublish(SCHED_ENGINE, &event, sizeof event);
        return;
    }
    WindMeteorsEventHeader event;
    event.kind = kind;
    SchedPublish(SCHED_ENGINE, &event, sizeof event);
}

inline void PublishWindMeteorsLimits(int min, int max)
{
    WindMeteorsLimits event;
    event.h.kind = WIND_METEORS_LIMITS;
    event.min = min;
    event.max = max;
    SchedPublish(SCHED_ENGINE, &event, sizeof event);
}

#endif
