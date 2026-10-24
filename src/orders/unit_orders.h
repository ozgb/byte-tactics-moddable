#ifndef UNIT_ORDERS_H
#define UNIT_ORDERS_H
struct Order;
struct Unit;

struct Order;
struct Unit;

int __stdcall StopOrder(Unit* unit, Order* order, int unused);
int __stdcall MakeSelectableOrder(Unit* unit, Order* order, int unused);
int __stdcall WaitOrder(Unit* unit, Order* order, int flags);
int __stdcall SelfDestructOrder(Unit* unit, Order* order, int flags);
int __stdcall AttackNoMoveOrder(Unit* unit, Order* order, int flags);
int __stdcall GuardNoMoveOrder(Unit* unit, Order* order, int flags);
int __stdcall SelfRepairOrder(Unit* unit, Order* order, int unused);
int __stdcall BuildingBuildOrder(Unit* unit, Order* order, int flags);
int __stdcall BuildWeaponOrder(Unit* unit, Order* order, int unused);
int __stdcall ParalyzeOrder(Unit* unit, Order* order, int unused);
int __stdcall BeCarriedOrder(Unit* unit, Order* order, int unused);
int __stdcall StandingMoveOrder(Unit* unit, Order* order, int unused);
int __stdcall StandingFireOrder(Unit* unit, Order* order, int unused);
int __stdcall AttackSpecialOrder(Unit* unit, Order* order, int unused);
int __stdcall MoveGroundOrder(Unit* unit, Order* order, int flags);
int __stdcall PatrolOrder(Unit* unit, Order* order, int flags);
int __stdcall RepairUnitOrder(Unit* unit, Order* order, int flags);
int __stdcall RepairUnitNoMoveOrder(Unit* unit, Order* order, int unused);
int __stdcall StandbyOrder(Unit* unit, Order* order, int flags);
int __stdcall StandbyMineOrder(Unit* unit, Order* order, int unused);
int __stdcall ParkOrder(Unit* unit, Order* order, int flags);
int __stdcall GroundUnloadOrder(Unit* unit, Order* order, int flags);
void __stdcall RegisterGroundOrders();
int __stdcall AttackUTypeOrder(Unit* unit, Order* order, int unused);
int __stdcall HelpBuildOrder(Unit* unit, Order* order, int flags);

int __stdcall WaitForAttackOrder(int unused1, Order* obj, int unused3);
int __stdcall GetBuiltOrder(Unit* unit, Order* order, unsigned int flags);
int __stdcall ActivateOrder(Unit* unit, int unused1, int unused2);
int __stdcall DeactivateOrder(Unit* unit, int unused1, int unused2);
int __stdcall CloakOnOrder(char* param1, int unused1, int unused2);
int __stdcall CloakOffOrder(char* unit, int unused1, int unused2);
int __stdcall QMoveQPatrolOrder(int unused1, void* order, int unused3);
int __stdcall AttackKamikazeOrder(Unit* unit, Order* order, unsigned flags);
int __stdcall AttackChaseOrder(Unit* unit, Order* order, unsigned int flags);
int __stdcall SuppressOrder(Unit* unit, Order* order, unsigned flags);
int __stdcall CaptureOrder(Unit* unit, Order* order, unsigned int flags);
int __stdcall ReclaimUnitOrder(Unit* unit, Order* order, unsigned int flags);

int __stdcall MobileBuildOrder(Unit* unit, Order* order, int flags);
int __stdcall ReclaimOrder(Unit* unit, Order* order, int flags);
int __stdcall ResurrectOrder(Unit* unit, Order* order, int flags);
int __stdcall RepairPatrolOrder(Unit* unit, Order* order, int flags);
int __stdcall FollowGroundOrder(Unit* unit, Order* order, int flags);
int __stdcall TeleportOrder(Unit* unit, Order* order, int unused);
void __stdcall CopyDwordIfNonNull(int* dest, int* src);

#endif
