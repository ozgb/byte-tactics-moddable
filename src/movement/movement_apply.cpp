#include "win_types.h"
#include <stddef.h>
#include <string.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/state_writer.h"
#include "../../interfaces/ref.h"
#include "../state/block.h"
#include "../network/player.h"
#include "../game/game.h"
#include "../files/files_bank.h"
#include "../util/vec3.h"
#include "../units/unit.h"
#include "../orders/order.h"
#include "ai_search_goal.h"
#include "order_fx.h"
#include "path_order.h"
#include "air_maneuver_order.h"
#include "approach_areas.h"
#include "unit_motion.h"
#include "../game/movement_events.h"
#include "../../link/globals.h"
#include "movement_seam.h"

static_assert(offsetof(UnitMotion, pathLockStamp) == 0x26, "pathLockStamp");

static Unit* UnitOfOffset(unsigned int offset)
{
    Ref<Unit> ref;
    ref.offset = offset;
    return ref.get();
}

static OrderFx* TargetOfOffset(unsigned int offset)
{
    Ref<OrderFx> ref;
    ref.offset = offset;
    return ref.get();
}

static int TargetKind(int kind)
{
    return kind == MOVEMENT_DESTROY_TARGET || kind == MOVEMENT_SET_ALTITUDE || kind == MOVEMENT_SET_APPROACH_RADIUS ||
           kind == MOVEMENT_SET_HEADING;
}

static int Valid(const MovementEventHeader* e, Unit* unit, OrderFx* target)
{
    if (e->kind < MOVEMENT_NEW || e->kind > MOVEMENT_SET_HEADING)
        return 0;
    if (TargetKind(e->kind))
        return target != nullptr;
    if (unit == nullptr)
        return 0;
    if (e->kind != MOVEMENT_NEW && unit->motion == nullptr)
        return 0;
    return 1;
}

// FUNCTION: 0x43de30
static void LoadMotionState(UnitMotion* motion, const MovementMotionState* state)
{
    motion->velocity.x = state->velocity[0];
    motion->velocity.y = state->velocity[1];
    motion->velocity.z = state->velocity[2];
    motion->p2.x = state->p2[0];
    motion->p2.y = state->p2[1];
    motion->p2.z = state->p2[2];
    motion->speed = state->speed;
    motion->turn = (short)state->turn;
    motion->pathLockStamp = state->pathLockStamp;
    motion->mode = state->mode;
    motion->flag = state->flag;
}

int __stdcall MovementApply(const MovementEventHeader* e, unsigned int size)
{
    if (size < sizeof(MovementEventHeader))
        return 0;
    if (size < MovementEventSize(e->kind))
        return 0;
    Unit* unit = UnitOfOffset(e->unit);
    OrderFx* target = nullptr;
    if (e->kind == MOVEMENT_SET_TARGET || TargetKind(e->kind))
        target = TargetOfOffset(reinterpret_cast<const MovementTarget*>(e)->target);
    if (!Valid(e, unit, target))
        return 0;
    switch (e->kind) {
    case MOVEMENT_NEW:
        unit->motion = new (kSim) UnitMotion(unit);
        break;
    case MOVEMENT_DESTROY: {
        UnitMotion* head = unit->motion;
        head->DestroyObject();
        operator delete(head);
        unit->motion = 0;
        break;
    }
    case MOVEMENT_LOAD:
        LoadMotionState(unit->motion, &reinterpret_cast<const MovementLoad*>(e)->state);
        break;
    case MOVEMENT_SET_MODE:
        unit->motion->mode = reinterpret_cast<const MovementSetMode*>(e)->mode;
        break;
    case MOVEMENT_SET_SPEED:
        unit->motion->speed = reinterpret_cast<const MovementSetSpeed*>(e)->speed;
        break;
    case MOVEMENT_PLACED:
        unit->motion->pathLockStamp = g_game->ticks;
        break;
    case MOVEMENT_SET_TARGET:
        ((PathGoal*)unit->motion->obj.get())->SetPathOrder(target);
        break;
    case MOVEMENT_DESTROY_TARGET:
        delete target;
        break;
    case MOVEMENT_SET_FLIGHT_MODE:
        unit->motion->SetFlightMode(unit, reinterpret_cast<const MovementSetMode*>(e)->mode);
        break;
    case MOVEMENT_SET_ALTITUDE: {
        int altitude = reinterpret_cast<const MovementTargetValue*>(e)->value;
        int type = target->GetType();
        if (type == 2)
            static_cast<PathOrder*>(target)->SetAltitude(altitude);
        else if (type == 3)
            static_cast<AirManeuverOrder*>(target)->SetAltitude(altitude);
        else
            return 0;
        break;
    }
    case MOVEMENT_SET_APPROACH_RADIUS:
        if (target->GetType() != 2)
            return 0;
        static_cast<PathOrder*>(target)->SetApproachRadius((short)reinterpret_cast<const MovementTargetValue*>(e)->value);
        break;
    case MOVEMENT_SET_HEADING:
        if (target->GetType() != 2)
            return 0;
        static_cast<PathOrder*>(target)->SetHeading((short)reinterpret_cast<const MovementTargetValue*>(e)->value);
        break;
    }
    return 1;
}

void __stdcall MovementUpdateMotion(Unit* unit)
{
    unit->motion->UpdateMotion(unit);
}

// FUNCTION: 0x43dd70
void __stdcall MovementGetMotionState(Unit* unit, MovementMotionState* out)
{
    UnitMotion* motion = unit->motion;
    if (motion == nullptr) {
        memset(out, 0, sizeof(*out));
        return;
    }
    out->velocity[0] = motion->velocity.x;
    out->velocity[1] = motion->velocity.y;
    out->velocity[2] = motion->velocity.z;
    out->p2[0] = motion->p2.x;
    out->p2[1] = motion->p2.y;
    out->p2[2] = motion->p2.z;
    out->speed = motion->speed;
    out->turn = motion->turn;
    out->pathLockStamp = motion->pathLockStamp;
    out->mode = motion->mode;
    out->flag = motion->flag;
}

void __stdcall MovementReadNetState(Unit* unit, BitReader* reader)
{
    unit->motion->ReadNetState(reader);
}

int __stdcall MovementHasNetState(Unit* unit)
{
    if (unit->motion == nullptr)
        return 0;
    return ((PathGoal*)unit->motion->player.get())->HasNetUnitState();
}

void __stdcall MovementWriteNetState(Unit* unit, BitWriter* stream)
{
    ((PathGoal*)unit->motion->player.get())->SerializeNetUnitState(stream);
}

unsigned int __stdcall MovementUnitTarget(Unit* unit)
{
    if (unit->motion == nullptr)
        return 0;
    return ((PathGoal*)unit->motion->obj.get())->target.offset;
}

void __stdcall MovementWriteState(const StateWriter*)
{
}
