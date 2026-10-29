#ifndef DISPLAY_H
#define DISPLAY_H
struct DisplayAppView;

#include "win_types.h"
#include "surface.h"
#include "../util/mouse_event.h"

struct OPENHAPIFILE;
struct Font;

union DisplayFlags {
    unsigned short word;
    unsigned char byte;
    struct {
        unsigned short flag0 : 1;
        unsigned short flag1 : 1;
        unsigned short gdi : 1;
        unsigned short bit3 : 1;
        unsigned short bit4 : 1;
        unsigned short has_c0 : 1;
        unsigned short has_c4 : 1;
        unsigned short has_c8 : 1;
        unsigned short has_cc : 1;
        unsigned short has_d0 : 1;
        unsigned short sound_opt : 1;
        unsigned short bit11_15 : 5;
    } bits;
};

struct Screen {
    char pad_0[0x8];
    void* primary;
    void* surface;
    char pad_10[0x14 - 0x10];
    void* ddPalette;
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
    unsigned char* alphaTable;
    unsigned char* shadeTable;
    unsigned char* lightTable;
    unsigned char* grayTable;
    unsigned char* blueTable;
    int width;
    int height;
    int useOverrideSurface;
    char pad_e0[0xe4 - 0xe0];
    int field_e4;
    char pad_e8[0xf0 - 0xe8];
    DisplayFlags flags;
    int keyQueueSize;
    int keyQueue[30];
    int keyQueueHead;
    int keyQueueTail;
    char pad_176[0x186 - 0x176];
    int mouseQueueSize;
    MouseEvent* mouseQueue;
    int mouseQueueHead;
    int mouseQueueTail;
    MouseEvent mouseEvent;
    int cursorHideCount;
    GafFrame* cursorSprite;
    int x;
    int y;
    Surface* saveMouse1;
    Surface* saveMouse2;
    Surface* saveMouse3;
    int mouseThread;
    int cursorThreadEnabled;
    int cursorOverlayEnabled;
    int mouseThreadStop;
    char pad_1da[0x204 - 0x1da];
    Font* font;
    int foreColor;
    int backColor;
    int keyColor;
    tagPALETTEENTRY entries[256];
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


Display* GetDisplay(void);
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

void __stdcall ReleaseDirectDraw(DisplayAppView* d);
void __stdcall InitDisplayDefaults(DisplayAppView* p);
int __stdcall InitEnvironment(DisplayAppView* d);
void __stdcall ShutdownEnvironment(DisplayAppView* d);
void __stdcall QuitApp(char* message);
#include "../../interfaces/services.h"
unsigned long __stdcall GetMilliseconds(void);
int __stdcall GetScreenWidth();
int __stdcall GetScreenHeight();

#endif
