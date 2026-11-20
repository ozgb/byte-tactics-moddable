#ifndef GAME_LOS_RADAR_EVENTS_H
#define GAME_LOS_RADAR_EVENTS_H

#include <string.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/los_radar.h"
#include "los_radar_event_size.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"

template <class U>
inline LosRadarUnit LosRadarUnitEventOf(int kind, U* unit)
{
    LosRadarUnit event;
    memset(&event, 0, sizeof(event));
    event.h.kind = kind;
    event.unit = Ref<U>(unit).offset;
    return event;
}

template <class U>
inline void LosRadarPublishUpdateUnit(int outbox, U* unit)
{
    LosRadarUnit event = LosRadarUnitEventOf(LOS_RADAR_UPDATE_UNIT, unit);
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void LosRadarPublishRemoveUnit(int outbox, U* unit)
{
    LosRadarUnit event = LosRadarUnitEventOf(LOS_RADAR_REMOVE_UNIT, unit);
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void LosRadarPublishRevealUnit(int outbox, U* unit)
{
    LosRadarUnit event = LosRadarUnitEventOf(LOS_RADAR_REVEAL_UNIT, unit);
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void LosRadarPublishSightReset(int outbox, U* unit)
{
    LosRadarUnit event = LosRadarUnitEventOf(LOS_RADAR_SIGHT_RESET, unit);
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void LosRadarPublishSightLoad(int outbox, U* unit, int cell, unsigned char frameIdx)
{
    LosRadarSightLoad event;
    memset(&event, 0, sizeof(event));
    event.h.kind = LOS_RADAR_SIGHT_LOAD;
    event.unit = Ref<U>(unit).offset;
    event.cell = cell;
    event.frameIdx = frameIdx;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class P>
inline void LosRadarPublishAddEyeball(int outbox, const P* pos, int sightDistance, int eyeHeight, int lifetime)
{
    LosRadarAddEyeball event;
    memset(&event, 0, sizeof(event));
    event.h.kind = LOS_RADAR_ADD_EYEBALL;
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
    LosRadarCellHeights event;
    memset(&event, 0, sizeof(event));
    event.h.kind = LOS_RADAR_CELL_HEIGHTS;
    event.cell[0] = cell.x;
    event.cell[1] = cell.y;
    event.size[0] = size.x;
    event.size[1] = size.y;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void LosRadarPublishRecalculate(int outbox, int value)
{
    LosRadarRecalculate event;
    memset(&event, 0, sizeof(event));
    event.h.kind = LOS_RADAR_RECALCULATE;
    event.reset = value;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void LosRadarPublishShareMapping(int outbox, int player)
{
    LosRadarSharePlayer event;
    memset(&event, 0, sizeof(event));
    event.h.kind = LOS_RADAR_SHARE_MAPPING;
    event.player = player;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void LosRadarPublishShareRadar(int outbox, int player)
{
    LosRadarSharePlayer event;
    memset(&event, 0, sizeof(event));
    event.h.kind = LOS_RADAR_SHARE_RADAR;
    event.player = player;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void LosRadarPublishShareMap(int outbox, int from, int to)
{
    LosRadarShareMap event;
    memset(&event, 0, sizeof(event));
    event.h.kind = LOS_RADAR_SHARE_MAP;
    event.from = from;
    event.to = to;
    SchedPublish(outbox, &event, sizeof(event));
}

#endif
