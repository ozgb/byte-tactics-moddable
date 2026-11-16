#ifndef GAME_MAP_EVENTS_H
#define GAME_MAP_EVENTS_H

#include <string.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/map.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"

inline MapEvent MapEventOf(int kind)
{
    MapEvent event;
    memset(&event, 0, sizeof(event));
    event.kind = kind;
    return event;
}

template <class U>
inline MapEvent MapEventOf(int kind, U* unit)
{
    MapEvent event = MapEventOf(kind);
    event.unit = Ref<U>(unit).offset;
    return event;
}

template <class U>
inline void MapPublishUnit(int outbox, int kind, U* unit)
{
    MapEvent event = MapEventOf(kind, unit);
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void MapPublishSetYard(int outbox, U* unit, int open)
{
    MapEvent event = MapEventOf(MAP_SET_YARD, unit);
    event.value = open;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class C, class S>
inline MapEvent MapRectEventOf(int kind, int value, C cell, S size)
{
    MapEvent event = MapEventOf(kind);
    event.value = value;
    event.cell[0] = cell.x;
    event.cell[1] = cell.y;
    event.size[0] = size.x;
    event.size[1] = size.y;
    return event;
}

template <class C, class S>
inline void MapPublishRefreshPassMap(int outbox, int movementClass, C cell, S size)
{
    MapEvent event = MapRectEventOf(MAP_REFRESH_PASS_MAP, movementClass, cell, size);
    SchedPublish(outbox, &event, sizeof(event));
}

template <class C, class S>
inline void MapPublishPassMaps(int outbox, C cell, S size)
{
    MapEvent event = MapRectEventOf(MAP_PASS_MAPS, 0, cell, size);
    SchedPublish(outbox, &event, sizeof(event));
}

inline void MapPublishCellsChanged(int left, int top, int right, int bottom, unsigned int classes, int cause)
{
    MapEvent event = MapEventOf(MAP_EFFECT_CELLS_CHANGED);
    event.rect[0] = left;
    event.rect[1] = top;
    event.rect[2] = right;
    event.rect[3] = bottom;
    event.classes = classes;
    event.cause = cause;
    SchedPublish(SCHED_MAP, &event, sizeof(event));
}

#endif
