#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/ai.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/units.h"
#include "unit.h"
#include "ai_effects.h"

static Unit* UnitOfOffset(unsigned int offset)
{
    Ref<Unit> ref;
    ref.offset = offset;
    return ref.get();
}

void UnitsOnAiActivate(const void* event)
{
    const AiEvent* e = static_cast<const AiEvent*>(event);
    g_seams.units->SetStateBits(UnitOfOffset(e->unit), e->value, e->extra);
}

void UnitsOnAiSetStates(const void* event)
{
    const AiEvent* e = static_cast<const AiEvent*>(event);
    Unit* unit = UnitOfOffset(e->unit);
    unit->flags = (unit->flags & ~0x3c0000u) | ((unsigned int)e->value << 18) | ((unsigned int)e->extra << 20);
}
