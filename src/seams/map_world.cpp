#include <stddef.h>

#include "../units/unit_def.h"

#include "../../interfaces/seams.h"
#include "../../interfaces/map.h"
#include "../../interfaces/features.h"
#include "../../interfaces/services.h"
#include "../harness/replay.h"
#include "../harness/replay_services.h"
#include "../game/scheduler.h"
#include "../game/game.h"
#include "../util/vec3.h"
#include "../game/map_events.h"
#include "../units/unit.h"
#include "../../link/globals.h"
#include "map_world.h"

void MapInbox(const void* event)
{
    const MapEvent* e = static_cast<const MapEvent*>(event);
    HarnessMapEvent(e, g_seams.map->Apply(e));
}

void MapOnFeaturesPassMaps(const void* event)
{
    const FeaturesEvent* effect = static_cast<const FeaturesEvent*>(event);
    Point16 at;
    at.x = (short)effect->x;
    at.y = (short)effect->z;
    Point16 footprint;
    footprint.x = (short)effect->width;
    footprint.y = (short)effect->height;
    MapEvent passMaps = MapRectEventOf(MAP_PASS_MAPS, 0, at, footprint);
    MapInbox(&passMaps);
}

void __stdcall VisitUnitsInRange(const Vec3* pos, int range, UnitVisitor* visitor)
{
    unsigned short ids[256];
    int skip = 0;
    for (;;) {
        int count = g_seams.map->UnitsInRange(pos, range, skip, ids, 256);
        for (int i = 0; i < count; i++)
            visitor->Visit(&g_game->units[ids[i]]);
        if (count < 256)
            return;
        skip += count;
    }
}

int __stdcall ScheduleMapCanBuildAt(UnitDef* unit, Point16 cell, short type, Player* player)
{
    ReplayImport call(RI_SCHEDULE_MAP_CAN_BUILD_AT);
    call.Ptr(unit);
    call.Ptr(&cell);
    call.Int(type);
    call.Ptr(player);
    SchedDeliver(SCHED_MAP);
    return call.Result(g_seams.map->CanBuildAt(unit, cell, type, player));
}

int __stdcall ScheduleMapCanPlaceUnitFootprint(UnitDef* unit, unsigned short otherTypeId, Point16 cell, int flags)
{
    ReplayImport call(RI_SCHEDULE_MAP_CAN_PLACE_UNIT_FOOTPRINT);
    call.Ptr(unit);
    call.Int(otherTypeId);
    call.Ptr(&cell);
    call.Int(flags);
    SchedDeliver(SCHED_MAP);
    return call.Result(g_seams.map->CanPlaceUnitFootprint(unit, otherTypeId, cell, flags));
}

int __stdcall ScheduleMapPickReclaimable(Vec3* pos, int range, Vec3** energy, float* energyAmount, Vec3** metal, float* metalAmount)
{
    ReplayImport call(RI_SCHEDULE_MAP_PICK_RECLAIMABLE);
    call.Ptr(pos);
    call.Int(range);
    call.Ptr(energy);
    call.Ptr(energyAmount);
    call.Ptr(metal);
    call.Ptr(metalAmount);
    SchedDeliver(SCHED_MAP);
    return call.Result(g_seams.map->PickRandomReclaimableResourcesInRadius(pos, range, energy, energyAmount, metal, metalAmount));
}
