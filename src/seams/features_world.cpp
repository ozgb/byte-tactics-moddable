#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/features.h"
#include "../../interfaces/services.h"
#include "../harness/replay.h"
#include "../harness/replay_services.h"
#include "../game/scheduler.h"
#include "features_world.h"

void FeaturesInbox(const void* event)
{
    const FeaturesEvent* e = static_cast<const FeaturesEvent*>(event);
    HarnessFeaturesEvent(e, g_seams.features->Apply(e));
}

int __stdcall ScheduleFeaturesPlace(const FeaturesEvent* place)
{
    ReplayImport call(RI_SCHEDULE_FEATURES_PLACE);
    call.Ptr(place);
    SchedDeliver(SCHED_FEATURES);
    int outcome = g_seams.features->Place(place);
    HarnessFeaturesEvent(place, outcome);
    return call.Result(outcome);
}
