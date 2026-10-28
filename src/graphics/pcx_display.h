#ifndef PCX_DISPLAY_H
#define PCX_DISPLAY_H

#include "win_types.h"

struct Bitmap_004caec0;

#pragma pack(push, 1)
struct Game_004caec0 {
    char unknown_0[0x40];
    HWND hwnd;
    char unknown_44[0xbc - 0x44];
    Bitmap_004caec0* bitmap;
    char unknown_c0[0x1c];
    int useOverrideSurface;
    char unknown_e0[0x1b2 - 0xe0];
    int cursorFrame;
    char unknown_1b6[0x214 - 0x1b6];
    PALETTEENTRY palette[256];
};
#pragma pack(pop)

#endif
