#ifndef DISPLAY_H
#define DISPLAY_H
struct App_4b5980;

int __stdcall ApplyPalette();
int __stdcall CheckDirectXVersion(int want0, int want1, int want2, int want3, int want4);
int __stdcall SetFullScreen(int mode);
void __stdcall ToggleFullScreen();
void __stdcall SetResolution(int x, int y);
int __stdcall DispatchHostEvents();
void __stdcall InitTimers(int tickRate);
int __stdcall GetTickRate();
void __stdcall UpdateTimers();
int __stdcall RemoveTimer(int i);
void __stdcall ResetTimers();
int __stdcall DrawFrameRate();
int __stdcall GetFrameRate();

void __stdcall ReleaseDirectDraw(App_4b5980* d);
void __stdcall InitDisplayDefaults(App_4b5980* p);
int __stdcall InitEnvironment(App_4b5980* d);
void __stdcall ShutdownEnvironment(App_4b5980* d);
void __stdcall QuitApp(char* message);
#include "../../interfaces/services.h"
unsigned long __stdcall GetMilliseconds(void);
int __stdcall GetScreenWidth();
int __stdcall GetScreenHeight();

#endif
