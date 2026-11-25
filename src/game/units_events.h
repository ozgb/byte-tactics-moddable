#ifndef GAME_UNITS_EVENTS_H
#define GAME_UNITS_EVENTS_H

#include <string.h>

#include "../../interfaces/units.h"
#include "../../interfaces/ref.h"
#include "../util/engine_services.h"

inline UnitsEventHeader UnitsHeaderOf(int kind)
{
    UnitsEventHeader header;
    header.kind = kind;
    header.unit = 0;
    return header;
}

template <class U>
inline UnitsEventHeader UnitsHeaderOf(int kind, U* unit)
{
    UnitsEventHeader header = UnitsHeaderOf(kind);
    header.unit = Ref<U>(unit).offset;
    return header;
}

template <class U>
inline void UnitsPublishScriptValue(int outbox, U* unit, int which, int value)
{
    UnitsScriptValue event;
    event.h = UnitsHeaderOf(UNITS_SCRIPT_VALUE, unit);
    event.which = which;
    event.value = value;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void UnitsPublishAttachToPiece(int outbox, U* unit, U* target, int piece, int mode)
{
    UnitsAttachToPiece event;
    event.h = UnitsHeaderOf(UNITS_ATTACH_TO_PIECE, unit);
    event.target = Ref<U>(target).offset;
    event.piece = piece;
    event.mode = mode;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void UnitsPublishCreate(int outbox, int player, int type, const int* pos)
{
    UnitsCreate event;
    memset(&event, 0, sizeof(event));
    event.h = UnitsHeaderOf(UNITS_CREATE);
    event.player = player;
    event.type = type;
    event.pos[0] = pos[0];
    event.pos[1] = pos[1];
    event.pos[2] = pos[2];
    SchedPublish(outbox, &event, sizeof(event));
}

inline void UnitsPublishKillAll(int outbox)
{
    UnitsEventHeader event = UnitsHeaderOf(UNITS_KILL_ALL);
    SchedPublish(outbox, &event, sizeof(event));
}

inline void UnitsPublishReloadType(int outbox, int type)
{
    UnitsReloadType event;
    memset(&event, 0, sizeof(event));
    event.h = UnitsHeaderOf(UNITS_RELOAD_TYPE);
    event.type = type;
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void UnitsPublishSetFlags(int outbox, U* unit, unsigned int clear, unsigned int set)
{
    UnitsSetFlags event;
    memset(&event, 0, sizeof(event));
    event.h = UnitsHeaderOf(UNITS_SET_FLAGS, unit);
    event.clear = clear;
    event.set = set;
    SchedPublish(outbox, &event, sizeof(event));
}

#endif
