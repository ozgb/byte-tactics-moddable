#include <string.h>

#include "../../interfaces/seams.h"
#include "../util/engine_services.h"
#include "../../interfaces/weapons.h"
#include "../../interfaces/wind_meteors.h"
#include "../util/vec3.h"
#include "../weapons/weapon_def.h"
#include "../game/game.h"
#include "../game/weapons_events.h"
#include "weapons_world.h"
#include "wind_meteors_world.h"

#include "../../link/globals.h"

void WindMeteorsInbox(const void* event, unsigned int size)
{
    const WindMeteorsEventHeader* e = static_cast<const WindMeteorsEventHeader*>(event);
    g_seams.wind_meteors->Apply(e, size);
    HarnessWindMeteorsEvent(event, size);
}

void WeaponsOnMeteorSpawn(const void* event, unsigned int size)
{
    if (size < sizeof(WindMeteorsSpawn))
        return;
    const WindMeteorsSpawn* effect = static_cast<const WindMeteorsSpawn*>(event);
    WeaponsSpawn e;
    e.h = WeaponsHeaderOf(WEAPONS_SPAWN);
    e.weapon = effect->weapon;
    memcpy(e.pos, effect->pos, sizeof(e.pos));
    memcpy(e.vel, effect->vel, sizeof(e.vel));
    e.announce = effect->announce;
    WeaponsInbox(&e, sizeof(e));
}
