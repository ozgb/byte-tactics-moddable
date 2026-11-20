#include <stddef.h>
#include <string.h>

#pragma pack(push, 1)
#include "../units/unit.h"
#pragma pack(pop)

#include "../../interfaces/seams.h"
#include "../../interfaces/scripts.h"
#include "../../interfaces/orders.h"
#include "../../interfaces/ref.h"
#include "cob_script.h"
#include "orders_effects.h"

static Unit* UnitOfOffset(unsigned int offset)
{
    Ref<Unit> ref;
    ref.offset = offset;
    return ref.get();
}

void ScriptsOnOrdersStartScript(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersStartScript))
        return;
    const OrdersStartScript* e = static_cast<const OrdersStartScript*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    char name[32];
    memset(name, 0, sizeof(name));
    strncpy(name, e->name, sizeof(name) - 1);
    Ref<Callback> callbackRef;
    callbackRef.offset = e->callback;
    Callback* callback = callbackRef.get();
    CobScript* script = (CobScript*)unit->script.get();
    switch (e->form) {
    case ORDERS_SCRIPT_BY_NAME:
        g_seams.scripts->StartScript(script, name, callback, e->update);
        break;
    case ORDERS_SCRIPT_BY_NAME_WITH_ARGS:
        g_seams.scripts->StartScriptWithArgs(script, name, callback, e->update, e->count, e->args[0], e->args[1], e->args[2], e->args[3]);
        break;
    case ORDERS_SCRIPT_BY_INDEX_WITH_ARGS:
        g_seams.scripts->StartScriptWithArgsByIndex(script, e->index, callback, e->update, e->count, e->args[0], e->args[1], e->args[2], e->args[3]);
        break;
    }
}
