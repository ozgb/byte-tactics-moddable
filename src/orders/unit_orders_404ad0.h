#ifndef UNIT_ORDERS_404AD0_H
#define UNIT_ORDERS_404AD0_H

struct Order;
struct Unit;

int __stdcall ReclaimOrder(Unit* unit, Order* order, int flags);

#endif
