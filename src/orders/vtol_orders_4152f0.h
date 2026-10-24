#ifndef VTOL_ORDERS_4152F0_H
#define VTOL_ORDERS_4152F0_H

struct Order;
struct Unit;

int __stdcall VtolRepairPatrolOrder(Unit* unit, Order* order, unsigned int flags);

#endif
