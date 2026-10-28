#ifndef MOUSE_H
#define MOUSE_H
struct MouseDisplay;

struct MouseQueueEvent;

void __stdcall DrawSoftwareCursor();
void __cdecl MouseThreadProc(int param);
void __stdcall SetCursorSprite(int value);
int __stdcall GetCursorSprite();
void __stdcall ClearMouseEventQueue();
void __stdcall InitMouse(int count, int start);
void __stdcall ShutdownMouse();

int __stdcall PeekMouseEvent(MouseQueueEvent* out);
void __stdcall PushMouseEvent(MouseQueueEvent* ev);

void __stdcall RestoreCursorBackground(MouseDisplay* obj);
void __stdcall CaptureBackgroundAndDrawCursor(MouseDisplay* obj);
void __stdcall RedrawMouseCursor(MouseDisplay* app);
int __stdcall StartMouseThread(MouseDisplay* s);
int __stdcall StopMouseThread(MouseDisplay* s);

#endif
