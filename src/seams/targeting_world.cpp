#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/targeting.h"
#include "../util/engine_services.h"
#include "../util/vec3.h"
#include "../harness/replay.h"
#include "../harness/replay_services.h"
#include "../game/scheduler.h"
#include "../game/targeting_events.h"
#include "targeting_world.h"

void TargetingInbox(const void* event, unsigned int size)
{
    const TargetingEventHeader* e = static_cast<const TargetingEventHeader*>(event);
    HarnessTargetingEvent(e, size, g_seams.targeting->Apply(e, size));
}

Unit* __stdcall ScheduleTargetingTargetUnit(Unit* unit, int weapon)
{
    ReplayImport call(RI_SCHEDULE_TARGETING_TARGET_UNIT);
    call.Ptr(unit);
    call.Int(weapon);
    SchedDeliver(SCHED_TARGETING);
    return call.Result(g_seams.targeting->GetWeaponTargetUnit(unit, weapon));
}

int __stdcall ScheduleTargetingTargetPos(Unit* unit, Vec3* pos, int weapon)
{
    Vec3 seen;
    seen.x = 0;
    seen.y = 0;
    seen.z = 0;
    ReplayImport call(RI_SCHEDULE_TARGETING_TARGET_POS);
    call.Ptr(unit);
    call.Ptr(&seen);
    call.Int(weapon);
    call.Ptr(&seen.x);
    call.Ptr(&seen.y);
    call.Ptr(&seen.z);
    SchedDeliver(SCHED_TARGETING);
    int found = g_seams.targeting->GetWeaponTargetPos(unit, pos, weapon);
    if (found)
        seen = *pos;
    return call.Result(found);
}

Unit* __stdcall ScheduleTargetingFind(Unit* unit, unsigned char weapon, int useRange)
{
    ReplayImport call(RI_SCHEDULE_TARGETING_FIND);
    call.Ptr(unit);
    call.Int(weapon);
    call.Int(useRange);
    SchedDeliver(SCHED_TARGETING);
    return call.Result(g_seams.targeting->FindWeaponTarget(unit, weapon, useRange));
}

Unit* __stdcall ScheduleTargetingFireAtWill(Unit* unit)
{
    ReplayImport call(RI_SCHEDULE_TARGETING_FIRE_AT_WILL);
    call.Ptr(unit);
    SchedDeliver(SCHED_TARGETING);
    return call.Result(g_seams.targeting->FindBestTargetIfFireAtWill(unit));
}
