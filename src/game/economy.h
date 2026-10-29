#ifndef ECONOMY_H
#define ECONOMY_H

struct ResourceAccount;

void __stdcall SettleResourceAccount(ResourceAccount* r, float ratioBacklog, float ratioDemand);

#endif
