#ifndef VTOL_ORDERS_H
#define VTOL_ORDERS_H

struct Order;
struct Point16;
struct Unit;
struct Vec3;
union Fixed;

int __stdcall VtolEvadeOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolLandIfCanOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolStandbyOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolMoveOrder(Unit* unit, Order* order, int flags);
Vec3 __stdcall DirectionFromAngle(short angle, Fixed scale);
int __stdcall VtolUnloadOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolMobileBuildOrder(Unit* unit, Order* order, int flags);
void __stdcall CellToWorldPos(Point16 a, Vec3* out, Point16 b);
int __stdcall VtolRepairUnitOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolGetRepairedOrder(Unit* unit, Order* order, int unused);
int __stdcall AirToGroundHoverOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolPickupOrder(Unit* unit, Order* order, int flags);

#endif
