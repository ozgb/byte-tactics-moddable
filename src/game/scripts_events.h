#ifndef GAME_SCRIPTS_EVENTS_H
#define GAME_SCRIPTS_EVENTS_H

#include <string.h>

#include "../../interfaces/scripts.h"
#include "../../interfaces/ref.h"
#include "../util/engine_services.h"

inline ScriptsEventHeader ScriptsHeaderOf(int kind)
{
    ScriptsEventHeader header;
    header.kind = kind;
    header.unit = 0;
    return header;
}

template <class U>
inline ScriptsEventHeader ScriptsHeaderOf(int kind, U* unit)
{
    ScriptsEventHeader header = ScriptsHeaderOf(kind);
    header.unit = Ref<U>(unit).offset;
    return header;
}

template <class U>
inline ScriptsStart ScriptsStartOf(U* unit, int form, const char* name, int index, unsigned int callback, int update, int count, int a, int b, int c, int d)
{
    ScriptsStart event;
    memset(&event, 0, sizeof(event));
    event.h = ScriptsHeaderOf(SCRIPTS_START, unit);
    event.form = form;
    event.index = index;
    if (name != nullptr)
        strncpy(event.name, name, sizeof(event.name) - 1);
    event.callback = callback;
    event.update = update;
    event.count = count;
    event.args[0] = a;
    event.args[1] = b;
    event.args[2] = c;
    event.args[3] = d;
    return event;
}

template <class U>
inline void ScriptsPublishStart(int outbox, U* unit, const char* name, int update)
{
    ScriptsStart event = ScriptsStartOf(unit, SCRIPTS_START_BY_NAME, name, 0, 0, update, 0, 0, 0, 0, 0);
    SchedPublish(outbox, &event, sizeof(event));
}

template <class U>
inline void ScriptsPublishStartWithArgs(int outbox, U* unit, const char* name, int update, int count, int a, int b, int c, int d)
{
    ScriptsStart event = ScriptsStartOf(unit, SCRIPTS_START_BY_NAME_WITH_ARGS, name, 0, 0, update, count, a, b, c, d);
    SchedPublish(outbox, &event, sizeof(event));
}

#endif
