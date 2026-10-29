#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../../interfaces/host.h"
#include "win_types.h"

#include "rect.h"

union DisplayAppFlags {
    unsigned short value;
    struct {
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
        unsigned short sound_opt : 1;
        unsigned short quitting : 1;
        unsigned short bit12 : 4;
    } bits;
};

struct FrameRateCounter {
    int accum;
    unsigned int lastTick;
    int frames;
    int rate;

    void Tick()
    {
        unsigned int now = host_time_ms();
        accum += now - lastTick;
        lastTick = now;
        frames++;
        if (accum > 2000) {
            accum = 1000;
        }
        if (accum > 1000) {
            rate = frames;
            accum -= 1000;
            frames = 0;
        }
    }
};

struct DirectDrawState {
    void *ddraw;
    void *primary;
    void *back;
    void *clipper;
    void *palette;
    void *activeSurface;
    int haveBackBuffer;
};

#pragma pack(push, 2)
struct DisplayAppView {
    void* hInstance;
    int nCmdShow;
    char* className;
    char* title;
    void* wndProc;
    unsigned short menuId;
    char unknown_16[2];
    char wc[0x28];
    void* hwnd;
    int windowed;
    char unknown_48[8];
    char unknown_50[0x80 - 0x50];
    void (__cdecl *callback)(int, int, int);
    DirectDrawState draw;
    Rect mode;
    void* font;
    char unknown_b4[0xc4 - 0xb4];
    int shadeTable;
    int lightTable;
    char unknown_cc[0xd4 - 0xcc];
    int width;
    int height;
    int useOverrideSurface;
    int active;
    char unknown_e4[0xe8 - 0xe4];
    int tickScale;
    int wa_left;
    DisplayAppFlags flags;
    char wa_rest[0xfc - 0xf2];
    char unknown_fc[0x1da - 0xfc];
    FrameRateCounter counter;
    Rect startMode;
    int startWidth;
    int startHeight;
    unsigned short videoFlags;
    char unknown_204[0x214 - 0x204];
    tagPALETTEENTRY entries[256];
    float scale;
    void** items;
    int itemCount;
    int availPhys;
    int unknown_624;
    char unknown_628[0x728 - 0x628];
    unsigned char lastDir;
};
#pragma pack(pop)

#include "../../include/display_mode.h"

struct DisplayModeTable {
    int count;
    DisplayMode* modes;
    char unknown_8[12];
    char* buffer;
};

struct MouseEventView {
    int x;
    int y;
    int buttons;
    int time;
    int message;
    int flag;
};

#include "surface.h"

struct TimerSlot {
    void* callback;
    int id;
    int interval;
    int countdown;
};

struct TimerSlotTable {
    TimerSlot slots[10];
    void (__cdecl *closeHandler)(int);
    int closeHandlerArg;
    int count;
};

typedef void (__stdcall *TimerCallback)(int);

extern DisplayAppView* g_display;
extern TimerSlotTable g_timerSlots;
extern unsigned int g_lastTimerTick;
extern long g_gfxBlitLockHeld;
extern long g_gfxBlitLockOwner;
extern void* g_gfxBlitLockEvent;

#include "../files/files_api.h"
#include "../gui/keyboard.h"
#include "../gui/keyboard.h"
#include "../gui/mouse.h"
#include "palette.h"
int __stdcall LockScreen(Surface *s);
void __cdecl BlitSurface(Surface *dst, void *src, int x, int y);
int __stdcall UnlockScreen(Surface *s);
#include "object_pool.h"

// FUNCTION: 0x4b4f50
int ApplyPalette()
{
    DisplayAppView* d = g_display;
    if (d->windowed) {
        host_palette_apply();
        return 1;
    }
    return host_palette_apply();
}

// FUNCTION: 0x4b4fd0
void __stdcall SetCloseHandler(void (__cdecl *callback)(int), int param)
{
    g_timerSlots.closeHandler = callback;
    g_timerSlots.closeHandlerArg = param;
}

// FUNCTION: 0x4b4ff0
void __stdcall ReleaseDirectDraw(DisplayAppView* d)
{
    host_display_close();
}

