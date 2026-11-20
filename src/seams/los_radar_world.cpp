#include "../../interfaces/seams.h"
#include "../../interfaces/los_radar.h"
#include "../../interfaces/services.h"
#include "los_radar_world.h"

void LosRadarInbox(const void* event, unsigned int size)
{
    const LosRadarEventHeader* e = static_cast<const LosRadarEventHeader*>(event);
    HarnessLosRadarEvent(e, size, g_seams.los_radar->Apply(e, size));
}
