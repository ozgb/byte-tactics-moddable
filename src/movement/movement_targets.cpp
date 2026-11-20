#include "win_types.h"
#include <stddef.h>

#include "../../interfaces/seams.h"
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
#include "../../link/globals.h"
#include "movement_seam.h"

static Order* OrderOfOffset(unsigned int offset)
{
    Ref<Order> ref;
    ref.offset = offset;
    return ref.get();
}

static OrderFx* TargetOfOffset(unsigned int offset)
{
    Ref<OrderFx> ref;
    ref.offset = offset;
    return ref.get();
}

static unsigned int OffsetOfTarget(OrderFx* target)
{
    return Ref<OrderFx>(target).offset;
}

unsigned int __stdcall MovementNewPathOrderAt(unsigned int order, const Vec3* pos)
{
    PathOrder* target = new (kSim) PathOrder(OrderOfOffset(order), *pos);
    return OffsetOfTarget(target);
}

unsigned int __stdcall MovementNewPathOrderToUnit(unsigned int order, Unit* unit)
{
    PathOrder* target = new (kSim) PathOrder(OrderOfOffset(order), unit);
    return OffsetOfTarget(target);
}

unsigned int __stdcall MovementNewPathOrderToUnitAt(unsigned int order, Unit* unit, const Vec3* pos)
{
    PathOrder* target = new (kSim) PathOrder(OrderOfOffset(order), unit, *pos);
    return OffsetOfTarget(target);
}

unsigned int __stdcall MovementNewPathOrderToUnitAtAngle(unsigned int order, Unit* unit, short angle)
{
    PathOrder* target = new (kSim) PathOrder(OrderOfOffset(order), unit, angle);
    return OffsetOfTarget(target);
}

unsigned int __stdcall MovementNewAirManeuverOrder(unsigned int order, const Vec3* from, const Vec3* to)
{
    AirManeuverOrder* target = new (kSim) AirManeuverOrder(OrderOfOffset(order), *from, *to);
    return OffsetOfTarget(target);
}

unsigned int __stdcall MovementNewApproachRadius(unsigned int order, int x, int y, int radius)
{
    ApproachRadius* target = new (kSim) ApproachRadius(OrderOfOffset(order), x, y, radius);
    return OffsetOfTarget(target);
}

unsigned int __stdcall MovementNewRingApproach(unsigned int order, int x, int y, int inner, int outer)
{
    RingApproach* target = new (kSim) RingApproach(OrderOfOffset(order), x, y, inner, outer);
    return OffsetOfTarget(target);
}

unsigned int __stdcall MovementNewPointMarker(unsigned int order, Point16 pos, Point16 size)
{
    PointMarker* target = new (kSim) PointMarker(OrderOfOffset(order), pos, size);
    return OffsetOfTarget(target);
}

unsigned int __stdcall MovementLoadTarget(int type, unsigned int order, HapiBank* file, const char* name)
{
    int source = (int)OrderOfOffset(order);
    char* bank = const_cast<char*>(name);
    switch (type) {
    case 2:
        return OffsetOfTarget(new (kSim) PathOrder(source, file, bank));
    case 3:
        return OffsetOfTarget(new (kSim) AirManeuverOrder(source, file, bank));
    case 4:
        return OffsetOfTarget(new (kSim) ApproachRadius(source, file, bank));
    case 5:
        return OffsetOfTarget(new (kSim) RingApproach(source, file, bank));
    case 6:
        return OffsetOfTarget(new (kSim) PointMarker(source, file, bank));
    }
    return 0;
}

void __stdcall MovementSaveTarget(unsigned int target, unsigned int order, HapiBank* file, const char* name)
{
    TargetOfOffset(target)->SerializeToSave(OrderOfOffset(order), file, const_cast<char*>(name));
}

int __stdcall MovementTargetType(unsigned int target)
{
    OrderFx* fx = TargetOfOffset(target);
    return fx == nullptr ? 0 : fx->GetType();
}