// FUNCTION: 0x4b5070
int __stdcall CheckDirectXVersion(int want0, int want1, int want2, int want3, int want4)
{
    host_media_version v;
    if (!host_media_version_get(&v)) {
        return 0;
    }
    if (v.major_only) {
        return v.major_lo >= (unsigned int)want4;
    }
    if (v.major_hi == (unsigned int)want0) {
        if (v.major_lo == (unsigned int)want1) {
            if (v.minor_hi == (unsigned int)want2) {
                return v.minor_lo >= (unsigned int)want3;
            }
            return v.minor_hi >= (unsigned int)want2;
        }
        return v.major_lo >= (unsigned int)want1;
    }
    return v.major_hi >= (unsigned int)want1;
}

// FUNCTION: 0x4b52e0
void __stdcall InitDisplayDefaults(DisplayAppView* p)
{
    p->videoFlags &= 0xfc00;
    p->startWidth = 640;
    p->startHeight = 480;
    p->startMode.left = 0;
    p->startMode.top = 0;
    p->startMode.right = 639;
    p->startMode.bottom = 479;
    p->scale = 1.0f;
}

// FUNCTION: 0x4b5330
void __stdcall EnumModesCallback(const host_display_mode* desc, void* context)
{
    DisplayModeTable* list = (DisplayModeTable*)context;
    DisplayMode* mode = &list->modes[list->count];
    if (desc->bits == 8) {
        mode->width = desc->width;
        mode->height = desc->height;
        mode->refreshRate = desc->refresh;
        list->count++;
    }
}

// FUNCTION: 0x4b5370
int __stdcall GetDisplayModes(DisplayModeTable* list)
{
    if (g_display->windowed) {
        int width;
        int height;
        list->count = 0;
        list->modes[list->count].width = 0x280;
        list->modes[list->count].height = 0x1e0;
        list->modes[list->count].refreshRate = 0;
        list->count++;
        list->modes[list->count].width = 0x320;
        list->modes[list->count].height = 0x258;
        list->modes[list->count].refreshRate = 0;
        list->count++;
        list->modes[list->count].width = 0x400;
        list->modes[list->count].height = 0x300;
        list->modes[list->count].refreshRate = 0;
        list->count++;
        host_display_screen_size(&width, &height);
        if (width >= 0x500 && height >= 0x400) {
            list->modes[list->count].width = 0x500;
            list->modes[list->count].height = 0x400;
            list->modes[list->count].refreshRate = 0;
            list->count++;
        }
        if (width >= 0x640 && height >= 0x4b0) {
            list->modes[list->count].width = 0x640;
            list->modes[list->count].height = 0x4b0;
            list->modes[list->count].refreshRate = 0;
            list->count++;
        }
        return 1;
    }

    host_display_mode found[512];
    int found_count = 0;
    int result = host_display_mode_list(found, 512, &found_count);
    list->count = 0;
    for (int i = 0; i < found_count; i++)
        EnumModesCallback(&found[i], list);
    return result;
}

static inline void ClearWindowedFlag(DisplayAppView *d)
{
    d->windowed = 0;
}

// FUNCTION: 0x4b5510
int __stdcall SetFullScreen(int mode) {
    int lockResult;
    DisplayAppView *d;
    Surface surf;

    while (1) {
        int result = host_atomic_exchange((int*)&g_gfxBlitLockHeld, 0x4d41494e);
        if (result == 0) {
            g_gfxBlitLockOwner = 0x4d41494e;
            lockResult = 0;
            break;
        }
        if (g_gfxBlitLockOwner == 0x4d41494e) {
            lockResult = result;
            break;
        }
        host_event_wait(g_gfxBlitLockEvent, HOST_WAIT_FOREVER);
    }
    d = g_display;
    DirectDrawState *dd = &d->draw;
    dd->haveBackBuffer = 0;
    ReleaseDirectDraw(g_display);

    ClearWindowedFlag(g_display);
    host_window_resize(g_display->width, g_display->height);

    if (mode != 0) {
        g_display->flags.value |= 2;

        int shown = host_display_mode_set(g_display->width, g_display->height, 0,
                                          (const unsigned char*)g_display->entries);
        dd->haveBackBuffer = host_display_has_back_buffer();
        if (shown == HOST_DISPLAY_FAILED)
            goto fail;
        if (g_display->draw.activeSurface) {
            LockScreen(&surf);
            BlitSurface(&surf, g_display->draw.activeSurface, 0, 0);
            UnlockScreen(&surf);
        }
    } else {
        g_display->flags.value &= ~2;

        unsigned char *bits = 0;
        host_frame frame;
        if (host_display_mode_set(g_display->width, g_display->height, HOST_DISPLAY_WINDOWED, 0) == HOST_DISPLAY_OK) {
            d->windowed = 1;
            if (host_frame_lock(HOST_FRAME_BACK, &frame) == HOST_FRAME_OK) {
                bits = frame.pixels;
                host_frame_unlock(HOST_FRAME_BACK);
            }
        }
        InitSurface((Surface *)&d->unknown_50[0], g_display->width, g_display->height,
                     (g_display->width + 3) & ~3, (int)bits);
        host_window_set_topmost(0);
    }

    SetPaletteColors((unsigned char *)g_display->entries, 0, 0x100);
    if (lockResult == 0) {
        g_gfxBlitLockOwner = 0;
        host_atomic_exchange((int*)&g_gfxBlitLockHeld, 0);
        host_event_set(g_gfxBlitLockEvent);
    }
    return 1;

fail:
    if (lockResult == 0) {
        g_gfxBlitLockOwner = 0;
        host_atomic_exchange((int*)&g_gfxBlitLockHeld, 0);
        host_event_set(g_gfxBlitLockEvent);
    }
    return 0;
}

