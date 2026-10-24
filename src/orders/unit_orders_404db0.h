#ifndef UNIT_ORDERS_404DB0_H
#define UNIT_ORDERS_404DB0_H

struct Order;
struct Unit;

int __stdcall ResurrectOrder(Unit* unit, Order* order, int flags);

#endif
