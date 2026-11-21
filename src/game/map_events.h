#ifndef GAME_MAP_EVENTS_H
#define GAME_MAP_EVENTS_H

#include <string.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/map.h"
#include "map_event_size.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"

template <class U>
inline void MapPublishUnit(int outbox, int kind, U* unit)
{
    MapEventHeader event;
    memset(&event, 0, sizeof(event));
    event.kind = kind;
    event.unit = Ref<U>(unit).offset;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void MapPublishSetYard(int outbox, U* unit, int open)
{
    MapSetYard event;
    memset(&event, 0, sizeof(event));
    event.h.kind = MAP_SET_YARD;
    event.h.unit = Ref<U>(unit).offset;
    event.open = open;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void MapPublishPathLockStamp(int outbox, U* unit, int stamp)
{
    MapPathLockStamp event;
    memset(&event, 0, sizeof(event));
    event.h.kind = MAP_LOAD_PATH_LOCK_STAMP;
    event.h.unit = Ref<U>(unit).offset;
    event.stamp = stamp;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class C, class S>
inline MapPassMaps MapPassMapsOf(C cell, S size)
{
    MapPassMaps event;
    memset(&event, 0, sizeof(event));
    event.h.kind = MAP_PASS_MAPS;
    event.cell[0] = cell.x;
    event.cell[1] = cell.y;
    event.size[0] = size.x;
    event.size[1] = size.y;
    return event;
}

template <class C, class S>
inline void MapPublishPassMaps(int outbox, C cell, S size)
{
    MapPassMaps event = MapPassMapsOf(cell, size);
    SchedPublish(outbox, &event, sizeof(event));
}

inline void MapPublishCellsChanged(int left, int top, int right, int bottom, unsigned int classes, int cause)
{
    MapCellsChanged event;
    memset(&event, 0, sizeof(event));
    event.h.kind = MAP_EFFECT_CELLS_CHANGED;
    event.rect[0] = left;
    event.rect[1] = top;
    event.rect[2] = right;
    event.rect[3] = bottom;
    event.classes = classes;
    event.cause = cause;
    HarnessMapEvent(&event.h, sizeof(event), 1);
    SchedPublish(SCHED_MAP, &event, sizeof(event));
}

#endif
