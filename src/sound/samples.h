#ifndef SAMPLES_H
#define SAMPLES_H

int __stdcall NullStubRet0(void);
void __stdcall NopRet_B(void);
int __stdcall NullStubRet0_B(void);
void __stdcall NopRet_C(void);
int __stdcall NullStubRet0_C(void);
void __stdcall OnStreamTimer(int unused1);
void __stdcall InitLzssLock();
void __stdcall LzssInitTree(int index);
int __cdecl LzssAllocWindow(void);
int __stdcall LzssAllocTree();

#endif
