#include <stddef.h>

#include "../../interfaces/seams.h"
#include <string.h>

#include "../../interfaces/ai.h"
#include "../../interfaces/weapons.h"
#include "../../interfaces/services.h"
#include "ai_world.h"

void AiInbox(const void* event)
{
    const AiEvent* e = static_cast<const AiEvent*>(event);
    HarnessAiEvent(e, g_seams.ai->Apply(e));
}

void AiOnWeaponsAttacked(const void* event)
{
    const WeaponsEvent* effect = static_cast<const WeaponsEvent*>(event);
    AiEvent e;
    memset(&e, 0, sizeof(e));
    e.kind = AI_REACT_TO_ATTACK;
    e.unit = effect->unit;
    e.target = effect->target;
    e.value = effect->value;
    AiInbox(&e);
}
