#ifndef VTOL_ORDERS_4103E0_H
#define VTOL_ORDERS_4103E0_H

struct Order;
struct Unit;

int __stdcall VtolSeekAttackOrder(Unit* unit, Order* order, int flags);

#endif
