#ifndef DISPLAY_CONTEXT_H
#define DISPLAY_CONTEXT_H

#pragma pack(push, 1)
struct DisplayContext {
    char unknown_0[0x44];
    int windowed;
    char unknown_48[0x94 - 0x48];
    void* ddPalette;
    char unknown_98[0xc0 - 0x98];
    void* alphaTable;
    void* shadeTable;
    void* lightTable;
    void* grayTable;
    void* blueTable;
    char unknown_d4[0xf0 - 0xd4];
    unsigned short opt : 1;
    unsigned short bit1 : 1;
    unsigned short gdi : 1;
    unsigned short bit3 : 1;
    unsigned short bit4 : 1;
    unsigned short has_c0 : 1;
    unsigned short has_c4 : 1;
    unsigned short has_c8 : 1;
    unsigned short has_cc : 1;
    unsigned short has_d0 : 1;
    unsigned short bit10_15 : 6;
    char unknown_f2[0x1d2 - 0xf2];
    int cursorOverlayEnabled;
    char unknown_1d6[0x214 - 0x1d6];
    unsigned int entries[256];
    int paletteBrightness;
};
#pragma pack(pop)

#endif
