#ifndef VTOL_ORDERS_414A80_H
#define VTOL_ORDERS_414A80_H

struct Order;
struct Unit;

int __stdcall VtolReclaimUnitOrder(Unit* unit, Order* order, unsigned int flags);

#endif
