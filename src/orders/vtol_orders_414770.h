#ifndef VTOL_ORDERS_414770_H
#define VTOL_ORDERS_414770_H

struct Order;
struct Unit;

int __stdcall VtolReclaimOrder(Unit* unit, Order* order, int flags);

#endif
