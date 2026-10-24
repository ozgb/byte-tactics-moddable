#ifndef VTOL_ORDERS_40FBE0_H
#define VTOL_ORDERS_40FBE0_H

struct Order;
struct Unit;

int __stdcall VtolFollowOrder(Unit* unit, Order* order, int flags);

#endif
