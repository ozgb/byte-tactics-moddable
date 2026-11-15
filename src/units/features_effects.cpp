#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/features.h"
#include "../../interfaces/ref.h"
#include "../state/block.h"
#include "../util/vec3.h"
#include "../graphics/model_types.h"
#include "../map/feature.h"
#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)
#include "object_state.h"
#include "features_effects.h"

#include "../../link/globals.h"

static ModelState* StateOfHandle(unsigned int handle)
{
    Ref<ModelState> ref;
    ref.offset = handle;
    return ref.get();
}

void UnitsOnFeaturesFreeState(const void* event)
{
    const FeaturesEvent* effect = static_cast<const FeaturesEvent*>(event);
    g_seams.units->FreeObjectState(StateOfHandle(effect->state));
}

void UnitsOnFeaturesInitState(const void* event)
{
    const FeaturesEvent* effect = static_cast<const FeaturesEvent*>(event);
    InitObjectState(StateOfHandle(effect->state), (Object3do*)g_game->features[effect->feature].object);
}
