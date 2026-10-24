#ifndef UNIT_ORDERS_406300_H
#define UNIT_ORDERS_406300_H

struct Order;
struct Unit;

int __stdcall FollowGroundOrder(Unit* unit, Order* order, int flags);

#endif