// FUNCTION: 0x4b5910
void ToggleFullScreen()
{
    if (g_display->flags.bits.bit1) {
        SetFullScreen(0);
    } else {
        SetFullScreen(1);
    }
}

// FUNCTION: 0x4b5940
void __stdcall SetResolution(int x, int y)
{
    g_display->width = x;
    g_display->height = y;
    SetFullScreen(g_display->flags.bits.bit1);
}

// FUNCTION: 0x4b5980
int __stdcall InitEnvironment(DisplayAppView* d)
{
    g_display = d;
    host_memory m;
    host_memory_status(&m);
    d->availPhys = m.total;
    d->counter.accum = 0;
    d->counter.lastTick = host_time_ms();
    d->counter.frames = 0;
    d->counter.rate = 0;
    d->draw.ddraw = 0;
    d->draw.primary = 0;
    d->draw.back = 0;
    d->draw.clipper = 0;
    d->draw.palette = 0;
    d->draw.activeSurface = 0;
    d->unknown_624 = 0;
    d->useOverrideSurface = 0;
    d->flags.bits.quitting = 0;
    d->hwnd = 0;
    d->flags.bits.sound_opt = (d->videoFlags >> 9) & 1;
    InitKeyQueue(0x1e);
    InitMouse(0x14, d->flags.bits.sound_opt);
    MouseEventView view;
    view.x = 0;
    view.y = 0;
    view.buttons = 0;
    d->lastDir = 0;
    SaveStartDirectory();
    SetCurrentMouseEvent((MouseEvent *)((int *)&view));

    d->items = 0;
    d->itemCount = 0;
    int w = d->startWidth;
    int h = d->startHeight;
    d->width = w;
    d->height = h;
    int subsys = (d->videoFlags & 0x1fe) << 1;
    d->flags.value = (d->flags.value & 0xfc03) | subsys | 1;
    if (d->flags.bits.has_c4) {
        AllocShadeTable((Display *)d);
    } else {
        d->shadeTable = 0;
    }
    if (d->flags.bits.has_c0) {
        AllocAlphaTable((Display *)d);
    }
    if (d->flags.bits.has_c8) {
        AllocLightTable((Display *)d);
    } else {
        d->lightTable = 0;
    }
    if (d->flags.bits.has_cc) {
        AllocGrayTable((Display *)d);
    }
    if (d->flags.bits.has_d0) {
        AllocBlueTable((Display *)d);
    }
    if (d->flags.bits.gdi) {
        d->windowed = 0;
        d->callback = 0;
        d->mode = d->startMode;
        d->hwnd = host_window_open(d->title, w, h, d->videoFlags & 1);
        if (d->hwnd != 0) {
            int r = SetFullScreen(d->videoFlags & 1);
            if (r != 0) {
                return 1;
            }
        }
        ReleaseDirectDraw(d);
        d->windowed = 0;
        host_message(d->hwnd, d->title, "Error:  Environment Initialization Failed!\nCheck your DirectX setup", HOST_MESSAGE_PLAIN);
        host_window_close();
        return 0;
    }
    return 1;
}

