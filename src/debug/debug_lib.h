#ifndef DEBUG_LIB_H
#define DEBUG_LIB_H

#include <stddef.h>

char __stdcall IsMemFussy();
void __stdcall MemFussyAtexitHandler(void);
char __stdcall IsGonzo();
void __stdcall GonzoAtexitHandler(void);
char __stdcall IsMemSet();
void __stdcall MemSetAtexitHandler(void);
int __stdcall GetMemSetValue();
void __stdcall SetValueAtexitHandler(void);
size_t __cdecl GetBlockSize(void* p);
void __stdcall NopAbortHook(void);
void __cdecl AllocNotifyNop(int);
void* __cdecl GameAlloc(unsigned int size);
void* __cdecl GameCalloc(unsigned int count, unsigned int size);
void* __cdecl GameRealloc(void* block, unsigned int size);
void __cdecl GameFree(void* p);
char __cdecl AvWriteDetectStub(unsigned long);
void __cdecl ProtectPages(int addr, int size, int protect);
void __cdecl ProtectBlockReadOnly(void* p);
void __cdecl ProtectBlock(void* p, int protect);
void __cdecl ProtectBlockReadWrite(void* p);
void __stdcall AbortProgram(void);
bool __cdecl IsDebugKeyword(const char* arg);
void* __stdcall GetAllocLock();
void __stdcall AllocLockAtexitHandler(void);
void __cdecl CountAlloc(unsigned int size);
char __stdcall IsBackAlign(void);
void __stdcall BackAlignAtexitHandler(void);
int __stdcall GetDebugFillPattern(void);
void* __cdecl ReallocDebugBlock(void* p, unsigned int size, int flags);
unsigned int __cdecl LookupBlockSize(unsigned int key);
void __cdecl RoundRangeToPages(unsigned int* start, unsigned int* end);
void __stdcall SetFpuFussy();
void __stdcall FpuFussyAtexitHandler(void);
int __stdcall StringMaxSize(void);

#endif
