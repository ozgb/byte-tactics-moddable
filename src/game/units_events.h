#ifndef GAME_UNITS_EVENTS_H
#define GAME_UNITS_EVENTS_H

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

#endif
