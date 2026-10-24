#ifndef UNIT_ORDERS_405980_H
#define UNIT_ORDERS_405980_H

struct Order;
struct Unit;

int __stdcall RepairPatrolOrder(Unit* unit, Order* order, int flags);

#endif
