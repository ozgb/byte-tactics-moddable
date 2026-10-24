#ifndef VICTORY_H
#define VICTORY_H

int __stdcall AreEnemiesEliminated();
int __stdcall IsLocalPlayerEliminated();
int __stdcall IsLocalPlayerOutOfUnits();
void __stdcall QueryGlobalMemoryStatus(void);
int __stdcall HarnessConditionClass(void* condition);

#endif
