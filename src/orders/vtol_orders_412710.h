#ifndef VTOL_ORDERS_412710_H
#define VTOL_ORDERS_412710_H

struct Order;
struct Unit;

int __stdcall AirToGroundOrder(Unit* unit, Order* order, int flags);

#endif
