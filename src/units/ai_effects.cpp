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

void UnitsOnAiActivate(const void* event, unsigned int size)
{
    if (size < sizeof(AiActivate))
        return;
    const AiActivate* e = static_cast<const AiActivate*>(event);
    g_seams.units->SetStateBits(UnitOfOffset(e->h.unit), e->bits, e->on);
}

void UnitsOnAiSetStates(const void* event, unsigned int size)
{
    if (size < sizeof(AiSetStates))
        return;
    const AiSetStates* e = static_cast<const AiSetStates*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    unit->flags = (unit->flags & ~0x3c0000u) | ((unsigned int)e->movement << 18) | ((unsigned int)e->firing << 20);
}
