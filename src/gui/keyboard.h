#ifndef KEYBOARD_H
#define KEYBOARD_H

void __stdcall InitKeyQueue(int size);
void __stdcall DestroyPreCleanup(void);
void __stdcall PushKeyCode(int v);
int __stdcall IsKeyDown(int key);
void __stdcall HandleVirtualKey(int key, int flag);
void __stdcall InitBlitLock();
void __stdcall SetCursorOverlayEnabled(int param);
void __stdcall SetCursorPosition(int x, int y);
void __stdcall GetCursorPosition(int* x, int* y);

#endif
