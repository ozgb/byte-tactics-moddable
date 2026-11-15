#include "../../interfaces/seams.h"
#include "../../interfaces/services.h"
#include "../../interfaces/wind_meteors.h"
#include "../util/vec3.h"
#include "../weapons/weapon_def.h"
#include "../game/game.h"
#include "../harness/replay.h"
#include "../harness/replay_services.h"
#include "wind_meteors_world.h"

#include "../../link/globals.h"

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
    Vec3 pos;
    pos.x = effect->x;
    pos.y = effect->y;
    pos.z = effect->z;
    Vec3 vel;
    vel.x = effect->vx;
    vel.y = effect->vy;
    vel.z = effect->vz;
    WeaponDef* weapon = effect->weapon < 0 ? nullptr : &g_game->weapons[effect->weapon];
    g_seams.weapons->SpawnProjectile(weapon, &pos, &vel, effect->announce);
}
