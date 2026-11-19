#include <string.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/services.h"
#include "../../interfaces/weapons.h"
#include "../../interfaces/wind_meteors.h"
#include "../util/vec3.h"
#include "../weapons/weapon_def.h"
#include "../game/game.h"
#include "../harness/replay.h"
#include "../harness/replay_services.h"
#include "../game/weapons_events.h"
#include "weapons_world.h"
#include "wind_meteors_world.h"

#include "../../link/globals.h"

void PublishWindMeteors(int kind, const MeteorShowerParams* params)
{
    WindMeteorsEvent event;
    memset(&event, 0, sizeof event);
    event.kind = kind;
    if (params != nullptr)
        event.params = *params;
    SchedPublish(SCHED_ENGINE, &event, sizeof event);
}

void WindMeteorsInbox(const void* event)
{
    g_seams.wind_meteors->Apply(static_cast<const WindMeteorsEvent*>(event));
    HarnessWindMeteorsEvent(event, sizeof(WindMeteorsEvent));
}

void __stdcall WindMeteorsEmit(const WindMeteorsEffect* effect)
{
    ReplayImport call(RI_WIND_METEORS_EMIT);
    call.Ptr(effect);
    HarnessWindMeteorsEvent(effect, sizeof *effect);
    SchedPublish(SCHED_WIND_METEORS, effect, sizeof *effect);
}

void WeaponsOnMeteorSpawn(const void* event)
{
    const WindMeteorsEffect* effect = static_cast<const WindMeteorsEffect*>(event);
    WeaponsEvent e = WeaponsEventOf(WEAPONS_SPAWN);
    e.value = effect->weapon;
    e.pos[0] = effect->x;
    e.pos[1] = effect->y;
    e.pos[2] = effect->z;
    e.vel[0] = effect->vx;
    e.vel[1] = effect->vy;
    e.vel[2] = effect->vz;
    e.extra = effect->announce;
    WeaponsInbox(&e);
}
