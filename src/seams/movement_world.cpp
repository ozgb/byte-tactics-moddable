#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/movement.h"
#include "../util/engine_services.h"
#include "movement_world.h"

void MovementInbox(const void* event, unsigned int size)
{
    const MovementEventHeader* e = static_cast<const MovementEventHeader*>(event);
    HarnessMovementEvent(e, size, g_seams.movement->Apply(e, size));
}
