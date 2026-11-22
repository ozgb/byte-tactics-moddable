#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/state_writer.h"
#include "../state/state_fields.h"
#include "../../interfaces/ref.h"
#include "../state/block.h"
#include "../network/player.h"
#include "../game/game.h"
#include "../util/vec3.h"
#include "../units/unit.h"
#include "../orders/order.h"
#include "ai_search_goal.h"
#include "order_fx.h"
#include "path_order.h"
#include "air_maneuver_order.h"
#include "unit_motion.h"
#include "../game/movement_events.h"
#include "../game/movement_event_size.h"
#include "../../interfaces/movement.h"
#include "../../link/globals.h"
#include "movement_seam.h"

extern void* g_patrolGoalVtable[];
extern void* g_aiSearchGoalVtable[];
extern void* g_packedGoalVtable[];
extern void* g_liteGoalVtable[];
extern void* g_packedPosGoalVtable[];

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

static int KnownKind(int kind)
{
    switch (kind) {
    case MOVEMENT_NEW:
    case MOVEMENT_DESTROY:
    case MOVEMENT_LOAD:
    case MOVEMENT_SET_MODE:
    case MOVEMENT_SET_SPEED:
    case MOVEMENT_SET_TARGET:
    case MOVEMENT_DESTROY_TARGET:
    case MOVEMENT_SET_FLIGHT_MODE:
    case MOVEMENT_SET_ALTITUDE:
    case MOVEMENT_SET_APPROACH_RADIUS:
    case MOVEMENT_SET_HEADING:
        return 1;
    }
    return 0;
}

static int MovementEventValid(const MovementEventHeader* e, Unit* unit, OrderFx* target)
{
    if (!KnownKind(e->kind))
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
static void LoadMotionState(UnitMotion* motion, const MovementSavedState* state)
{
    motion->velocity.x = state->velocity[0];
    motion->velocity.y = state->velocity[1];
    motion->velocity.z = state->velocity[2];
    motion->p2.x = state->p2[0];
    motion->p2.y = state->p2[1];
    motion->p2.z = state->p2[2];
    motion->speed = state->speed;
    motion->turn = (short)state->turn;
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
    if (!MovementEventValid(e, unit, target))
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
        LoadMotionState(unit->motion, &reinterpret_cast<const MovementLoadState*>(e)->state);
        break;
    case MOVEMENT_SET_MODE:
        unit->motion->mode = reinterpret_cast<const MovementSetMode*>(e)->mode;
        break;
    case MOVEMENT_SET_SPEED:
        unit->motion->speed = reinterpret_cast<const MovementSetSpeed*>(e)->speed;
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

void __stdcall MovementGetMotionState(Unit* unit, MovementSavedState* out)
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

static const StateField kPatrolGoal[] = {
    {"goal.target", 4, 4, F_HEAP, 0x0},
    {"goal.owner", 8, 4, F_UNIT, 0x0},
    {"goal.count", 24, 4, F_INT, 0x0},
};

static const StateField kAiSearchGoal[] = {
    {"goal.target", 4, 4, F_HEAP, 0x0},
    {"goal.owner", 8, 4, F_UNIT, 0x0},
    {"goal.count", 92, 4, F_INT, 0x0},
    {"goal.repathClaimTick", 96, 4, F_INT, 0x0},
    {"goal.flags", 100, 1, F_INT, 0xb},
};

static const StateField kPackedGoal[] = {
    {"goal.target", 4, 4, F_HEAP, 0x0},
    {"goal.owner", 8, 4, F_UNIT, 0x0},
    {"goal.pos.x", 12, 4, F_INT, 0x0},
    {"goal.pos.y", 16, 4, F_INT, 0x0},
    {"goal.pos.z", 20, 4, F_INT, 0x0},
    {"goal.vel.x", 24, 4, F_INT, 0x0},
    {"goal.vel.y", 28, 4, F_INT, 0x0},
    {"goal.vel.z", 32, 4, F_INT, 0x0},
    {"goal.heading", 36, 2, F_INT, 0x0},
};

static const StateField kLiteGoalState[] = {
    {"goal.netDirty", 39, 1, F_INT, 0x7},
};

enum GoalClass { GOAL_NONE, GOAL_PATROL, GOAL_AISEARCH, GOAL_PACKED, GOAL_LITE, GOAL_PACKEDPOS, GOAL_UNKNOWN };

static GoalClass ClassOfGoal(const unsigned char* goal)
{
    void** vptr = *(void***)goal;
    if (vptr == g_patrolGoalVtable)
        return GOAL_PATROL;
    if (vptr == g_aiSearchGoalVtable)
        return GOAL_AISEARCH;
    if (vptr == g_packedGoalVtable)
        return GOAL_PACKED;
    if (vptr == g_liteGoalVtable)
        return GOAL_LITE;
    if (vptr == g_packedPosGoalVtable)
        return GOAL_PACKEDPOS;
    return GOAL_UNKNOWN;
}

static void WritePoints(const StateWriter* w, const unsigned char* goal, unsigned int countOffset, unsigned int limit)
{
    unsigned int n = ReadStateWord(goal + countOffset, 4);
    if (n > limit)
        n = limit;
    w->Bytes("goal.points", 0, goal + 0xc, (int)(n * 4));
}

static void WriteGoal(const StateWriter* w, const unsigned char* goal)
{
    GoalClass cls = goal == nullptr ? GOAL_NONE : ClassOfGoal(goal);
    w->Int("goal.class", 0, 4, (unsigned int)cls);
    switch (cls) {
    case GOAL_PATROL:
        WriteStateFields(w, goal, STATE_FIELDS(kPatrolGoal), 0);
        WritePoints(w, goal, 0x18, 3);
        break;
    case GOAL_AISEARCH:
        WriteStateFields(w, goal, STATE_FIELDS(kAiSearchGoal), 0);
        WritePoints(w, goal, 0x5c, 20);
        break;
    case GOAL_PACKED:
    case GOAL_PACKEDPOS:
        WriteStateFields(w, goal, STATE_FIELDS(kPackedGoal), 0);
        break;
    case GOAL_LITE:
        WriteStateFields(w, goal, STATE_FIELDS(kPackedGoal), 0);
        WriteStateFields(w, goal, STATE_FIELDS(kLiteGoalState), 0);
        break;
    default:
        break;
    }
}

void __stdcall MovementWriteState(const StateWriter* w)
{
    Unit* units = g_game->units.get();
    char name[64];
    for (unsigned int i = 0; units != nullptr && i < g_game->poolCount; i++) {
        Unit* u = &units[i];
        if (u->unitDefIndex == 0 || u->motion.offset == 0)
            continue;
        UnitMotion* motion = u->motion.get();
        sprintf(name, "movementState.units[%u]", i);
        w->Begin(53, name);
        w->Int("motion.p2.x", 0, 4, (unsigned int)motion->p2.x);
        w->Int("motion.p2.y", 0, 4, (unsigned int)motion->p2.y);
        w->Int("motion.p2.z", 0, 4, (unsigned int)motion->p2.z);
        w->Int("motion.turn", 0, 2, (unsigned short)motion->turn);
        WriteGoal(w, (const unsigned char*)motion->obj.get());
    }
}
