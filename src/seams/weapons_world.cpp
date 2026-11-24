#include <stddef.h>
#include <string.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/features.h"
#include "../../interfaces/weapons.h"
#include "../util/engine_services.h"
#include "../game/weapons_events.h"
#include "weapons_world.h"


void WeaponsInbox(const void* event, unsigned int size)
{
    const WeaponsEventHeader* e = static_cast<const WeaponsEventHeader*>(event);
    HarnessWeaponsEvent(e, size, g_seams.weapons->Apply(e, size));
}

void WeaponsOnFeaturesAreaDamage(const void* event, unsigned int size)
{
    if (size < sizeof(FeaturesAreaDamage))
        return;
    const FeaturesAreaDamage* effect = static_cast<const FeaturesAreaDamage*>(event);
    WeaponsAreaDamage e;
    e.h = WeaponsHeaderOf(WEAPONS_AREA_DAMAGE);
    e.weapon = effect->weapon;
    memcpy(e.pos, effect->pos, sizeof(e.pos));
    WeaponsInbox(&e, sizeof(e));
}
