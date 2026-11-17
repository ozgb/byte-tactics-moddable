#include "../../interfaces/ai.h"

#include "ai_idle.h"

namespace {

AiSeam s_idle;
const AiSeam* s_standard = 0;

int __stdcall ApplyIdle(const AiEvent* event)
{
    if (event->kind == AI_REACT_TO_ATTACK || event->kind == AI_DAMAGE_SPLIT)
        return 0;
    return s_standard->Apply(event);
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
