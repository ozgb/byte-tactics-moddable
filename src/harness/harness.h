#ifndef HARNESS_H
#define HARNESS_H

struct Unit;

void __stdcall HarnessInit(char* cmdLine);
unsigned __stdcall HarnessStepsPerFrame();
unsigned __stdcall HarnessGameSeed(unsigned live);
unsigned __stdcall HarnessCrtSeed(unsigned live);
int __stdcall HarnessViewRand();
void __stdcall HarnessStartSkirmish();
void __stdcall HarnessAfterStep();
void __stdcall HarnessBeforeStep();
void __stdcall HarnessPhase(int phase);
void __stdcall HarnessSoundEvent(const char* kind, int index, const char* file, const int* volume, const int* position);
void __stdcall HarnessUiOrder(unsigned char kind, int remove, const Unit* unit, const Unit* target, const int* pos, int typeId, int amount);
void __stdcall HarnessUiQueue(const char* name, const Unit* unit, int count);
void __stdcall HarnessUiCancel(unsigned char kind, const Unit* unit);
void __stdcall HarnessUiGroup(const char* what, int group);

#endif
