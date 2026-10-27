#ifndef ORDER_LIST_H
#define ORDER_LIST_H
struct OrderTableEntry;

struct Unit;

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

void __stdcall DrawOrderOverlays(Unit* unit, unsigned int mask, void* obj, void* sel, int flag);
int __stdcall GetBuildWeaponPercent(Unit* owner);
int __stdcall SumQueuedBuildCount(Unit* owner, int index);
int __stdcall GetOrderName(Unit* obj);
int __stdcall ReadyOrder(int arg1, int arg2, int arg3);
int __stdcall IssueAttackOrder(Unit* unit, Unit* target, int);

int __stdcall CompareOrderTypeNames(const OrderTableEntry& a, const OrderTableEntry& b);

#endif
