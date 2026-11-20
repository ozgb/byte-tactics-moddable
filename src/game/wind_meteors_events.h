#ifndef GAME_WIND_METEORS_EVENTS_H
#define GAME_WIND_METEORS_EVENTS_H

#include "../../interfaces/wind_meteors.h"

inline unsigned int WindMeteorsEventSize(int kind)
{
    switch (kind) {
    case WIND_METEORS_PARAMS:
        return sizeof(WindMeteorsParams);
    case WIND_METEORS_EFFECT_SPAWN:
        return sizeof(WindMeteorsSpawn);
    }
    return sizeof(WindMeteorsEventHeader);
}

#endif
