#ifndef ORDERS_ORDERS_SEAM_H
#define ORDERS_ORDERS_SEAM_H

class HapiBank;
class MissionType;
struct OrdersEventHeader;
struct StateWriter;
struct Unit;
struct Vec3;

int __stdcall OrdersApply(const OrdersEventHeader* event, unsigned int size);
void __stdcall OrdersRunOrders(Unit* unit);
void __stdcall OrdersRunSecondaryOrders(Unit* unit);
int __stdcall OrdersGetOrderFlags(Unit* unit);
int __stdcall OrdersGetOrderType(unsigned char mode, Unit* unit, Unit* target, Vec3* pos);
int __stdcall OrdersMissionTypeFromName(const char* name);
void __stdcall OrdersRegisterAllOrderTypes(void);
void __stdcall OrdersClearOrderTypeTable(void);
void __stdcall OrdersLoadOrders(Unit* unit, HapiBank* file, int count);
int __stdcall OrdersSaveOrders(Unit* unit, HapiBank* file);
void __stdcall OrdersWriteState(const StateWriter* writer);

#endif
