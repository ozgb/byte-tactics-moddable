#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/features.h"
#include "../util/engine_services.h"
#include "../harness/replay.h"
#include "../harness/replay_services.h"
#include "../game/scheduler.h"
#include "features_world.h"

void FeaturesInbox(const void* event, unsigned int size)
{
    const FeaturesEventHeader* e = static_cast<const FeaturesEventHeader*>(event);
    HarnessFeaturesEvent(e, size, g_seams.features->Apply(e, size));
}

int __stdcall ScheduleFeaturesPlace(const FeaturesPlace* place)
{
    ReplayImport call(RI_SCHEDULE_FEATURES_PLACE);
    call.Ptr(place);
    SchedDeliver(SCHED_FEATURES);
    int outcome = g_seams.features->Place(place);
    HarnessFeaturesEvent(&place->h, sizeof(*place), outcome);
    return call.Result(outcome);
}
