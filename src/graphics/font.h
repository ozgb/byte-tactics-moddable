#ifndef FONT_H
#define FONT_H
struct Surface;

struct Surface;

struct Font {
    unsigned char glyphs[1];
    char unknown_1[2];
    unsigned char first;
    unsigned short offsets[1];
};

void __stdcall DrawStringCentered(Surface* dst, unsigned char* text, int flag);

void __stdcall DrawOutlinedString(Surface* dst, unsigned char* text, int fore, int back, int y);

#endif
