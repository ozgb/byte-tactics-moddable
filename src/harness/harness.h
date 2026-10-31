#ifndef HARNESS_H
#define HARNESS_H

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

#endif
