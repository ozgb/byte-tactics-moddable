#ifndef GAME_MOVEMENT_EVENTS_H
#define GAME_MOVEMENT_EVENTS_H

#include <string.h>

#include "../../interfaces/movement.h"
#include "movement_event_size.h"
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

inline void MovementPublishOrderFlags(unsigned int order, unsigned int unit, unsigned int flags)
{
    MovementOrderFlags effect;
    effect.h = MovementHeaderOf(MOVEMENT_EFFECT_ORDER_FLAGS);
    effect.h.unit = unit;
    effect.order = order;
    effect.flags = flags;
    MovementPublishEffect(&effect, sizeof(effect));
}

template <class U>
inline void MovementPublishTurn(U* unit, int delta)
{
    MovementTurn effect;
    effect.h = MovementHeaderOf(MOVEMENT_EFFECT_TURN, unit);
    effect.delta = delta;
    MovementPublishEffect(&effect, sizeof(effect));
}

template <class U>
inline void MovementPublishTilt(U* unit, int bank, int pitch)
{
    MovementTilt effect;
    effect.h = MovementHeaderOf(MOVEMENT_EFFECT_TILT, unit);
    effect.bank = bank;
    effect.pitch = pitch;
    MovementPublishEffect(&effect, sizeof(effect));
}

template <class U>
inline void MovementPublishAngles(U* unit, int bank, int heading, int pitch)
{
    MovementAngles effect;
    effect.h = MovementHeaderOf(MOVEMENT_EFFECT_ANGLES, unit);
    effect.bank = bank;
    effect.heading = heading;
    effect.pitch = pitch;
    MovementPublishEffect(&effect, sizeof(effect));
}

template <class U>
inline void MovementPublishMove(U* unit, int x, int y, int z)
{
    MovementMove effect;
    effect.h = MovementHeaderOf(MOVEMENT_EFFECT_MOVE, unit);
    effect.pos[0] = x;
    effect.pos[1] = y;
    effect.pos[2] = z;
    MovementPublishEffect(&effect, sizeof(effect));
}

template <class U>
inline void MovementPublishMoved(U* unit, int moved)
{
    MovementMoved effect;
    effect.h = MovementHeaderOf(MOVEMENT_EFFECT_MOVED, unit);
    effect.moved = moved;
    MovementPublishEffect(&effect, sizeof(effect));
}

template <class U>
inline void MovementPublishChangeCell(U* unit, int x, int y, int z, int cellX, int cellY, int mode)
{
    MovementChangeCell effect;
    effect.h = MovementHeaderOf(MOVEMENT_EFFECT_CHANGE_CELL, unit);
    effect.pos[0] = x;
    effect.pos[1] = y;
    effect.pos[2] = z;
    effect.cell[0] = cellX;
    effect.cell[1] = cellY;
    effect.mode = mode;
    MovementPublishEffect(&effect, sizeof(effect));
}

template <class U>
inline void MovementPublishMoveRate(U* unit, int rate)
{
    MovementMoveRate effect;
    effect.h = MovementHeaderOf(MOVEMENT_EFFECT_MOVE_RATE, unit);
    effect.rate = rate;
    MovementPublishEffect(&effect, sizeof(effect));
}

template <class U>
inline void MovementPublishSetPosition(U* unit, int x, int y, int z, int mode)
{
    MovementSetPosition effect;
    effect.h = MovementHeaderOf(MOVEMENT_EFFECT_SET_POSITION, unit);
    effect.pos[0] = x;
    effect.pos[1] = y;
    effect.pos[2] = z;
    effect.mode = mode;
    MovementPublishEffect(&effect, sizeof(effect));
}

template <class U>
inline void MovementPublishSetStateBits(U* unit, int mask, int set)
{
    MovementSetStateBits effect;
    effect.h = MovementHeaderOf(MOVEMENT_EFFECT_SET_STATE_BITS, unit);
    effect.mask = mask;
    effect.set = set;
    MovementPublishEffect(&effect, sizeof(effect));
}

template <class U>
inline void MovementPublishSfxOccupy(U* unit)
{
    MovementEventHeader effect = MovementHeaderOf(MOVEMENT_EFFECT_SFX_OCCUPY, unit);
    MovementPublishEffect(&effect, sizeof(effect));
}

template <class U>
inline void MovementPublishStartScript(U* unit, int script)
{
    MovementStartScript effect;
    effect.h = MovementHeaderOf(MOVEMENT_EFFECT_START_SCRIPT, unit);
    effect.script = script;
    MovementPublishEffect(&effect, sizeof(effect));
}

#endif
