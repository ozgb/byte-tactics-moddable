#ifndef FONT_H
#define FONT_H
struct Surface;

struct Surface;

void __stdcall DrawStringCentered(Surface* dst, unsigned char* text, int flag);

void __stdcall DrawOutlinedString(Surface* dst, unsigned char* text, int fore, int back, int y);

#endif
