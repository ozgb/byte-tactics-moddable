#ifndef VTOL_ORDERS_410E70_H
#define VTOL_ORDERS_410E70_H

struct Order;
struct Unit;

int __stdcall VtolPatrolOrder(Unit* unit, Order* order, int flags);

#endif
