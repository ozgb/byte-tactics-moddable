#ifndef UNIT_ORDERS_H
#define UNIT_ORDERS_H

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

#endif
