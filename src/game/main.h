#ifndef MAIN_H
#define MAIN_H

#include "../../include/win_types.h"

void __stdcall AppendToDebugFile(char* text);
void __stdcall InstallOutOfMemoryHandler();
void __stdcall OutOfMemoryHandler();
int __stdcall GameMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
void __stdcall AtexitNoOp(void);
int __stdcall GameEntry(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
void __stdcall ChdirToExeDirectory();

#endif
