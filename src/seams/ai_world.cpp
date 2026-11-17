#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/ai.h"
#include "../../interfaces/services.h"
#include "ai_world.h"

#include "../../link/globals.h"

void AiInbox(const void* event)
{
    const AiEvent* e = static_cast<const AiEvent*>(event);
    HarnessAiEvent(e, g_seams.ai->Apply(e));
}
