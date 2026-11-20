#include "../../interfaces/seams.h"
#include "../../interfaces/los_radar.h"
#include "../../interfaces/services.h"
#include "../game/los_radar_events.h"
#include "los_radar_world.h"

void LosRadarInbox(const void* event)
{
    const LosRadarEventHeader* e = static_cast<const LosRadarEventHeader*>(event);
    unsigned int size = LosRadarEventSize(e->kind);
    HarnessLosRadarEvent(e, size, g_seams.los_radar->Apply(e, size));
}
