#ifndef VTOL_ORDERS_H
#define VTOL_ORDERS_H
struct Order;
struct Unit;
struct Vec3;

struct Unit;

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

int __stdcall VtolFollowOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolPatrolOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolLandingOrder(Unit* unit, Order* order, int flags);
int __stdcall AirStrikeOrder(Unit* unit, Order* order, unsigned int flags);
int __stdcall AirToGroundOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolHelpBuildOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolReclaimOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolReclaimUnitOrder(Unit* unit, Order* order, unsigned int flags);

int __stdcall FindLandingPad(Unit* unit, int pad);

int __stdcall VtolSeekAttackOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolSeekGuardOrder(Unit* unit, Order* order, int flags);
int __stdcall AirToAirOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolRepairPatrolOrder(Unit* unit, Order* order, unsigned int flags);

void __stdcall PrepVtolClimb(Unit* unit, Order* order, unsigned int flags);
Vec3 __stdcall AddVec3(const Vec3& a, const Vec3& b);

#endif
