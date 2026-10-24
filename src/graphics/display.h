#ifndef DISPLAY_H
#define DISPLAY_H

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

#endif