int __stdcall HandleHostEvent(host_event* ev)
{
    MouseEventView e;
    switch (ev->type) {
    case HOST_EVENT_DESTROY:
        if (g_display->flags.bits.bit1)
            SetFullScreen(0);
        host_post_quit();
        return 0;
    case HOST_EVENT_FOCUS:
        if (ev->a == 0)
            g_display->active = 0;
        else
            g_display->active = 1;
        return 1;
    case HOST_EVENT_CLOSE:
        if (g_timerSlots.closeHandler != 0) {
            g_timerSlots.closeHandler(g_timerSlots.closeHandlerArg);
            return 1;
        }
        host_window_close();
        return 1;
    case HOST_EVENT_KEY_DOWN:
        HandleVirtualKey(ev->a, 0);
        return 1;
    case HOST_EVENT_CHAR:
        PushKeyCode(ev->a);
        return 1;
    case HOST_EVENT_SYS_KEY_DOWN:
        HandleVirtualKey(ev->a, 1);
        return 1;
    case HOST_EVENT_MOUSE_MOVE:
        e.x = ev->x;
        e.y = ev->y;
        e.buttons = ev->buttons;
        e.time = ev->time_ms * g_display->tickScale / 1000;
        e.flag = 0;
        e.message = ev->type;
        SetCurrentMouseEvent((MouseEvent *)((int*)&e));
        return 1;
    case HOST_EVENT_LBUTTON_DOWN:
    case HOST_EVENT_LBUTTON_UP:
    case HOST_EVENT_RBUTTON_DOWN:
    case HOST_EVENT_RBUTTON_UP:
        e.x = ev->x;
        e.y = ev->y;
        e.buttons = ev->buttons;
        e.time = ev->time_ms * g_display->tickScale / 1000;
        e.flag = 0;
        break;
    case HOST_EVENT_LBUTTON_DOUBLE:
    case HOST_EVENT_RBUTTON_DOUBLE:
        e.x = ev->x;
        e.y = ev->y;
        e.buttons = ev->buttons;
        e.time = ev->time_ms * g_display->tickScale / 1000;
        e.flag = 1;
        break;
    case HOST_EVENT_DEVICE_CHANGE:
    case HOST_EVENT_MEDIA_NOTIFY:
        if (g_display->callback != 0)
            g_display->callback(ev->type, ev->a, ev->b);
        return 1;
    case HOST_EVENT_PALETTE_QUERY:
        host_palette_apply();
        return 1;
    case HOST_EVENT_PALETTE_CHANGED:
        if (ev->a != 0)
            return 1;
        host_palette_apply();
        return 1;
    default:
        return 1;
    }
    e.message = ev->type;
    PushMouseEvent((MouseEvent *)&e);
    return 1;
}

// FUNCTION: 0x4b5cc0
int DispatchHostEvents()
{
    host_event ev;
    int alive = 1;
    while (host_event_next(&ev)) {
        if (!HandleHostEvent(&ev))
            alive = 0;
    }
    return alive;
}

// FUNCTION: 0x4b6110
void __stdcall ShutdownEnvironment(DisplayAppView* d)
{
    DestroyPreCleanup();
    ShutdownMouse();
    for (int i = 0; i < d->itemCount; i++)
        HAPI_CloseArchive((OPENHAPIFILE*)d->items[i]);
    if (d->items)
        GameFreeThunk(d->items);
    if (d->flags.bits.has_c4)
        FreeShadeTable((Display *)d);
    if (d->flags.bits.has_c0)
        FreeAlphaTable((Display *)d);
    if (d->flags.bits.has_c8)
        FreeLightTable((Display *)d);
    if (d->flags.bits.has_cc)
        FreeGrayTable((Display *)d);
    if (d->flags.bits.has_d0)
        FreeBlueTable((Display *)d);
    ReleaseDirectDraw(d);
    d->windowed = 0;
}

// FUNCTION: 0x4b6220
int GetDisplay(void)
{
    return (int)g_display;
}

// FUNCTION: 0x4b6230
void __stdcall QuitApp(char* message)
{
    g_display->flags.bits.quitting = 1;
    if (g_display->flags.bits.bit1) {
        SetFullScreen(0);
    }
    if (message) {
        host_message(g_display->hwnd, g_display->title, message, HOST_MESSAGE_PLAIN);
    }
    host_window_post(HOST_EVENT_DESTROY);
}

// FUNCTION: 0x4b6290
void __stdcall FatalError(char* message)
{
    host_fatal(g_display->hwnd, g_display->title, message);
}

// FUNCTION: 0x4b62c0
int __stdcall EmptyPostArchiveMountHook(int)
{
    return 0;
}

