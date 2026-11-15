#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/features.h"
#include "../util/vec3.h"
#include "weapon_def.h"
#include "../game/game.h"
#include "features_effects.h"

#include "../../link/globals.h"

void WeaponsOnFeaturesAreaDamage(const void* event)
{
    const FeaturesEvent* effect = static_cast<const FeaturesEvent*>(event);
    Vec3 pos;
    pos.x = effect->pos[0];
    pos.y = effect->pos[1];
    pos.z = effect->pos[2];
    g_seams.weapons->ApplyAreaDamageAt(&g_game->weapons[effect->value], &pos);
}
