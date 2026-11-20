#include <stddef.h>

#include "../../interfaces/seams.h"
#include <string.h>

#include "../../interfaces/ai.h"
#include "../../interfaces/weapons.h"
#include "../../interfaces/services.h"
#include "../game/ai_events.h"
#include "ai_world.h"

void AiInbox(const void* event)
{
    const AiEventHeader* e = static_cast<const AiEventHeader*>(event);
    unsigned int size = AiEventSize(e->kind);
    HarnessAiEvent(e, size, g_seams.ai->Apply(e, size));
}

void AiOnWeaponsAttacked(const void* event)
{
    const WeaponsAttacked* effect = static_cast<const WeaponsAttacked*>(event);
    AiReactToAttack e;
    memset(&e, 0, sizeof(e));
    e.h.kind = AI_REACT_TO_ATTACK;
    e.h.unit = effect->h.unit;
    e.attacker = effect->attacker;
    e.damage = effect->amount;
    AiInbox(&e);
}
