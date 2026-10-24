#ifndef ORDER_QUEUE_4384A0_H
#define ORDER_QUEUE_4384A0_H

struct Order;
struct Unit;

void __stdcall AddBeCarriedOrder(Unit* p);
void __stdcall StopBuildingScript(Unit* obj, Order* target);
int __stdcall ComputeReclaimDamagePulse(Unit* a, Unit* b, int n);
int __fastcall GetTableEntryByTypeByte(unsigned char*);

#endif
