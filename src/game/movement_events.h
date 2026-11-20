#ifndef GAME_MOVEMENT_EVENTS_H
#define GAME_MOVEMENT_EVENTS_H

#include <string.h>

#include "../../interfaces/movement.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"

struct Unit;

inline MovementEventHeader MovementHeaderOf(int kind)
{
    MovementEventHeader header;
    header.kind = kind;
    header.unit = 0;
    return header;
}

template <class U>
inline MovementEventHeader MovementHeaderOf(int kind, U* unit)
{
    MovementEventHeader header = MovementHeaderOf(kind);
    header.unit = Ref<U>(unit).offset;
    return header;
}

inline unsigned int MovementEventSize(int kind)
{
    switch (kind) {
    case MOVEMENT_LOAD:
        return sizeof(MovementLoad);
    case MOVEMENT_SET_MODE:
    case MOVEMENT_SET_FLIGHT_MODE:
        return sizeof(MovementSetMode);
    case MOVEMENT_SET_SPEED:
        return sizeof(MovementSetSpeed);
    case MOVEMENT_SET_TARGET:
    case MOVEMENT_DESTROY_TARGET:
        return sizeof(MovementTarget);
    case MOVEMENT_SET_ALTITUDE:
    case MOVEMENT_SET_APPROACH_RADIUS:
    case MOVEMENT_SET_HEADING:
        return sizeof(MovementTargetValue);
    case MOVEMENT_EFFECT_ORDER_FLAGS:
        return sizeof(MovementOrderFlags);
    case MOVEMENT_EFFECT_TURN:
        return sizeof(MovementTurn);
    case MOVEMENT_EFFECT_TILT:
        return sizeof(MovementTilt);
    case MOVEMENT_EFFECT_ANGLES:
        return sizeof(MovementAngles);
    case MOVEMENT_EFFECT_MOVE:
        return sizeof(MovementMove);
    case MOVEMENT_EFFECT_MOVED:
        return sizeof(MovementMoved);
    case MOVEMENT_EFFECT_CHANGE_CELL:
        return sizeof(MovementChangeCell);
    case MOVEMENT_EFFECT_MOVE_RATE:
        return sizeof(MovementMoveRate);
    case MOVEMENT_EFFECT_SET_POSITION:
        return sizeof(MovementSetPosition);
    case MOVEMENT_EFFECT_SET_STATE_BITS:
        return sizeof(MovementSetStateBits);
    case MOVEMENT_EFFECT_START_SCRIPT:
        return sizeof(MovementStartScript);
    }
    return sizeof(MovementEventHeader);
}

inline void MovementPublishEffect(const void* effect, unsigned int size)
{
    HarnessMovementEvent(static_cast<const MovementEventHeader*>(effect), size, 1);
    SchedPublish(SCHED_MOVEMENT, effect, size);
}

inline void MovementPublishUnit(int outbox, int kind, Unit* unit)
{
    MovementEventHeader event = MovementHeaderOf(kind, unit);
    SchedPublish(outbox, &event, sizeof(event));
}

inline void MovementPublishLoad(int outbox, Unit* unit, const MovementMotionState* state)
{
    MovementLoad event;
    event.h = MovementHeaderOf(MOVEMENT_LOAD, unit);
    event.state = *state;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void MovementPublishSetMode(int outbox, int kind, Unit* unit, int mode)
{
    MovementSetMode event;
    event.h = MovementHeaderOf(kind, unit);
    event.mode = mode;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void MovementPublishSetSpeed(int outbox, Unit* unit, int speed)
{
    MovementSetSpeed event;
    event.h = MovementHeaderOf(MOVEMENT_SET_SPEED, unit);
    event.speed = speed;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void MovementPublishTarget(int outbox, int kind, Unit* unit, unsigned int target)
{
    MovementTarget event;
    event.h = MovementHeaderOf(kind, unit);
    event.target = target;
    SchedPublish(outbox, &event, sizeof(event));
}

inline void MovementPublishTargetValue(int outbox, int kind, Unit* unit, unsigned int target, int value)
{
    MovementTargetValue event;
    event.h = MovementHeaderOf(kind, unit);
    event.target = target;
    event.value = value;
    SchedPublish(outbox, &event, sizeof(event));
}

#endif
