#ifndef VTOL_ORDERS_410850_H
#define VTOL_ORDERS_410850_H

struct Order;
struct Unit;

int __stdcall VtolSeekGuardOrder(Unit* unit, Order* order, int flags);

#endif
