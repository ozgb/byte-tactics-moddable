#ifndef MOUSE_H
#define MOUSE_H

void __stdcall DrawSoftwareCursor();
void __cdecl MouseThreadProc(int param);
void __stdcall SetCursorSprite(int value);
int __stdcall GetCursorSprite();
void __stdcall ClearMouseEventQueue();
void __stdcall InitMouse(int count, int start);
void __stdcall ShutdownMouse();

#endif
