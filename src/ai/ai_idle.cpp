#include "../../interfaces/ai.h"
#include "../../interfaces/ref.h"
#include "../units/unit.h"
#include "../network/player.h"

#include "ai_idle.h"

namespace {

AiSeam s_idle;
const AiSeam* s_standard = nullptr;

int __stdcall ApplyIdle(const AiEventHeader* event, unsigned int size)
{
    if (size < sizeof(AiEventHeader))
        return 0;
    if (event->kind == AI_REACT_TO_ATTACK) {
        Ref<Unit> ref;
        ref.offset = event->unit;
        Unit* unit = ref.get();
        if (unit != nullptr && unit->player != nullptr && unit->player->type == 2)
            return 0;
    }
    return s_standard->Apply(event, size);
}

void __stdcall UpdateSquadsIdle(int)
{
}

void __stdcall UpdatePlayerAIIdle(int)
{
}

}

AiSeam* AiIdleSeam(const AiSeam* standard)
{
    s_standard = standard;
    s_idle = *standard;
    s_idle.Apply = ApplyIdle;
    s_idle.UpdateSquads = UpdateSquadsIdle;
    s_idle.UpdatePlayerAI = UpdatePlayerAIIdle;
    return &s_idle;
}
