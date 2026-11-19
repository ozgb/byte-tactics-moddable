#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/ai.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/orders.h"
#include "../util/vec3.h"
#include "../game/game.h"
#include "../units/unit_def.h"
#include "../units/unit.h"
#include "mission_type.h"
#include "ai_effects.h"

#include "../../link/globals.h"

static Unit* UnitOfOffset(unsigned int offset)
{
    Ref<Unit> ref;
    ref.offset = offset;
    return ref.get();
}

void OrdersOnAiAddOrder(const void* event)
{
    const AiEvent* e = static_cast<const AiEvent*>(event);
    Vec3 pos;
    pos.x = e->pos[0];
    pos.y = e->pos[1];
    pos.z = e->pos[2];
    g_seams.orders->AddOrder(MissionType((unsigned char)e->value), e->extra, UnitOfOffset(e->unit), UnitOfOffset(e->target),
                             e->hasPos ? &pos : nullptr, e->params[0], e->params[1]);
}

void OrdersOnAiQueueBuild(const void* event)
{
    const AiEvent* e = static_cast<const AiEvent*>(event);
    g_seams.orders->QueueBuildOrder((char*)&g_game->defs[e->value].unitname[0], UnitOfOffset(e->unit), e->extra);
}

void OrdersOnAiStop(const void* event)
{
    const AiEvent* e = static_cast<const AiEvent*>(event);
    g_seams.orders->DeleteOrders(UnitOfOffset(e->unit), e->value);
}
