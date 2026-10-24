#ifndef REGISTRY_H
#define REGISTRY_H

struct Rect;

int __stdcall RectInsideRect(Rect* a, Rect* b);
int __stdcall CompareByte1Equal(int a, int b);
void __stdcall SetBytes1To3(unsigned char* buf, unsigned char b2, unsigned char b3, unsigned char b4);
char* __stdcall SkipTextLines(char* text, int n);

#endif
