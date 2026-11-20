#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/movement.h"
#include "../../interfaces/units.h"
#include "../../interfaces/ref.h"
#include "../util/vec3.h"
#include "unit.h"
#include "movement_effects.h"

static Unit* UnitOfOffset(unsigned int offset)
{
    Ref<Unit> ref;
    ref.offset = offset;
    return ref.get();
}

void UnitsOnMovementTurn(const void* event, unsigned int size)
{
    if (size < sizeof(MovementTurn))
        return;
    const MovementTurn* e = static_cast<const MovementTurn*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    unit->angles.heading = (unsigned short)(unit->angles.heading + e->delta);
    unit->moved = 1;
}

void UnitsOnMovementTilt(const void* event, unsigned int size)
{
    if (size < sizeof(MovementTilt))
        return;
    const MovementTilt* e = static_cast<const MovementTilt*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    unit->angles.bank = (short)e->bank;
    unit->angles.pitch = (short)e->pitch;
}

void UnitsOnMovementAngles(const void* event, unsigned int size)
{
    if (size < sizeof(MovementAngles))
        return;
    const MovementAngles* e = static_cast<const MovementAngles*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    unit->angles.bank = (short)e->bank;
    unit->angles.heading = (unsigned short)e->heading;
    unit->angles.pitch = (short)e->pitch;
}

void UnitsOnMovementMove(const void* event, unsigned int size)
{
    if (size < sizeof(MovementMove))
        return;
    const MovementMove* e = static_cast<const MovementMove*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    unit->pos.x = e->pos[0];
    unit->pos.y = e->pos[1];
    unit->pos.z = e->pos[2];
    unit->moved = 1;
}

void UnitsOnMovementMoved(const void* event, unsigned int size)
{
    if (size < sizeof(MovementMoved))
        return;
    const MovementMoved* e = static_cast<const MovementMoved*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    unit->moved = e->moved;
}

void UnitsOnMovementChangeCell(const void* event, unsigned int size)
{
    if (size < sizeof(MovementChangeCell))
        return;
    const MovementChangeCell* e = static_cast<const MovementChangeCell*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    unit->pos.x = e->pos[0];
    unit->pos.y = e->pos[1];
    unit->pos.z = e->pos[2];
    unit->cell.x = (short)e->cell[0];
    unit->cell.y = (short)e->cell[1];
    unit->mode = (short)e->mode;
}

void UnitsOnMovementMoveRate(const void* event, unsigned int size)
{
    if (size < sizeof(MovementMoveRate))
        return;
    const MovementMoveRate* e = static_cast<const MovementMoveRate*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    unit->flags = (unit->flags & 0xfffffff3) | ((e->rate & 3) << 2);
}

void UnitsOnMovementSetPosition(const void* event, unsigned int size)
{
    if (size < sizeof(MovementSetPosition))
        return;
    const MovementSetPosition* e = static_cast<const MovementSetPosition*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    Vec3 pos;
    pos.x = e->pos[0];
    pos.y = e->pos[1];
    pos.z = e->pos[2];
    g_seams.units->SetUnitPosition(unit, pos, e->mode);
}

void UnitsOnMovementSetStateBits(const void* event, unsigned int size)
{
    if (size < sizeof(MovementSetStateBits))
        return;
    const MovementSetStateBits* e = static_cast<const MovementSetStateBits*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    g_seams.units->SetStateBits(unit, e->mask, e->set);
}

void UnitsOnMovementSfxOccupy(const void* event, unsigned int size)
{
    if (size < sizeof(MovementEventHeader))
        return;
    const MovementEventHeader* e = static_cast<const MovementEventHeader*>(event);
    Unit* unit = UnitOfOffset(e->unit);
    if (unit == nullptr)
        return;
    g_seams.units->UpdateSfxOccupy(unit->motion, 0, unit);
}
