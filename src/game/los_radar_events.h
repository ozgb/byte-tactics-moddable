#ifndef GAME_LOS_RADAR_EVENTS_H
#define GAME_LOS_RADAR_EVENTS_H

#include <string.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/los_radar.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"

inline LosRadarEvent LosRadarEventOf(int kind)
{
    LosRadarEvent event;
    memset(&event, 0, sizeof(event));
    event.kind = kind;
    return event;
}

template <class U>
inline LosRadarEvent LosRadarEventOf(int kind, U* unit)
{
    LosRadarEvent event = LosRadarEventOf(kind);
    event.unit = Ref<U>(unit).offset;
    return event;
}

template <class U>
inline void LosRadarPublishUpdateUnit(int outbox, U* unit)
{
    LosRadarEvent event = LosRadarEventOf(LOS_RADAR_UPDATE_UNIT, unit);
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void LosRadarPublishRemoveUnit(int outbox, U* unit)
{
    LosRadarEvent event = LosRadarEventOf(LOS_RADAR_REMOVE_UNIT, unit);
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void LosRadarPublishRevealUnit(int outbox, U* unit)
{
    LosRadarEvent event = LosRadarEventOf(LOS_RADAR_REVEAL_UNIT, unit);
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void LosRadarPublishSightReset(int outbox, U* unit)
{
    LosRadarEvent event = LosRadarEventOf(LOS_RADAR_SIGHT_RESET, unit);
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void LosRadarPublishSightLoad(int outbox, U* unit, int cell, unsigned char frameIdx)
{
    LosRadarEvent event = LosRadarEventOf(LOS_RADAR_SIGHT_LOAD, unit);
    event.sight.cell = cell;
    event.sight.frameIdx = frameIdx;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class P>
inline void LosRadarPublishAddEyeball(int outbox, const P* pos, int sightDistance, int eyeHeight, int lifetime)
{
    LosRadarEvent event = LosRadarEventOf(LOS_RADAR_ADD_EYEBALL);
    event.pos[0] = pos->x;
    event.pos[1] = pos->y;
    event.pos[2] = pos->z;
    event.sightDistance = sightDistance;
    event.eyeHeight = eyeHeight;
    event.lifetime = lifetime;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class C, class S>
inline void LosRadarPublishCellHeights(int outbox, C cell, S size)
{
    LosRadarEvent event = LosRadarEventOf(LOS_RADAR_CELL_HEIGHTS);
    event.cell[0] = cell.x;
    event.cell[1] = cell.y;
    event.size[0] = size.x;
    event.size[1] = size.y;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void LosRadarPublishRecalculate(int outbox, int value)
{
    LosRadarEvent event = LosRadarEventOf(LOS_RADAR_RECALCULATE);
    event.value = value;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void LosRadarPublishShareMapping(int outbox, int player)
{
    LosRadarEvent event = LosRadarEventOf(LOS_RADAR_SHARE_MAPPING);
    event.player = player;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void LosRadarPublishShareRadar(int outbox, int player)
{
    LosRadarEvent event = LosRadarEventOf(LOS_RADAR_SHARE_RADAR);
    event.player = player;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void LosRadarPublishShareMap(int outbox, int from, int to)
{
    LosRadarEvent event = LosRadarEventOf(LOS_RADAR_SHARE_MAP);
    event.player = from;
    event.value = to;
    SchedPublish(outbox, &event, sizeof(event));
}

#endif
