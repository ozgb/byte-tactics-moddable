#ifndef ORDER_LIST_H
#define ORDER_LIST_H

class HapiBank;
struct Order;
struct Unit;

int __stdcall GetOrderTargetIfFlagged(void*);
void __stdcall DeleteOrders(Unit* owner, int all);
void __stdcall EnsurePatrolReturnOrder(Unit* p, Order* item);
void __stdcall WriteUnitTypeNameKey(HapiBank* file, unsigned short id);
int __stdcall OrderTypeNameLess(int, char*);
void __stdcall InsertOrderBefore(Unit* owner, Order* node, Order* before);
void __stdcall AppendOrder(Unit* owner, Order* node);
void __stdcall AppendOrderToTail(Unit* owner, Order* node);
void __stdcall InsertOrderAfterMarked(Unit* owner, Order* node);
int __stdcall IssueRepairOrder(Unit* unit, Unit* target, int);
int __stdcall FindBestTargetIfFireAtWill(Unit* unit);
void __stdcall EnqueueOrderType(Unit* p, unsigned char type);
void __stdcall MoveOrderToTail(Unit* owner, Order* node);
void __stdcall RunOrders(Unit* unit);
void __stdcall RunSecondaryOrders(Unit* p);
void __stdcall RegisterAllOrderTypes();
void __stdcall ClearOrderTypeTable();

#endif
