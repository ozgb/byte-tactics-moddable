#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/orders.h"
#include "../../interfaces/units.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"
#include "../util/vec3.h"
#include "../util/angles.h"
#include "unit.h"
#include "unit_ref.h"
#include "orders_effects.h"

static Unit* UnitOfOffset(unsigned int offset)
{
    Ref<Unit> ref;
    ref.offset = offset;
    return ref.get();
}

void UnitsOnOrdersSetStateBits(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersSetStateBits))
        return;
    const OrdersSetStateBits* e = static_cast<const OrdersSetStateBits*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    g_seams.units->SetStateBits(unit, e->mask, e->set);
}

void UnitsOnOrdersSetPosition(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersSetPosition))
        return;
    const OrdersSetPosition* e = static_cast<const OrdersSetPosition*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    Vec3 pos;
    pos.x = e->pos[0];
    pos.y = e->pos[1];
    pos.z = e->pos[2];
    g_seams.units->SetUnitPosition(unit, pos, e->mode);
}

void UnitsOnOrdersAttachToPiece(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersAttachToPiece))
        return;
    const OrdersAttachToPiece* e = static_cast<const OrdersAttachToPiece*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    g_seams.units->AttachUnitToPiece(unit, UnitOfOffset(e->target), (char)e->piece, (char)e->mode);
}

void UnitsOnOrdersFinishConstruction(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersFinishConstruction))
        return;
    const OrdersFinishConstruction* e = static_cast<const OrdersFinishConstruction*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    g_seams.units->FinishConstruction(UnitOfOffset(e->builder), unit);
}

void UnitsOnOrdersBuildDecay(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersBuildDecay))
        return;
    const OrdersBuildDecay* e = static_cast<const OrdersBuildDecay*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    g_seams.units->ApplyUnfinishedBuildDecay(unit, e->ticks);
}

void UnitsOnOrdersRepair(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersRepair))
        return;
    const OrdersRepair* e = static_cast<const OrdersRepair*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    g_seams.units->AddRepairProgress(UnitOfOffset(e->builder), unit, e->amount);
}

void UnitsOnOrdersUnlink(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersUnlink))
        return;
    const OrdersUnlink* e = static_cast<const OrdersUnlink*>(event);
    Ref<UnitRef> ref;
    ref.offset = e->ref;
    if (ref.get() == nullptr)
        return;
    g_seams.units->UnitRefUnlink(ref.get());
}

void UnitsOnOrdersBusy(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersBusy))
        return;
    const OrdersBusy* e = static_cast<const OrdersBusy*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    unit->workTime = e->until;
}

void UnitsOnOrdersSetFlags(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersSetFlags))
        return;
    const OrdersSetFlags* e = static_cast<const OrdersSetFlags*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    unit->flags = (unit->flags & ~e->clear) | e->set;
}

void UnitsOnOrdersClearNetDirty(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersClearNetDirty))
        return;
    const OrdersClearNetDirty* e = static_cast<const OrdersClearNetDirty*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    unit->netDirtyFlags &= ~e->mask;
}

void UnitsOnOrdersSetStance(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersSetStance))
        return;
    const OrdersSetStance* e = static_cast<const OrdersSetStance*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    if (e->fields & ORDERS_STANCE_MOVE)
        unit->moveOrder = e->moveOrder;
    if (e->fields & ORDERS_STANCE_FIRE)
        unit->fireOrder = e->fireOrder;
    if (e->fields & ORDERS_STANCE_GROUP)
        unit->group = e->group;
}

void UnitsOnOrdersSetAngles(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersSetAngles))
        return;
    const OrdersSetAngles* e = static_cast<const OrdersSetAngles*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    Angles16 angles;
    angles.bank = (short)e->bank;
    angles.heading = (unsigned short)e->heading;
    angles.pitch = (short)e->pitch;
    unit->angles = angles;
}

void UnitsOnOrdersSetBuildState(const void* event, unsigned int size)
{
    if (size < sizeof(OrdersSetBuildState))
        return;
    const OrdersSetBuildState* e = static_cast<const OrdersSetBuildState*>(event);
    Unit* unit = UnitOfOffset(e->h.unit);
    if (unit == nullptr)
        return;
    unit->buildLeft = e->buildLeft;
    unit->health = (short)e->health;
}