// FUNCTION: 0x4b62d0
void __stdcall InitTimers(int tickRate)
{
    g_display->tickScale = tickRate;
    g_timerSlots.count = 0;
    for (int i = 0; i < 10; i++) {
        g_timerSlots.slots[i].interval = -1;
    }
    g_lastTimerTick = (host_time_ms() * g_display->tickScale) / 1000;
}

// FUNCTION: 0x4b6330
int GetTickRate()
{
    return g_display->tickScale;
}

// FUNCTION: 0x4b6340
unsigned int GetTicks()
{
    return host_time_ms() * g_display->tickScale / 1000;
}

// FUNCTION: 0x4b6370
void UpdateTimers()
{
    unsigned int q1 = (host_time_ms() * g_display->tickScale) / 1000;
    int diff = (int)q1 - g_lastTimerTick;
    g_lastTimerTick = (host_time_ms() * g_display->tickScale) / 1000;

    for (TimerSlot* s = g_timerSlots.slots; s < g_timerSlots.slots + 10; s++) {
        if (s->interval >= 0) {
            if ((s->countdown -= diff) <= 0) {
                int arg = s->id;
                TimerCallback fn = (TimerCallback)s->callback;
                fn(arg);
                s->countdown = s->interval;
            }
        }
    }
}

// FUNCTION: 0x4b63f0
int __stdcall AddTimer(int interval, int id, TimerCallback callback)
{
    unsigned int now = (host_time_ms() * g_display->tickScale) / 1000;
    int diff = (int)now - g_lastTimerTick;
    g_lastTimerTick = (host_time_ms() * g_display->tickScale) / 1000;

    for (TimerSlot* s = g_timerSlots.slots; s < g_timerSlots.slots + 10; s++) {
        if (s->interval >= 0) {
            if ((s->countdown -= diff) <= 0) {
                TimerCallback fn = (TimerCallback)s->callback;
                fn(s->id);
                s->countdown = s->interval;
            }
        }
    }

    for (int i = 0; i < 10; i++) {
        if (g_timerSlots.slots[i].interval < 0) {
            g_timerSlots.slots[i].callback = callback;
            g_timerSlots.slots[i].id = id;
            g_timerSlots.slots[i].interval = interval;
            g_timerSlots.slots[i].countdown = interval;
            g_timerSlots.count++;
            return i;
        }
    }
    return -1;
}

// FUNCTION: 0x4b64d0
int __stdcall RemoveTimer(int i)
{
    if (i >= g_timerSlots.count)
        return 0;
    if (i < 0)
        return 0;
    g_timerSlots.slots[i].interval = -1;
    return 1;
}

// FUNCTION: 0x4b6510
void ResetTimers()
{
    g_timerSlots.count = 0;
    for (int i = 0; i < 10; i++) {
        g_timerSlots.slots[i].interval = -1;
    }
    g_lastTimerTick = host_time_ms() * g_display->tickScale / 1000;
}

// FUNCTION: 0x4b6560
unsigned long GetMilliseconds(void)
{
    return host_time_ms();
}

// FUNCTION: 0x4b6570
int __stdcall DrawFrameRate()
{
    FrameRateCounter* c = &g_display->counter;
    unsigned int now = host_time_ms();
    c->accum += now - c->lastTick;
    c->lastTick = now;
    c->frames++;
    if (c->accum > 2000) {
        c->accum = 1000;
    }
    if (c->accum > 1000) {
        c->rate = c->frames;
        c->accum -= 1000;
        c->frames = 0;
    }

    if (!g_display->windowed) {
        char buf[128];
        int len = sprintf(buf, "FRATE %d", g_display->counter.rate);
        host_frame_text(HOST_FRAME_BACK, g_display->font, 0, 0, buf, len, 0xffff00);
    }

    return g_display->counter.rate;
}

// FUNCTION: 0x4b66a0
int GetFrameRate()
{
    g_display->counter.Tick();
    return g_display->counter.rate;
}

// FUNCTION: 0x4b6700
int GetScreenWidth()
{
    return g_display->width;
}

// FUNCTION: 0x4b6710
int GetScreenHeight()
{
    return g_display->height;
}

// FUNCTION: 0x4b6720
int __stdcall PointInRect(Rect* r, int x, int y)
{
    if (x < r->left || x > r->right)
        return 0;
    if (y < r->top || y > r->bottom)
        return 0;
    return 1;
}
