#ifndef CRASH_H
#define CRASH_H

#include "../../../include/win_types.h"

int __cdecl GetStackLow(void);
void* __stdcall GetStackHigh();
int __cdecl ReportException(EXCEPTION_POINTERS* ep, char* handlerName);
char* __cdecl GetExceptionName(unsigned long code);
void __stdcall UnhandledExceptionHandler(int ep);
void __stdcall FunctionTableAccess(int unused, unsigned int address);
void __stdcall CloseLoadedImage();
void __stdcall GetModuleBase(int arg1, int arg2);
void __stdcall UnloadImageHelp();
char __cdecl LoadImageHelp(char param);
void __stdcall ImageHlpAtexitHandler(void);
void __cdecl FormatSystemInfo(char* dest, int destLen);
void __cdecl NormalizeLineEndings(char * text, int size);

#endif
