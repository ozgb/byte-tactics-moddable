#ifndef UNIT_ORDERS_403A20_H
#define UNIT_ORDERS_403A20_H

struct Order;
struct Unit;

int __stdcall MobileBuildOrder(Unit* unit, Order* order, int flags);

#endif
