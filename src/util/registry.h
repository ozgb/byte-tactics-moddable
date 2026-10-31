#ifndef REGISTRY_H
#define REGISTRY_H
#include "../../interfaces/services.h"
struct Rect;

struct Rect;

int __stdcall RectInsideRect(Rect* a, Rect* b);
int __stdcall CompareByte1Equal(int a, int b);
void __stdcall SetBytes1To3(unsigned char* buf, unsigned char b2, unsigned char b3, unsigned char b4);

int __stdcall RectsOverlap(Rect* a, Rect* b);
int __stdcall GetWindowsUserName(char* out);
void __stdcall SetMediaNotifyCallback(void (__cdecl* callback)(int, int, int));

#endif
