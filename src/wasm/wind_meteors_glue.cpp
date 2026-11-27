#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

#include "../game/los_helpers.h"
#include "../../interfaces/wind_meteors.h"
#include "module_glue.h"

namespace {

const char* CheckSpawn(const void* effect)
{
    const WindMeteorsSpawn* spawn = static_cast<const WindMeteorsSpawn*>(effect);
    if (spawn->weapon < -1 || spawn->weapon >= (int)(sizeof(g_game->weapons) / sizeof(g_game->weapons[0])))
        return GlueImportError("Emit", "the weapon slot is outside the table of weapons");
    return nullptr;
}

}

const GlueEffectHook g_glueEffectHooks[] = {
    {WIND_METEORS_EFFECT_SPAWN, CheckSpawn},
};
const unsigned g_glueEffectHookCount = sizeof g_glueEffectHooks / sizeof g_glueEffectHooks[0];
