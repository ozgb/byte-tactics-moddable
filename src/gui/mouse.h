#ifndef MOUSE_H
#define MOUSE_H
struct Event_4c2d60;

void __stdcall DrawSoftwareCursor();
void __cdecl MouseThreadProc(int param);
void __stdcall SetCursorSprite(int value);
int __stdcall GetCursorSprite();
void __stdcall ClearMouseEventQueue();
void __stdcall InitMouse(int count, int start);
void __stdcall ShutdownMouse();

int __stdcall PeekMouseEvent(Event_4c2d60* out);
void __stdcall PushMouseEvent(Event_4c2d60* ev);

#endif
