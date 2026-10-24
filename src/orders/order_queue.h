#ifndef ORDER_QUEUE_H
#define ORDER_QUEUE_H

struct Order;
struct Unit;

void __stdcall AddBeCarriedOrder(Unit* p);
void __stdcall StopBuildingScript(Unit* obj, Order* target);
int __stdcall ComputeReclaimDamagePulse(Unit* a, Unit* b, int n);
int __fastcall GetTableEntryByTypeByte(unsigned char*);

unsigned int __stdcall WaitIfNotInBuildStance(void*, void*, unsigned int);
int __stdcall WaitIfCobBusy(char*, char*, unsigned int);
int __stdcall GetOrderFlags(void* param);

#endif
