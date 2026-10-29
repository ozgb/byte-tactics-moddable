#ifndef DISPLAY_H
#define DISPLAY_H
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
        unsigned short fullscreen : 1;
        unsigned short gdi : 1;
        unsigned short bit3 : 1;
        unsigned short bit4 : 1;
        unsigned short has_c0 : 1;
        unsigned short has_c4 : 1;
        unsigned short has_c8 : 1;
        unsigned short has_cc : 1;
        unsigned short has_d0 : 1;
        unsigned short soundOpt : 1;
        unsigned short quitting : 1;
        unsigned short bit12_15 : 4;
    } bits;
};

union DisplayVideoFlags {
    unsigned short word;
    struct {
        unsigned short b0 : 1;
        unsigned short b1 : 1;
        unsigned short b2 : 1;
        unsigned short b3 : 1;
        unsigned short b4 : 1;
        unsigned short b5 : 1;
        unsigned short b6 : 1;
        unsigned short b7 : 1;
        unsigned short b8 : 1;
        unsigned short b9 : 1;
        unsigned short spare : 6;
    } bits;
};

struct Screen {
    void (__cdecl *callback)(int, int, int);
    void* ddraw;
    void* primary;
    void* surface;
    void* clipper;
    void* ddPalette;
};

struct FrameRateCounter {
    int accum;
    unsigned int lastTick;
    int frames;
    int rate;

    void Tick();
};

#pragma pack(push, 1)
struct Display {
    void* hInstance;
    int nCmdShow;
    char* className;
    char* title;
    char pad_10[4];
    int menuId;
    char pad_18[0x40 - 0x18];
    void* hwnd;
    int windowed;
    char pad_48[8];
    Surface cached;
    Screen screen;
    Surface* activeSurface;
    int haveBackBuffer;
    Rect vec;
    void* frameRateFont;
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
    int active;
    int field_e4;
    int tickScale;
    char pad_ec[0xf0 - 0xec];
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
    FrameRateCounter counter;
    Rect startMode;
    int startWidth;
    int startHeight;
    DisplayVideoFlags videoFlags;
    Font* font;
    int foreColor;
    int backColor;
    int keyColor;
    tagPALETTEENTRY entries[256];
    float paletteBrightness;
    OPENHAPIFILE** files;
    int count;
    int availPhys;
    int reserved624;
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

void __stdcall ReleaseDirectDraw(Display* d);
void __stdcall InitDisplayDefaults(Display* p);
int __stdcall InitEnvironment(Display* d);
void __stdcall ShutdownEnvironment(Display* d);
void __stdcall QuitApp(char* message);
#include "../../interfaces/services.h"
unsigned long __stdcall GetMilliseconds(void);
int __stdcall GetScreenWidth();
int __stdcall GetScreenHeight();

#endif
