#ifndef MAIN_H
#define MAIN_H

#include "../../include/win_types.h"

void __stdcall AppendToDebugFile(char* text);
void __stdcall ReportViaException(const char* message);
int __stdcall ParseCommandLine(char* cmdLine, char* appName);
void __stdcall InstallOutOfMemoryHandler();
void __stdcall OutOfMemoryHandler();
int __stdcall GameMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
void __stdcall AtexitNoOp(void);
int __stdcall GameEntry(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
void __stdcall ChdirToExeDirectory();

int __stdcall GetPreferredLanguage(void);
UINT __stdcall GetPreferenceInt(char* key, int defaultValue);

#endif
