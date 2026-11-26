#ifndef HARNESS_H
#define HARNESS_H


void __stdcall HarnessInit(char* cmdLine);
void __stdcall HarnessExit();
unsigned __stdcall HarnessStepsPerFrame();
unsigned __stdcall HarnessGameSeed(unsigned live);
unsigned __stdcall HarnessCrtSeed(unsigned live);
int __stdcall HarnessViewRand();
void __stdcall HarnessStartSkirmish();
void __stdcall HarnessLoadBegins();
void __stdcall HarnessLoadEnded();
int __stdcall HarnessAwaitLoadAndEndFrame();
void __stdcall HarnessAfterStep();
void __stdcall HarnessBeforeStep();
void __stdcall HarnessPhase(int phase);
void __stdcall HarnessRecordPhase(int phase);
int __stdcall HarnessSharing();
void __stdcall HarnessSharePacket(const void* packet, int size);
bool HarnessEventLogOpen();
void __stdcall HarnessSimEventSlot(unsigned slot);
const char* HarnessSimEventPhase(unsigned slot);
int HarnessSimEventOtherThread(unsigned slot);
void HarnessEventText(const char* text);
void HarnessViewEvent(unsigned kind, unsigned flags, const unsigned* args, unsigned count);
void __stdcall HarnessSoundEvent(const char* kind, int index, const char* file, const int* volume, const int* position);
void __stdcall HarnessUiGroup(const char* what, int group);
struct tagPALETTEENTRY;
struct DlPicture;
void __stdcall HarnessFrameDrawn(const unsigned char* pixels, int pitch, int width, int height, const tagPALETTEENTRY* entries);

const char* HarnessMapName();
void HarnessDisplayFrame(const unsigned char* palette, const DlPicture* cursor, int cursorX, int cursorY);

#endif
