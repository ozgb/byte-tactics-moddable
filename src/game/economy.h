#ifndef ECONOMY_H
#define ECONOMY_H

#include "../../interfaces/economy.h"

unsigned int __stdcall EconomyRegionSize(const EconMapInfo* map);
void __stdcall EconomyInit(const EconMapInfo* map, const EconomyWorldView* world, void* region, unsigned int size);
void* __stdcall EconomyShutdown();
int __stdcall EconomyApply(const EconomyEvent* event);
void __stdcall EconomyUpdatePlayer(int player);
void __stdcall EconomyUpdateMetalExtraction(EconUnit unit);
void __stdcall EconomyUpdateResourceSharing(int player);
int __stdcall EconomyRequestGranted(EconUnit unit, unsigned int resources);
double __stdcall EconomyGetEnergyUse(EconType type);
void __stdcall EconomyUnitAccount(EconUnit unit, EconAccount* out);
void __stdcall EconomyWriteState(const StateWriter* writer);

#endif
