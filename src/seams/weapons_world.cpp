#include <stddef.h>
#include <string.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/features.h"
#include "../../interfaces/weapons.h"
#include "../../interfaces/services.h"
#include "../game/weapons_events.h"
#include "weapons_world.h"


void WeaponsInbox(const void* event)
{
    const WeaponsEvent* e = static_cast<const WeaponsEvent*>(event);
    HarnessWeaponsEvent(e, g_seams.weapons->Apply(e));
}

void WeaponsOnFeaturesAreaDamage(const void* event)
{
    const FeaturesEvent* effect = static_cast<const FeaturesEvent*>(event);
    WeaponsEvent e = WeaponsEventOf(WEAPONS_AREA_DAMAGE);
    e.value = effect->value;
    memcpy(e.pos, effect->pos, sizeof(e.pos));
    WeaponsInbox(&e);
}
