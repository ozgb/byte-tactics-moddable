#ifndef DISPLAY_H
#define DISPLAY_H
struct App_4b5980;

#include "surface.h"

struct OPENHAPIFILE;

union Flags_004c61b0 {
    unsigned short word;
    unsigned char byte;
    struct {
        unsigned short flag0 : 1;
        unsigned short flag1 : 1;
        unsigned short bit2_15 : 14;
    } bits;
};

struct Screen {
    char pad_0[0x8];
    void* primary;
    void* surface;
    char pad_10[0x18 - 0x10];
};

#pragma pack(push, 1)
struct Display {
    char pad_0[0x40];
    void* hwnd;
    int windowed;
    char pad_48[8];
    Surface cached;
    Screen screen;
    Surface* activeSurface;
    int haveBackBuffer;
    Rect vec;
    char pad_b0[0xb4 - 0xb0];
    void* lastBackBuffer;
    char pad_b8[0xbc - 0xb8];
    Surface* overrideSurface;
    char pad_c0[0xc4 - 0xc0];
    unsigned char* palette;
    char pad_c8[0xd4 - 0xc8];
    int width;
    int height;
    int useOverrideSurface;
    char pad_e0[0xe4 - 0xe0];
    int field_e4;
    char pad_e8[0xf0 - 0xe8];
    Flags_004c61b0 flags;
    char pad_f2[0x196 - 0xf2];
    int rect_x;
    int rect_y;
    char pad_19e[0x1b2 - 0x19e];
    GafFrame* bmp;
    int x;
    int y;
    Surface* saveMouse1;
    char pad_1c2[0x1ce - 0x1c2];
    int cursorThreadEnabled;
    int cursorOverlayEnabled;
    char pad_1d6[0x614 - 0x1d6];
    float paletteBrightness;
    OPENHAPIFILE** files;
    int count;
    char pad_620[8];
    char cwd[0x100];
    char lastDir[0x100];

    int LockScreen(Surface* out);
    int LockPrimary(Surface* out);
};
#pragma pack(pop)


int __stdcall ApplyPalette();
int __stdcall CheckDirectXVersion(int want0, int want1, int want2, int want3, int want4);
int __stdcall SetFullScreen(int mode);
void __stdcall ToggleFullScreen();
void __stdcall SetResolution(int x, int y);
int __stdcall DispatchHostEvents();
void __stdcall InitTimers(int tickRate);
int __stdcall GetTickRate();
void __stdcall UpdateTimers();
int __stdcall RemoveTimer(int i);
void __stdcall ResetTimers();
int __stdcall DrawFrameRate();
int __stdcall GetFrameRate();

void __stdcall ReleaseDirectDraw(App_4b5980* d);
void __stdcall InitDisplayDefaults(App_4b5980* p);
int __stdcall InitEnvironment(App_4b5980* d);
void __stdcall ShutdownEnvironment(App_4b5980* d);
void __stdcall QuitApp(char* message);
#include "../../interfaces/services.h"
unsigned long __stdcall GetMilliseconds(void);
int __stdcall GetScreenWidth();
int __stdcall GetScreenHeight();

#endif
