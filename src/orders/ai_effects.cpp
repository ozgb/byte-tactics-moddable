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
    const AiAddOrder* e = static_cast<const AiAddOrder*>(event);
    Vec3 pos;
    pos.x = e->pos[0];
    pos.y = e->pos[1];
    pos.z = e->pos[2];
    g_seams.orders->AddOrder(MissionType((unsigned char)e->type), e->queue, UnitOfOffset(e->h.unit), UnitOfOffset(e->target),
                             e->hasPos ? &pos : nullptr, e->params[0], e->params[1]);
}

void OrdersOnAiQueueBuild(const void* event)
{
    const AiQueueBuild* e = static_cast<const AiQueueBuild*>(event);
    g_seams.orders->QueueBuildOrder((char*)&g_game->defs[e->type].unitname[0], UnitOfOffset(e->h.unit), e->count);
}

void OrdersOnAiStop(const void* event)
{
    const AiStop* e = static_cast<const AiStop*>(event);
    g_seams.orders->DeleteOrders(UnitOfOffset(e->h.unit), e->all);
}
