#ifndef VTOL_ORDERS_412D40_H
#define VTOL_ORDERS_412D40_H

struct Order;
struct Unit;

int __stdcall AirToAirOrder(Unit* unit, Order* order, int flags);

#endif
