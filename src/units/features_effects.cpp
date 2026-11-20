#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/features.h"
#include "../../interfaces/ref.h"
#include "../graphics/model_types.h"
#include "../map/feature.h"
#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)
#include "features_effects.h"

#include "../../link/globals.h"

static ModelState* StateOfHandle(unsigned int handle)
{
    Ref<ModelState> ref;
    ref.offset = handle;
    return ref.get();
}

// TODO(#986): route FREE_STATE and INIT_STATE to SCHED_SCRIPTS
void UnitsOnFeaturesFreeState(const void* event)
{
    const FeaturesFreeState* effect = static_cast<const FeaturesFreeState*>(event);
    g_seams.scripts->FreeObjectState(StateOfHandle(effect->state));
}

void UnitsOnFeaturesInitState(const void* event)
{
    const FeaturesInitState* effect = static_cast<const FeaturesInitState*>(event);
    g_seams.scripts->InitObjectState(StateOfHandle(effect->state), (Object3do*)g_game->features[effect->feature].object);
}
