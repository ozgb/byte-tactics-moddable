#include <stddef.h>

#pragma pack(push, 1)
#include "../units/unit.h"
#pragma pack(pop)

#include "../../interfaces/seams.h"
#include "../../interfaces/scripts.h"
#include "../../interfaces/weapons.h"
#include "../../interfaces/orders.h"
#include "../../interfaces/movement.h"
#include "../../interfaces/economy.h"
#include "../../interfaces/features.h"
#include "../../interfaces/targeting.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"
#include "../game/scripts_events.h"
#include "scripts_world.h"

#include "../../link/globals.h"

void ScriptsInbox(const void* event, unsigned int size)
{
    const ScriptsEventHeader* e = static_cast<const ScriptsEventHeader*>(event);
    HarnessScriptsEvent(e, size, g_seams.scripts->Apply(e, size));
}

static const char* WeaponsScriptName(int script)
{
    switch (script) {
    case WEAPONS_SCRIPT_FIRE_PRIMARY:
    case WEAPONS_SCRIPT_FIRE_SECONDARY:
    case WEAPONS_SCRIPT_FIRE_TERTIARY:
        return g_fireScriptNames[script - WEAPONS_SCRIPT_FIRE_PRIMARY];
    case WEAPONS_SCRIPT_AIM_PRIMARY:
    case WEAPONS_SCRIPT_AIM_SECONDARY:
    case WEAPONS_SCRIPT_AIM_TERTIARY:
        return g_aimScriptNames[script - WEAPONS_SCRIPT_AIM_PRIMARY];
    case WEAPONS_SCRIPT_ROCK_UNIT:
        return "RockUnit";
    case WEAPONS_SCRIPT_SET_MAX_RELOAD_TIME:
        return "SetMaxReloadTime";
    case WEAPONS_SCRIPT_HIT_BY_WEAPON:
        return g_hitByWeaponScriptName;
    case WEAPONS_SCRIPT_TAKE_DAMAGE:
        return g_takeDamageScriptName;
    }
    return "";
}

static const char* MovementScriptName(int script)
{
    switch (script) {
    case MOVEMENT_SCRIPT_START_MOVING:
        return "StartMoving";
    case MOVEMENT_SCRIPT_STOP_MOVING:
        return "StopMoving";
    case MOVEMENT_SCRIPT_MOVE_RATE_1:
        return "MoveRate1";
    case MOVEMENT_SCRIPT_MOVE_RATE_2:
        return "MoveRate2";
    case MOVEMENT_SCRIPT_MOVE_RATE_3:
        return "MoveRate3";
    }
    return nullptr;
}

static void ScriptsWorldStartOf(unsigned int unit, int form, const char* name, int index, unsigned int callback, int update, int count, const int* args)
{
    ScriptsStart e = ScriptsStartOf<Unit>(nullptr, form, name, index, callback, update, count, args[0], args[1], args[2], args[3]);
    e.h.unit = unit;
    ScriptsInbox(&e, sizeof(e));
}

void ScriptsOnWeaponsStartScript(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsStartScript))
        return;
    const WeaponsStartScript* effect = static_cast<const WeaponsStartScript*>(event);
    ScriptsWorldStartOf(effect->h.unit, effect->withArgs == 1 ? SCRIPTS_START_BY_NAME_WITH_ARGS : SCRIPTS_START_BY_NAME, WeaponsScriptName(effect->script), 0, effect->callback, effect->update, effect->count, effect->args);
}

void ScriptsOnWeaponsDetachPiece(const void* event, unsigned int size)
{
    if (size < sizeof(WeaponsDetachPiece))
        return;
    const WeaponsDetachPiece* effect = static_cast<const WeaponsDetachPiece*>(event);
    ScriptsDetachPiece e;
    e.h = ScriptsHeaderOf(SCRIPTS_DETACH_PIECE);
    e.h.unit = effect->h.unit;
    e.piece = effect->piece;
    e.pose = effect->pose;
    ScriptsInbox(&e, sizeof(e));
}

void ScriptsOnOrdersStartScript(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersStartScript))
        return;
    const OrdersStartScript* effect = static_cast<const OrdersStartScript*>(event);
    int form = SCRIPTS_START_BY_NAME;
    if (effect->form == ORDERS_SCRIPT_BY_NAME_WITH_ARGS)
        form = SCRIPTS_START_BY_NAME_WITH_ARGS;
    else if (effect->form == ORDERS_SCRIPT_BY_INDEX_WITH_ARGS)
        form = SCRIPTS_START_BY_INDEX_WITH_ARGS;
    ScriptsWorldStartOf(effect->h.unit, form, effect->name, effect->index, effect->callback, effect->update, effect->count, effect->args);
}

void ScriptsOnMovementStartScript(const void* event, unsigned int size)
{
    if (size < sizeof(MovementStartScript))
        return;
    const MovementStartScript* effect = static_cast<const MovementStartScript*>(event);
    const char* name = MovementScriptName(effect->script);
    if (name == nullptr)
        return;
    static const int kNoArgs[4] = {0, 0, 0, 0};
    ScriptsWorldStartOf(effect->h.unit, SCRIPTS_START_BY_NAME, name, 0, 0, 1, 0, kNoArgs);
}

void ScriptsOnEconomyExtractor(const void* event, unsigned int size)
{
    if (size < sizeof(EconomyExtractor))
        return;
    const EconomyExtractor* effect = static_cast<const EconomyExtractor*>(event);
    Ref<Unit> ref;
    ref.offset = effect->h.unit;
    Unit* unit = ref.get();
    if (unit->script != nullptr) {
        const int args[4] = {effect->metal, 0, 0, 0};
        ScriptsWorldStartOf(effect->h.unit, SCRIPTS_START_BY_NAME_WITH_ARGS, "SetSpeed", 0, 0, 0, 1, args);
    }
}

void ScriptsOnFeaturesFreeState(const void* event, unsigned int size)
{
    if (size < sizeof(FeaturesFreeState))
        return;
    const FeaturesFreeState* effect = static_cast<const FeaturesFreeState*>(event);
    ScriptsState e;
    e.h = ScriptsHeaderOf(SCRIPTS_FREE_STATE);
    e.state = effect->state;
    e.feature = 0;
    ScriptsInbox(&e, sizeof(e));
}

void ScriptsOnFeaturesInitState(const void* event, unsigned int size)
{
    if (size < sizeof(FeaturesInitState))
        return;
    const FeaturesInitState* effect = static_cast<const FeaturesInitState*>(event);
    ScriptsState e;
    e.h = ScriptsHeaderOf(SCRIPTS_INIT_STATE);
    e.state = effect->state;
    e.feature = effect->feature;
    ScriptsInbox(&e, sizeof(e));
}

void ScriptsOnTargetingCleared(const void* event, unsigned int size)
{
    if (size < sizeof(TargetingEventHeader))
        return;
    const TargetingEventHeader* effect = static_cast<const TargetingEventHeader*>(event);
    const int args[4] = {effect->weapon, 0, 0, 0};
    ScriptsWorldStartOf(effect->unit, SCRIPTS_START_BY_NAME_WITH_ARGS, "TargetCleared", 0, 0, 0, 1, args);
}
