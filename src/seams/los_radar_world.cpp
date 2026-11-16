#include "../../interfaces/seams.h"
#include "../../interfaces/los_radar.h"
#include "../../interfaces/services.h"
#include "los_radar_world.h"

void LosRadarInbox(const void* event)
{
    const LosRadarEvent* e = static_cast<const LosRadarEvent*>(event);
    HarnessLosRadarEvent(e, g_seams.los_radar->Apply(e));
}
