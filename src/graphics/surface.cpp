#include <string.h>
#include "../../interfaces/host.h"
#include "win_types.h"

#include "surface.h"
#include "gaf_frame.h"


#include "display.h"

#pragma pack(push, 1)
struct LockEntry {
    Surface* surface;
    char flag;
};
#pragma pack(pop)

extern int g_screenLockCount;
extern LockEntry g_screenLocks[];


inline int Display::LockScreen(Surface* out)
{
    if (useOverrideSurface != 0) {
        *out = *overrideSurface;
        return 1;
    }
    if (windowed != 0) {
        *out = cached;
        return 1;
    }
    host_frame frame;
    if (host_frame_lock(HOST_FRAME_BACK, &frame) != HOST_FRAME_OK)
        return 0;
    out->width = width;
    out->height = height;
    out->pitch = frame.pitch;
    out->pixels = frame.pixels;
    out->x = 0;
    out->y = 0;
    out->zPriority = 10000;
    out->colorKey = -1;
    out->flag0 = 0;
    out->clip = vec;
    if (g_screenLockCount < 10) {
        g_screenLocks[g_screenLockCount].surface = out;
        g_screenLocks[g_screenLockCount].flag = 0;
    }
    return 1;
}

inline int Display::LockPrimary(Surface* out)
{
    if (windowed != 0) {
        *out = cached;
        return 1;
    }
    host_frame frame;
    if (host_frame_lock(HOST_FRAME_FRONT, &frame) != HOST_FRAME_OK)
        return 0;
    out->width = width;
    out->height = height;
    out->pitch = frame.pitch;
    out->pixels = frame.pixels;
    out->x = 0;
    out->y = 0;
    out->zPriority = 10000;
    out->colorKey = -1;
    out->flag0 = 0;
    out->clip = vec;
    return 1;
}

struct DirectDrawSurfaceRefs {
    int ddraw;
    void* primary;
    void* back;
};

int __stdcall LockScreen(Surface* out);
int __stdcall UnlockScreen(Surface* s);
void __cdecl BlitSurface(Surface* dst, Surface* src, int x, int y);
void __cdecl BlitSurfaceKeyed(Surface* dst, Surface* src, int x, int y, int color);
void __stdcall DrawSurface(Surface* dst, Surface* bmp, int x, int y);
void __stdcall DrawFrame(Surface* dst, GafFrame* bmp, int x, int y);
#include "display.h"
#include "../debug/debug_lib.h"
#include "object_pool.h"

// FUNCTION: 0x4c5d90
void __stdcall SetLockEntry(Surface* surface, char flag)
{
    if (g_screenLockCount < 10) {
        g_screenLocks[g_screenLockCount].surface = surface;
        g_screenLocks[g_screenLockCount].flag = flag;
    }
}

// FUNCTION: 0x4c5dc0
void __stdcall PopScreenLock(int param_1, int param_2)
{
    if (g_screenLockCount > 0) {
        g_screenLockCount--;
    }
}

// FUNCTION: 0x4c5de0
int GetScreenLockCount(void)
{
    return g_screenLockCount;
}

// FUNCTION: 0x4c5df0
void UnlockAllScreens(void)
{
    while (g_screenLockCount > 0) {
        int i = g_screenLockCount;
        if (g_screenLocks[g_screenLockCount].flag)
            UnlockPrimary(g_screenLocks[g_screenLockCount - 1].surface, 0, 0);
        else
            UnlockScreen(g_screenLocks[g_screenLockCount - 1].surface);
        if (i == g_screenLockCount)
            g_screenLockCount--;
    }
}

#pragma auto_inline(off)
// FUNCTION: 0x4c5e70
int __stdcall LockScreen(Surface* out)
{
    return GetDisplay()->LockScreen(out);
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x4c5fa0
int __stdcall UnlockScreen(Surface* s)
{
    Display* d = GetDisplay();
    if (d->windowed == 0 && d->useOverrideSurface == 0) {
        if (!host_frame_unlock(HOST_FRAME_BACK))
            return 0;
        if (g_screenLockCount > 0)
            g_screenLockCount--;
    }
    return 1;
}
#pragma auto_inline(on)

// FUNCTION: 0x4c5ff0
int __stdcall LockPrimary(Surface* out)
{
    return GetDisplay()->LockPrimary(out);
}

// FUNCTION: 0x4c60d0
int __stdcall UnlockPrimary(Surface* unused, tagRECT* r1, tagRECT* r2)
{
    Display* d = GetDisplay();
    if (d->windowed != 0) {
        if (r1 != 0) {
            host_frame_present_rect(r1->left, r1->top, r1->right - r1->left,
                                    r1->bottom - r1->top);
        }
        if (r2 != 0) {
            host_frame_present_rect(r2->left, r2->top, r2->right - r2->left,
                                    r2->bottom - r2->top);
        }
        return 1;
    }
    if (!host_frame_unlock(HOST_FRAME_FRONT)) {
        return 0;
    }
    return 1;
}

// FUNCTION: 0x4c61b0
int __stdcall SetPageFlipping(int enable)
{
    Display* obj = GetDisplay();
    obj->flags.bits.flag0 = enable;
    if (enable && obj->haveBackBuffer == 0) {
        return 0;
    }
    return 1;
}

// FUNCTION: 0x4c61f0
void __stdcall SetRestoreSurface(int surface)
{
    Display* obj = GetDisplay();
    obj->activeSurface = (Surface*)surface;
}

// FUNCTION: 0x4c6210
int __stdcall RestoreSurfaces(DirectDrawSurfaceRefs* arg)
{
    Display* d = GetDisplay();
    if (d->windowed != 0)
        return 0;

    int r = host_frame_restore() ? 0 : 1;
    if (r == 0) {
        Surface screen;
        LockScreen(&screen);
        BlitSurface(&screen, d->activeSurface, r, r);

        Display* d2 = GetDisplay();
        if (d2->windowed == 0 && d2->useOverrideSurface == 0 && host_frame_unlock(HOST_FRAME_BACK)) {
            if (g_screenLockCount > 0)
                g_screenLockCount--;
        }
    }
    return r;
}

// FUNCTION: 0x4c62c0
void RestoreScreen(void)
{
    Display* d = GetDisplay();
    if (d->flags.bits.flag1) {
        if (host_frame_is_lost()) {
            Display* d2 = GetDisplay();
            if (d2->windowed == 0) {
                if (host_frame_restore()) {
                    Surface screen;
                    LockScreen(&screen);
                    BlitSurface(&screen, d2->activeSurface, 0, 0);
                    UnlockScreen(nullptr);
                }
            }
        }
    }
    d->useOverrideSurface = 0;
}

#include "../../link/globals.h"

static inline long Lock()
{
    while (1) {
        long r = host_atomic_exchange((int*)&g_gfxBlitLockHeld, 0x4d41494e);
        if (r == 0) {
            g_gfxBlitLockOwner = 0x4d41494e;
            return 0;
        }
        if (g_gfxBlitLockOwner == 0x4d41494e)
            return r;
        host_event_wait(g_gfxBlitLockEvent, HOST_WAIT_FOREVER);
    }
}

static inline void Unlock(long held)
{
    if (held == 0) {
        g_gfxBlitLockOwner = 0;
        host_atomic_exchange((int*)&g_gfxBlitLockHeld, 0);
        host_event_set(g_gfxBlitLockEvent);
    }
}

static inline int RestoreSurfacesInline(Display* d)
{
    Display* dd = GetDisplay();
    if (dd->windowed != 0)
        return 0;
    if (!host_frame_restore())
        return 1;
    Surface screen;
    LockScreen(&screen);
    BlitSurface(&screen, dd->activeSurface, 0, 0);
    UnlockScreen(nullptr);
    return 0;
}

// FUNCTION: 0x4c63a0
void FlipScreen(void)
{
    Display* d = GetDisplay();
    unsigned short flags = d->flags.word;

    if ((flags & 2) == 0) {
        long held = Lock();
        Surface* p = &d->cached;
        DrawSurface(p, d->overrideSurface, 0, 0);
        DrawCursor(d, p);
        host_frame_present_rect(0, 0, p->width, d->cached.height);
        Unlock(held);
        return;
    }

    if (d->useOverrideSurface != 0) {
        host_frame frame;
        Surface out;
        Surface* bmp = d->overrideSurface;
        if (bmp->width != GetScreenWidth())
            return;
        if (bmp->height != GetScreenHeight())
            return;

        long held = Lock();
        int lr = host_frame_lock(HOST_FRAME_FRONT, &frame);
        if (lr == HOST_FRAME_OK) {
            out.width = d->width;
            out.height = d->height;
            out.pitch = frame.pitch;
            out.pixels = frame.pixels;
            DrawCursor(d, bmp);
            BlitSurface(&out, bmp, 0, 0);
            if (d->cursorThreadEnabled != 0 && d->cursorOverlayEnabled != 0)
                DrawSurface(bmp, d->saveMouse1, d->x, d->y);
            host_frame_unlock(HOST_FRAME_FRONT);
        } else if (lr == HOST_FRAME_LOST) {
            RestoreSurfacesInline(d);
        }
        Unlock(held);
        return;
    }

    if (d->haveBackBuffer != 0 && (flags & 1) != 0) {
        host_frame_present(1);
        return;
    }

    int hr;
    while (1) {
        hr = host_frame_present(0);
        if (hr == HOST_FRAME_OK)
            return;
        if (hr != HOST_FRAME_LOST)
            continue;
        hr = RestoreSurfacesInline(d);
        if (hr != 0)
            continue;
        return;
    }
}

// FUNCTION: 0x4c67c0
void __stdcall DrawCursor(Display* obj, Surface* dst)
{
    if (obj->cursorThreadEnabled != 0 && obj->cursorOverlayEnabled != 0 && obj->cursorSprite != 0) {
        obj->saveMouse1->width = obj->cursorSprite->width;
        obj->saveMouse1->height = obj->cursorSprite->height;
        obj->saveMouse1->pitch = obj->cursorSprite->width;
        obj->x = obj->mouseEvent.x - obj->cursorSprite->xOffset;
        obj->y = obj->mouseEvent.y - obj->cursorSprite->yOffset;
        DrawSurface(obj->saveMouse1, dst, -obj->x, -obj->y);
        DrawFrame(dst, obj->cursorSprite, obj->mouseEvent.x, obj->mouseEvent.y);
    }
}

#include <memory.h>

static void set_mem(unsigned char* p, int count, int colour)
{
    memset(p, colour, count);
}

static void clear_surface(Surface* s, int colour)
{
    memset(s->pixels, colour, s->height * s->pitch);
}

static void clear_screen(Display* o, int colour)
{
    memset(o->cached.pixels, colour, o->cached.height * o->cached.pitch);
}

// FUNCTION: 0x4c6890
int __stdcall FillSurface(Surface* surface, int colour)
{
    Display* obj = GetDisplay();
    int ret = 1;
    if (surface == 0) {
        if (obj->useOverrideSurface != 0) {
            Surface* s = obj->overrideSurface;
            set_mem(s->pixels, s->height * s->pitch, colour);
        } else if (!(obj->flags.byte & 2)) {
            clear_screen(obj, colour);
        } else {
            if (!host_frame_fill(HOST_FRAME_BACK, colour))
                ret = 0;
            return ret;
        }
    } else if (surface->flag0) {
        clear_surface(surface, colour);
    } else {
        if (!host_frame_fill(HOST_FRAME_BACK, colour))
            ret = 0;
    }
    return ret;
}

// FUNCTION: 0x4c69a0
void __stdcall SetOffscreenSurface(int surface)
{
    Display* d = GetDisplay();
    d->useOverrideSurface = 1;
    d->overrideSurface = (Surface*)surface;
}

// FUNCTION: 0x4c69c0
void __stdcall ResetClipRect(int* surface)
{
    Rect q;
    q.left = 0;
    q.top = 0;
    q.right = surface[0] - 1;
    q.bottom = surface[1] - 1;
    *(Rect*)(surface + 7) = q;
}

// FUNCTION: 0x4c69f0
Surface* __stdcall AllocSurface(char* name, int width, int height)
{
    Surface* s = (Surface*)GameAllocIgnoreTag(name, height * width + 0x30);
    InitSurface(s, width, height, width, (int)(s + 1));
    return s;
}

// FUNCTION: 0x4c6a60
void __stdcall InitSurface(Surface* s, int width, int height, int a, int b)
{
    s->width = width;
    s->height = height;
    s->pitch = a;
    s->pixels = (unsigned char*)b;
    s->x = 0;
    s->y = 0;
    s->zPriority = 10000;
    s->colorKey = -1;
    s->flag0 = 1;
    s->flag1 = 0;
    Rect r;
    r.left = 0;
    r.top = 0;
    r.right = width - 1;
    r.bottom = height - 1;
    s->clip = r;
}

// FUNCTION: 0x4c6ac0
void __stdcall FreeSurface(Surface* obj)
{
    if (obj != 0 && obj->flag0) {
        GameFreeThunk(obj);
    }
}

#pragma auto_inline(off)
// FUNCTION: 0x4c6ae0
Rect* Surface::GetClipRect(Rect* out)
{
    *out = clip;
    return out;
}
#pragma auto_inline(on)

// FUNCTION: 0x4c6b10
void Surface::SetClipRect(Rect r)
{
    clip = r;
}

// FUNCTION: 0x4c6b40
void __stdcall SetDisplayFieldE4(int value)
{
    Display* d = GetDisplay();
    d->field_e4 = value;
}

// FUNCTION: 0x4c6b60
int GetDisplayFieldE4()
{
    return GetDisplay()->field_e4;
}

// FUNCTION: 0x4c6b70
void __stdcall DrawSurface(Surface* dst, Surface* bmp, int x, int y)
{
    if (dst == 0) {
        if (bmp == 0)
            return;
        Surface screen;
        if (LockScreen(&screen) == 0)
            return;
        BlitSurface(&screen, bmp, x - bmp->x, y - bmp->y);
        UnlockScreen(nullptr);
        return;
    }
    if (bmp == 0) {
        Surface screen;
        if (LockScreen(&screen) == 0)
            return;
        BlitSurface(dst, &screen, x, y);
        UnlockScreen(nullptr);
        return;
    }
    BlitSurface(dst, bmp, x - bmp->x, y - bmp->y);
}

// FUNCTION: 0x4c6c50
void __stdcall DrawSurfaceKeyed(Surface* dst, Surface* src, int x, int y, int color)
{
    if (dst == 0) {
        Surface screen;
        if (LockScreen(&screen)) {
            BlitSurfaceKeyed(&screen, src, x - src->x, y - src->y, color);
            UnlockScreen(&screen);
        }
    } else {
        BlitSurfaceKeyed(dst, src, x - src->x, y - src->y, color);
    }
}

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <malloc.h>
#include "../files/files_api.h"
#include "../util/point.h"

void __cdecl BlitRect(Surface* dst, Surface* src, Rect* rect, Point* pos);
void __cdecl BlitRectKeyed(Surface* dst, Surface* src, Rect* rect, Point* pos, unsigned char transparent);
void __cdecl BlitTile32x32(Surface* dst, Surface* src, Rect* rect, Point* pos);

// FUNCTION: 0x4c6d20
void __stdcall CopySurfaceRect(Surface* dst, Surface* src, Rect* rect, Point* pos)
{
    if (dst == 0) {
        Surface screen;
        if (LockScreen(&screen)) {
            BlitRect(&screen, src, rect, pos);
            UnlockScreen(&screen);
        }
    } else {
        BlitRect(dst, src, rect, pos);
    }
}

// FUNCTION: 0x4c6dc0
void __stdcall CopySurfaceRectKeyed(Surface* dst, Surface* src, Rect* rect, Point* pos, unsigned char transparent)
{
    if (dst == 0) {
        Surface screen;
        if (LockScreen(&screen)) {
            BlitRectKeyed(&screen, src, rect, pos, transparent);
            UnlockScreen(&screen);
        }
    } else {
        BlitRectKeyed(dst, src, rect, pos, transparent);
    }
}

// FUNCTION: 0x4c6e70
void __stdcall DrawTile(Surface* dst, Surface* src, Rect* rect, Point* pos)
{
    if (dst == 0) {
        Surface screen;
        if (LockScreen(&screen)) {
            BlitTile32x32(&screen, src, rect, pos);
            UnlockScreen(&screen);
        }
    } else {
        BlitTile32x32(dst, src, rect, pos);
    }
}

// FUNCTION: 0x4c6f10
void __stdcall SaveSurface(Surface* surface, HapiBank* file)
{
    file->SeekBox(0);
    int header[2];
    header[0] = surface->width;
    header[1] = surface->height;
    file->WriteBox(header, 8);
    for (int i = 0; i < header[1]; i++) {
        file->WriteBox(surface->pixels + i * surface->pitch, header[0]);
    }
}


static inline Surface* NewSurface(char* name, int w, int h)
{
    Surface* s = (Surface*)GameAllocIgnoreTag(name, h * w + 0x30);
    s->width = w;
    s->pitch = w;
    s->height = h;
    s->x = 0;
    s->pixels = (unsigned char*)(s + 1);
    s->flag0 = 1;
    s->flag1 = 0;
    s->y = 0;
    s->zPriority = 10000;
    s->colorKey = -1;
    Rect r;
    r.left = 0;
    r.top = 0;
    r.right = w - 1;
    r.bottom = h - 1;
    s->clip = r;
    return s;
}

// FUNCTION: 0x4c6f80
Surface* __stdcall LoadSurface(HapiBank* file)
{
    file->SeekBox(0);
    int header[2];
    if (file->ReadBox(header, 8) < 8u) {
        return 0;
    }
    Surface* s = NewSurface("Loaded Surface", header[0], header[1]);
    for (int i = 0; i < header[1]; i++) {
        if (file->ReadBox(s->pixels + i * s->pitch, header[0]) < header[0]) {
            GameFreeThunk(s);
            return 0;
        }
    }
    return s;
}

// GLOBAL: 0x51fe48
extern int g_edgeListPrev[];
// GLOBAL: 0x51fea0
extern int g_edgeListNext[];

// FUNCTION: 0x4c7080
void __stdcall InitSurfaceIndexFreeList(int count)
{
    for (int i = 0; i < count; i++) {
        if (i == 0) {
            g_edgeListPrev[0] = count - 1;
        } else {
            g_edgeListPrev[i] = i - 1;
        }
        if (i == count - 1) {
            g_edgeListNext[i] = 0;
        } else {
            g_edgeListNext[i] = i + 1;
        }
    }
}

// GLOBAL: 0x51fef0
extern int g_edgeSpan;

struct Chunk {
    int field_0;
    int field_4;
};

struct Range {
    int low;
    int high;
};

// GLOBAL: 0x51fef8
extern Chunk* g_edgeTable;
// GLOBAL: 0x51fefc
extern int g_edgeHighIndex;
// GLOBAL: 0x51ff00
extern int g_edgeLowIndex;

int __cdecl MulDivTrunc(int a, int b, int c);

// FUNCTION: 0x4c70d0
void __stdcall InterpEdgeUvFromScanline(int value, Range* out, int at_low, int at_high)
{
    int i = g_edgeLowIndex;
    Chunk* table = g_edgeTable;
    int j = g_edgeListPrev[i];
    g_edgeHighIndex = j;
    while (value > table[j].field_4) {
        j = g_edgeListPrev[j];
        i = g_edgeListPrev[i];
        g_edgeHighIndex = j;
        g_edgeLowIndex = i;
    }
    int hi = table[j].field_4;
    int lo = table[i].field_4;
    int size = hi - lo;
    int offset = hi - value;
    g_edgeSpan = size;
    if (size == 0) {
        return;
    }
    switch (i) {
    case 0: {
        int span = size - offset;
        int* spanp = &span;
        out->low = 0;
        out->high = MulDivTrunc(at_high, *spanp, g_edgeSpan);
        return; }
    case 1:
        out->low = MulDivTrunc(at_low, offset, size);
        out->high = 0;
        return;
    case 2:
        out->low = at_low;
        out->high = MulDivTrunc(at_high, offset, g_edgeSpan);
        return;
    case 3:
        out->low = MulDivTrunc(at_low, size - offset, size);
        out->high = at_high;
        return;
    }
}

// GLOBAL: 0x51fe98
extern int g_edgeLowIndexB;
// GLOBAL: 0x51fef4
extern int g_edgeHighIndexB;
// GLOBAL: 0x51fe40
extern int g_edgeSpanB;

// FUNCTION: 0x4c71f0
void __stdcall InterpEdgeUvFromScanline_B(int value, Range* out, int at_low, int at_high)
{
    int i = g_edgeLowIndexB;
    Chunk* table = g_edgeTable;
    int j = g_edgeListNext[i];
    g_edgeHighIndexB = j;
    while (value > table[j].field_4) {
        j = g_edgeListNext[j];
        i = g_edgeListNext[i];
        g_edgeHighIndexB = j;
        g_edgeLowIndexB = i;
    }
    int hi = table[j].field_4;
    int lo = table[i].field_4;
    int size = hi - lo;
    int offset = hi - value;
    g_edgeSpanB = size;
    if (size == 0) {
        return;
    }
    switch (i) {
    case 0:
        out->low = MulDivTrunc(at_low, size - offset, size);
        out->high = 0;
        return;
    case 1:
        out->low = at_low;
        {
            int span = size - offset;
            int* spanp = &span;
            out->high = MulDivTrunc(at_high, *spanp, g_edgeSpanB);
        }
        return;
    case 2:
        out->low = MulDivTrunc(at_low, offset, size);
        out->high = at_high;
        return;
    case 3:
        out->low = 0;
        out->high = MulDivTrunc(at_high, offset, g_edgeSpanB);
        return;
    }
}


void __cdecl BlitSpan128(unsigned char* dest, unsigned char* src, int width, int y, int x, int rowstep, int colstep);
void __cdecl BlitSpan64(unsigned char* dest, unsigned char* src, int width, int y, int x, int rowstep, int colstep);
void __cdecl BlitSpan32(unsigned char* dest, unsigned char* src, int width, int y, int x, int rowstep, int colstep);
void __cdecl BlitSpan16(unsigned char* dest, unsigned char* src, int width, int y, int x, int rowstep, int colstep);

// FUNCTION: 0x4c7310
void __stdcall DrawQuadRow(int row, int* rect, Surface* surf, GafFrame* info)
{
    unsigned char* dest = (unsigned char*)surf->pixels;
    unsigned char* src = info->pixelsOrLayers;
    int rowstep = (rect[4] - rect[2]) / (rect[1] - rect[0]);
    int colstep = (rect[5] - rect[3]) / (rect[1] - rect[0]);
    Rect bounds;
    int width;
    int y;
    int x;

    surf->GetClipRect(&bounds);
    if (rect[0] < bounds.left) {
        rect[2] += rowstep * (bounds.left - rect[0]);
        rect[3] += colstep * (bounds.left - rect[0]);
        rect[0] = bounds.left;
    }
    if (rect[1] > bounds.right)
        rect[1] = bounds.right;
    width = rect[1] - rect[0];
    if (width > 0) {
        y = rect[2];
        x = rect[3];
        dest += surf->pitch * row + rect[0];
        switch (info->width) {
        case 0x80:
            BlitSpan128(dest, src, width, y, x, rowstep, colstep);
            return;
        case 0x40:
            BlitSpan64(dest, src, width, y, x, rowstep, colstep);
            return;
        case 0x20:
            BlitSpan32(dest, src, width, y, x, rowstep, colstep);
            return;
        case 0x10:
            BlitSpan16(dest, src, width, y, x, rowstep, colstep);
            return;
        case 8: {
            int n = width;
            do {
                *dest++ = src[(y >> 16) + ((x >> 13) & ~7)];
                y += rowstep;
                x += colstep;
            } while (--n);
            break; }
        default: {
            int n = width;
            do {
                *dest++ = src[(y >> 16) + (x >> 16) * info->width];
                y += rowstep;
                x += colstep;
            } while (--n);
            break; }
        }
    }
}

struct FrameQuad {
    Point p[4];
};

struct FrameQuadScanline {
    int left;
    int right;
    int tx;
    int ty;
    int trx;
    int try_;
    int pad[4];
};

// FUNCTION: 0x4c7580
void __stdcall DrawFrameQuad(void* surf, GafFrame* bmp,
                            FrameQuad* dst, FrameQuad* src)
{
    if (bmp == 0)
        return;
    if (dst == 0)
        return;

    Surface local;
    int locked;
    if (surf == 0) {
        int ok = LockScreen(&local);
        if (ok == 0)
            return;
        locked = 1;
        surf = &local;
    } else {
        locked = 0;
    }

    FrameQuad tmp;
    if (src == 0) {
        src = &tmp;
        tmp.p[0].x = 0;
        tmp.p[0].y = 0;
        tmp.p[1].x = bmp->width - 1;
        tmp.p[1].y = 0;
        tmp.p[2].x = bmp->width - 1;
        tmp.p[2].y = bmp->height - 1;
        tmp.p[3].x = 0;
        tmp.p[3].y = bmp->height - 1;
    }

    int y0, y1;
    FrameQuadScanline recs[800];
    int ymin = 999999;
    int ymax = -999999;
    int xmin = 999999;
    int xmax = -999999;
    Rect clip;
    int imin, imax;
    int i;
    FrameQuadScanline* out;
    int j, k;
    int dxdy;
    int x, tx, ty, dtx, dty;

    for (i = 0; i < 4; i++) {
        int y = dst->p[i].y;
        if (y < ymin) {
            ymin = y;
            imin = i;
        }
        if (y > ymax) {
            ymax = y;
            imax = i;
        }
        if (dst->p[i].x > xmax)
            xmax = dst->p[i].x;
        int xx = dst->p[i].x;
        if (xx < xmin)
            xmin = xx;
    }

    ((Surface*)surf)->GetClipRect(&clip);
    if (xmax < clip.left) {
        if (locked) UnlockScreen(&local);
        return;
    }
    if (xmin > clip.right) {
        if (locked) UnlockScreen(&local);
        return;
    }
    if (ymax < clip.top) {
        if (locked) UnlockScreen(&local);
        return;
    }
    if (ymin > clip.bottom) {
        if (locked) UnlockScreen(&local);
        return;
    }
    if (ymin < clip.top)
        ymin = clip.top;
    if (ymax > clip.bottom)
        ymax = clip.bottom;
    if (ymax == ymin) {
        if (locked) UnlockScreen(&local);
        return;
    }

    out = recs;
    i = imin;
    for (;;) {
        j = k = i - 1;
        if (k < 0)
            k = 3;
        y0 = dst->p[i].y;
        y1 = dst->p[k].y;
        if (y1 > clip.top && y0 < y1) {
            int dy = y1 - y0;
            x = dst->p[i].x;
            dxdy = ((dst->p[k].x - x) << 16) / dy;
            x = (x << 16) + 0xffff;
            tx = src->p[i].x << 16;
            ty = src->p[i].y << 16;
            dtx = ((src->p[k].x << 16) - tx) / dy;
            dty = ((src->p[k].y << 16) - ty) / dy;
            if (y0 < clip.top) {
                x += dxdy * (clip.top - y0);
                tx += dtx * (clip.top - y0);
                ty += dty * (clip.top - y0);
                y0 = clip.top;
            }
            if (y1 > clip.bottom)
                y1 = clip.bottom;
            if (y0 < y1) {
                int n = y1 - y0;
                do {
                    out->left = x >> 16;
                    out->tx = tx;
                    out->ty = ty;
                    x += dxdy;
                    tx += dtx;
                    ty += dty;
                    out++;
                } while (--n);
            }
        }
        i = i - 1;
        if (i < 0)
            i = 3;
        if (i == imax)
            break;
    }

    out = recs;
    i = imin;
    for (;;) {
        j = (i + 1) & 3;
        k = j;
        y0 = dst->p[i].y;
        y1 = dst->p[k].y;
        if (y1 > clip.top && y0 < y1) {
            int dy = y1 - y0;
            x = dst->p[i].x;
            dxdy = ((dst->p[k].x - x) << 16) / dy;
            x = (x << 16) + 0xffff;
            tx = src->p[i].x << 16;
            ty = src->p[i].y << 16;
            dtx = ((src->p[k].x << 16) - tx) / dy;
            dty = ((src->p[k].y << 16) - ty) / dy;
            if (y0 < clip.top) {
                x += dxdy * (clip.top - y0);
                tx += dtx * (clip.top - y0);
                ty += dty * (clip.top - y0);
                y0 = clip.top;
            }
            if (y1 > clip.bottom)
                y1 = clip.bottom;
            if (y0 < y1) {
                int n = y1 - y0;
                do {
                    out->right = x >> 16;
                    out->trx = tx;
                    out->try_ = ty;
                    x += dxdy;
                    tx += dtx;
                    ty += dty;
                    out++;
                } while (--n);
            }
        }
        i = (i + 1) & 3;
        if (i == imax)
            break;
    }

    out = recs;
    for (i = ymin; i < ymax; i++) {
        if (out->right - out->left > 0)
            DrawQuadRow(i, (int*)out, (Surface *)surf, bmp);
        out++;
    }

    if (locked) {
unlock:
        UnlockScreen(&local);
    }
}

int __stdcall BroadcastPacket(int id, unsigned char* packet, int size);

// FUNCTION: 0x4c7a20
void __stdcall DrawTexturedSpan(int row, int* span, GafFrame* surf, GafFrame* info)
{
    unsigned char* mask = surf->scratch;
    unsigned char* dest = surf->pixelsOrLayers;
    unsigned char* src = info->pixelsOrLayers;
    int width = span[1] - span[0];
    int rowstep = (span[4] - span[2]) / width;
    int colstep = (span[5] - span[3]) / width;
    int dstep = (span[7] - span[6]) / width;

    if (span[0] < 0) {
        span[2] -= rowstep * span[0];
        span[3] -= colstep * span[0];
        int clippedDepth = span[6] - dstep * span[0];
        span[0] = 0;
        span[6] = clippedDepth;
    }
    if (span[1] > surf->width - 1)
        span[1] = surf->width - 1;
    width = span[1] - span[0];
    if (width > 0) {
        int y = span[2];
        int x = span[3];
        int w = span[6];

        dest += surf->width * row + span[0];
        if (mask != 0) {
            mask += surf->width * row + span[0];
            switch (info->width) {
            case 0x80: {
                int n = width;
                do {
                    if (*mask <= (unsigned char)(w >> 16)) {
                        *dest = src[(y >> 16) + ((x >> 9) & ~0x7f)];
                        *mask = (unsigned char)(w >> 16);
                    }
                    x += colstep;
                    w += dstep;
                    dest++;
                    mask++;
                    y += rowstep;
                } while (--n);
                return; }
            case 0x40: {
                int n = width;
                do {
                    if (*mask <= (unsigned char)(w >> 16)) {
                        *dest = src[(y >> 16) + ((x >> 10) & ~0x3f)];
                        *mask = (unsigned char)(w >> 16);
                    }
                    x += colstep;
                    w += dstep;
                    dest++;
                    mask++;
                    y += rowstep;
                } while (--n);
                return; }
            case 0x20: {
                int n = width;
                do {
                    if (*mask <= (unsigned char)(w >> 16)) {
                        *dest = src[(y >> 16) + ((x >> 11) & ~0x1f)];
                        *mask = (unsigned char)(w >> 16);
                    }
                    x += colstep;
                    w += dstep;
                    dest++;
                    mask++;
                    y += rowstep;
                } while (--n);
                return; }
            case 0x10: {
                int n = width;
                do {
                    if (*mask <= (unsigned char)(w >> 16)) {
                        *dest = src[(y >> 16) + ((x >> 12) & ~0xf)];
                        *mask = (unsigned char)(w >> 16);
                    }
                    x += colstep;
                    w += dstep;
                    dest++;
                    mask++;
                    y += rowstep;
                } while (--n);
                return; }
            case 8: {
                int n = width;
                do {
                    if (*mask <= (unsigned char)(w >> 16)) {
                        *dest = src[(y >> 16) + ((x >> 13) & ~7)];
                        *mask = (unsigned char)(w >> 16);
                    }
                    x += colstep;
                    w += dstep;
                    dest++;
                    mask++;
                    y += rowstep;
                } while (--n);
                return; }
            default: {
                int n = width;
                do {
                    if (*mask <= (unsigned char)(w >> 16)) {
                        *dest = src[(y >> 16) + ((x >> 16) * info->width)];
                        *mask = (unsigned char)(w >> 16);
                    }
                    x += colstep;
                    w += dstep;
                    dest++;
                    mask++;
                    y += rowstep;
                } while (--n);
                return; }
            }
        }
        switch (info->width) {
        case 0x80:
            BlitSpan128(dest, src, width, y, x, rowstep, colstep);
        case 0x40:
            BlitSpan64(dest, src, width, y, x, rowstep, colstep);
            return;
        case 0x20:
            BlitSpan32(dest, src, width, y, x, rowstep, colstep);
            return;
        case 0x10:
            BlitSpan16(dest, src, width, y, x, rowstep, colstep);
            return;
        case 8: {
            int n = width;
            do {
                *dest++ = src[(y >> 16) + ((x >> 13) & ~7)];
                y += rowstep;
                x += colstep;
            } while (--n);
            return; }
        default: {
            int n = width;
            do {
                *dest++ = src[(y >> 16) + ((x >> 16) * info->width)];
                y += rowstep;
                x += colstep;
            } while (--n);
            return; }
        }
    }
}

#include <float.h>

// FUNCTION: 0x4c8020
void __stdcall DrawLitTexturedSpan(int row, int* span, GafFrame* target, GafFrame* texture)
{
    unsigned char* dest = target->pixelsOrLayers;
    unsigned char* depth = target->scratch;
    unsigned char* src = texture->pixelsOrLayers;
    Display* display = GetDisplay();
    int width = span[1] - span[0];
    int du = (span[4] - span[2]) / width;
    int dv = (span[5] - span[3]) / width;
    int dz = (span[7] - span[6]) / width;
    int dl = (span[9] - span[8]) / width;
    if (span[0] < 0) {
        span[2] -= du * span[0];
        span[3] -= dv * span[0];
        span[6] -= dz * span[0];
        span[8] -= dl * span[0];
        span[0] = 0;
    }
    if (span[1] > target->width - 1) span[1] = target->width - 1;
    width = span[1] - span[0];
    if (width > 0) {
        int u = span[2];
        int v = span[3];
        int z = span[6];
        int light = span[8];
        int value;
        dest += target->width * row + span[0];
        if (depth) {
            depth += target->width * row + span[0];
            switch(texture->width) {
            case 128: {
                int n = width;
                do {
                    value = z >> 16;
                    if (*depth <= (unsigned char)value) {
                        unsigned int pixel = 0;
                        pixel = src[(u >> 16) + ((v >> 9) & ~127)];
                        *dest = display->shadeTable[pixel + ((light >> 16) * 256)];
                        *depth = (unsigned char)value;
                    }
                    v += dv; z += dz; light += dl;
                    dest++; depth++; u += du;
                } while (--n);
                return;
            }
            case 64: {
                int n = width;
                do {
                    value = z >> 16;
                    if (*depth <= (unsigned char)value) {
                        unsigned int pixel = 0;
                        pixel = src[(u >> 16) + ((v >> 10) & ~63)];
                        *dest = display->shadeTable[pixel + ((light >> 16) * 256)];
                        *depth = (unsigned char)value;
                    }
                    v += dv; z += dz; light += dl;
                    dest++; depth++; u += du;
                } while (--n);
                return;
            }
            case 32: {
                int n = width;
                do {
                    value = z >> 16;
                    if (*depth <= (unsigned char)value) {
                        unsigned int pixel = 0;
                        pixel = src[(u >> 16) + ((v >> 11) & ~31)];
                        *dest = display->shadeTable[pixel + ((light >> 16) * 256)];
                        *depth = (unsigned char)value;
                    }
                    v += dv; z += dz; light += dl;
                    dest++; depth++; u += du;
                } while (--n);
                return;
            }
            case 16: {
                int n = width;
                do {
                    value = z >> 16;
                    if (*depth <= (unsigned char)value) {
                        unsigned int pixel = 0;
                        pixel = src[(u >> 16) + ((v >> 12) & ~15)];
                        *dest = display->shadeTable[pixel + ((light >> 16) * 256)];
                        *depth = (unsigned char)value;
                    }
                    v += dv; z += dz; light += dl;
                    dest++; depth++; u += du;
                } while (--n);
                return;
            }
            case 8: {
                int n = width;
                do {
                    value = z >> 16;
                    if (*depth <= (unsigned char)value) {
                        unsigned int pixel = 0;
                        pixel = src[(u >> 16) + ((v >> 13) & ~7)];
                        *dest = display->shadeTable[pixel + ((light >> 16) * 256)];
                        *depth = (unsigned char)value;
                    }
                    v += dv; z += dz; light += dl;
                    dest++; depth++; u += du;
                } while (--n);
                return;
            }
            default: {
                int n = width;
                do {
                    value = z >> 16;
                    if (*depth <= (unsigned char)value) {
                        unsigned int pixel = 0;
                        pixel = src[(u >> 16) + (v >> 16) * texture->width];
                        *dest = display->shadeTable[pixel + ((light >> 16) * 256)];
                        *depth = (unsigned char)value;
                    }
                    v += dv; z += dz; light += dl;
                    dest++; depth++; u += du;
                } while (--n);
                return;
            }
            }
        }
        switch(texture->width) {
        case 128:
            BlitSpan128(dest, src, width, u, v, du, dv);
        case 64:
            BlitSpan64(dest, src, width, u, v, du, dv);
            return;
        case 32:
            BlitSpan32(dest, src, width, u, v, du, dv);
            return;
        case 16:
            BlitSpan16(dest, src, width, u, v, du, dv);
            return;
        case 8: {
            int n=width;
            do {
                *dest++ = src[(u >> 16) + ((v >> 13) & ~7)];
                u += du; v += dv;
            } while (--n);
            return;
        }
        default: {
            int n=width;
            do {
                *dest++ = src[(u >> 16) + (v >> 16) * texture->width];
                u += du; v += dv;
            } while (--n);
            return;
        }
        }
    }
}

// FUNCTION: 0x4c8760
void __stdcall DrawTexturedPolygon(GafFrame* target, GafFrame* texture, int* vertices, int* coords)
{
    int i, defaults[8];
    int spans[800][10];
    int y0;
    int dv, dx, du;
    int lowY, highY, highX, lowX;
    int lowIndex, highIndex;
    if (target && texture && vertices) {
            if (!coords) {
                coords=defaults;
                defaults[0]=0; defaults[1]=0;
                defaults[2]=texture->width-1; defaults[3]=0;
                defaults[4]=texture->width-1; defaults[5]=texture->height-1;
                defaults[6]=0; defaults[7]=texture->height-1;
            }
            lowY=999999; highX=-999999; highY=-999999; lowX=999999;
            i = 0;
            while (i<4) {
                int y=vertices[i*3+1];
                if(y<lowY) { lowY=y; lowIndex=i; }
                if(y>highY) { highY=y; highIndex=i; }
                int x=vertices[i*3];
                if(x>highX) highX=x;
                if(x<lowX) lowX=x;
                i = i + 1;
            }
            if (highX>=0 && lowX<=target->width-1 && highY>=0) {
                if (lowY<=target->height-1) {
                    int bottom=target->height-1;
                    if(lowY<0) lowY=0;
                    if(highY>bottom) highY=bottom;
                    if(highY!=lowY) {
                        int y1; int x;
                        int* out;
                        int* nextVertex;
                        {
                            out=&spans[0][0];
                            int index = lowIndex;
                            do {
                                int next=index-1;
                                if (next<0) next=3;
                                int* currentVertex=vertices+index*3;
                                y0=currentVertex[1];
                                nextVertex=vertices+next*3;
                                y1=nextVertex[1];
                                if (y0<y1) {
                                    int dy = y1-y0;
                                    dx=((nextVertex[0]-currentVertex[0])*0x10000)/dy;
                                    x=currentVertex[0]*0x10000+0xffff;
                                    int z=currentVertex[2]*0x10000;
                                    int u=coords[index*2]*0x10000;
                                    int v=coords[index*2+1]*0x10000;
                                    du=(coords[next*2]*0x10000-u)/dy;
                                    dv=(coords[next*2+1]*0x10000-v)/dy;
                                    int dz = (nextVertex[2]*0x10000-z)/dy;
                                    if(y0<0) {
                                        x-=y0*dx; u-=y0*du; v-=y0*dv; z-=y0*dz;
                                        y0=0;
                                    }
                                    if(y1>bottom) y1=bottom;
                                    if(y0<y1) {
                                        int n=y1-y0;
                                        do {
                                            out[0]=x>>16; out[2]=u;
                                            x+=dx;
                                            out[3]=v;
                                            out[6]=z;
                                            u+=du;
                                            v+=dv; out+=10;
                                            z+=dz;
                                        } while(--n);
                                    }
                                }
                                index--;
                                if(index<0) index=3;
                            } while(index!=highIndex);
                        }
                        {
                            out=&spans[0][0];
                            int index = lowIndex;
                            do {
                                int next=(index+1)&3;
                                int* currentVertex=vertices+index*3;
                                y0=currentVertex[1];
                                nextVertex=vertices+next*3;
                                y1=nextVertex[1];
                                if (y0<y1) {
                                    int dy = y1-y0;
                                    dx=((nextVertex[0]-currentVertex[0])*0x10000)/dy;
                                    x=currentVertex[0]*0x10000+0xffff;
                                    int z=currentVertex[2]*0x10000;
                                    int u=coords[index*2]*0x10000;
                                    int v=coords[index*2+1]*0x10000;
                                    du=(coords[next*2]*0x10000-u)/dy;
                                    dv=(coords[next*2+1]*0x10000-v)/dy;
                                    int dz = (nextVertex[2]*0x10000-z)/dy;
                                    if (y0 < 0) {
                                        x-=dx*y0; u-=du*y0; v-=dv*y0; z-=dz*y0;
                                        y0=0;
                                    }
                                    if(y1>bottom) y1=bottom;
                                    if(y0<y1) {
                                        int n=y1-y0;
                                        do {
                                            out[1]=x>>16; x+=dx;
                                            out[4]=u;
                                            out[5]=v;
                                            out[7]=z;
                                            u+=du;
                                            v+=dv; z+=dz;
                                            out+=10;
                                        } while(--n);
                                    }
                                }
                                index=(index+1)&3;
                            } while(index!=highIndex);
                        }
                        int* span=&spans[0][0];
                        for(int row=lowY;row<highY;row++) {
                            if(span[1]-span[0]>0)
                                DrawTexturedSpan(row,span,target,texture);
                            span+=10;
                        }
                    }
                }
            }
        }
}

// FUNCTION: 0x4c8bb0
void __stdcall DrawLitTexturedPolygon(GafFrame* target, GafFrame* texture, int* vertices, int* coords)
{
    int i, defaults[8];
    int spans[800][10];
    int y0;
    int dv, dx, du, dz, dl;
    int lowY, highY, highX, lowX;
    int lowIndex, highIndex;
    if (target && texture && vertices) {
            if (!coords) {
                coords=defaults;
                defaults[0]=0; defaults[1]=0;
                defaults[2]=texture->width-1; defaults[3]=0;
                defaults[4]=texture->width-1; defaults[5]=texture->height-1;
                defaults[6]=0; defaults[7]=texture->height-1;
            }
            lowY=999999; highX=-999999; highY=-999999; lowX=999999;
            i = 0;
            while (i<4) {
                int y=vertices[i*4+1];
                if(y<lowY) { lowY=y; lowIndex=i; }
                if(y>highY) { highY=y; highIndex=i; }
                int x=vertices[i*4];
                if(x>highX) highX=x;
                if(x<lowX) lowX=x;
                i = i + 1;
            }
            if (highX>=0 && lowX<=target->width-1 && highY>=0) {
                if (lowY<=target->height-1) {
                    int bottom=target->height-1;
                    if(lowY<0) lowY=0;
                    if(highY>bottom) highY=bottom;
                    if(highY!=lowY) {
                        int y1; int x;
                        int* out;
                        int* nextVertex;
                        int next;
                        {
                            out=&spans[0][0];
                            int index = lowIndex;
                            do {
                                next=index-1;
                                if (next<0) next=3;
                                y0=vertices[index*4+1];
                                nextVertex=vertices+next*4;
                                y1=vertices[next*4+1];
                                if (y1>0 && y0<y1) {
                                    int dy = y1-y0;
                                    dx=((nextVertex[0]-vertices[index*4+0])*0x10000)/dy;
                                    x=vertices[index*4+0]*0x10000+0xffff;
                                    int u=coords[index*2]*0x10000;
                                    int v=coords[index*2+1]*0x10000;
                                    int z=vertices[index*4+2]*0x10000;
                                    int l=vertices[index*4+3]*0x10000;
                                    du=(coords[next*2]*0x10000-u)/dy;
                                    dv=(coords[next*2+1]*0x10000-v)/dy;
                                    dz=(nextVertex[2]*0x10000-z)/dy;
                                    dl=(nextVertex[3]*0x10000-l)/dy;
                                    if(y0<0) {
                                        x-=dx*y0; u-=du*y0; v-=dv*y0; z-=dz*y0; l-=dl*y0;
                                        y0=0;
                                    }
                                    if(y1>bottom) y1=bottom;
                                    if(y0<y1) {
                                        int n=y1-y0;
                                        do {
                                            out[0]=x>>16; x+=dx; out[2]=u; u+=du; out[3]=v; v+=dv; out[6]=z; out[8]=l; z+=dz; out+=10; l+=dl;
                                        } while(--n);
                                    }
                                }
                                index--;
                                if(index<0) index=3;
                            } while(index!=highIndex);
                        }
                        {
                            out=&spans[0][0];
                            int index = lowIndex;
                            do {
                                next=(index+1)&3;
                                y0=vertices[index*4+1];
                                nextVertex=vertices+next*4;
                                y1=vertices[next*4+1];
                                if (y1>0 && y0<y1) {
                                    int dy = y1-y0;
                                    dx=((nextVertex[0]-vertices[index*4+0])*0x10000)/dy;
                                    x=vertices[index*4+0]*0x10000+0xffff;
                                    int u=coords[index*2]*0x10000;
                                    int v=coords[index*2+1]*0x10000;
                                    int z=vertices[index*4+2]*0x10000;
                                    int l=vertices[index*4+3]*0x10000;
                                    du=(coords[next*2]*0x10000-u)/dy;
                                    dv=(coords[next*2+1]*0x10000-v)/dy;
                                    dz=(nextVertex[2]*0x10000-z)/dy;
                                    dl=(nextVertex[3]*0x10000-l)/dy;
                                    if(y0<0) {
                                        x-=dx*y0; u-=du*y0; v-=dv*y0; z-=dz*y0; l-=dl*y0;
                                        y0=0;
                                    }
                                    if(y1>bottom) y1=bottom;
                                    if(y0<y1) {
                                        int n=y1-y0;
                                        do {
                                            out[1]=x>>16; x+=dx; out[4]=u; u+=du; out[5]=v; out[7]=z; v+=dv; out[9]=l; z+=dz; out+=10; l+=dl;
                                        } while(--n);
                                    }
                                }
                                index=(index+1)&3;
                            } while(index!=highIndex);
                        }
                        int* span=&spans[0][0];
                        for(int row=lowY;row<highY;row++) {
                            if(span[1]-span[0]>0)
                                DrawLitTexturedSpan(row,span,target,texture);
                            span+=10;
                        }
                    }
                }
            }
        }
}

class StringRefAppendView {
public:
    char* ptr;

    bool IsEmpty() const { return *ptr == 0; }

    StringRefAppendView* Append(const StringRefAppendView& other);
};

// FUNCTION: 0x4c90b0
StringRefAppendView* StringRefAppendView::Append(const StringRefAppendView& other)
{
    if (!other.IsEmpty()) {
        char* chars;
        int n = (int)strlen(ptr);
        int m = (int)strlen(other.ptr);
        int len = n + m + 1;
        int* lp = &len;
        *lp += 5;
        int* block = (int*)malloc(len);
        *block = 1;
        chars = (char*)(block + 1);
        strcpy(chars, ptr);
        strcpy(chars + n, other.ptr);
        ((int*)ptr)[-1]--;
        int* old = (int*)ptr - 1;
        if (((int*)ptr)[-1] == 0) {
            free(old);
        }
        ptr = chars;
    }
    return this;
}

#include "../util/string_ref.h"

// FUNCTION: 0x4c9180 ??0StringRef@@QAE@XZ
StringRef::StringRef()
{
    g_emptyStringRep.refs++;
    data = (char*)&g_emptyStringRep.chars;
}

// FUNCTION: 0x4c91a0 ??0StringRef@@QAE@ABV0@@Z
StringRef::StringRef(const StringRef& other)
{
    data = other.data;
    ((int*)data)[-1]++;
}

// FUNCTION: 0x4c91b0 ??0StringRef@@QAE@PBD@Z
StringRef::StringRef(const char* text)
{
    char* chars;
    if (text == 0 || *text == 0) {
        g_emptyStringRep.refs++;
        chars = (char*)&g_emptyStringRep.chars;
    } else {
        int* block = (int*)malloc(strlen(text) + 1 + sizeof(int));
        *block = 1;
        chars = (char*)(block + 1);
        strcpy(chars, text);
    }
    data = chars;
}

// FUNCTION: 0x4c9230 ??0StringRef@@QAE@PBDH@Z
StringRef::StringRef(const char* text, int len)
{
    if (text == 0) {
        g_emptyStringRep.refs++;
        data = (char*)&g_emptyStringRep.chars;
    } else {
        int* block = (int*)malloc(len + 1 + sizeof(int));
        *block = 1;
        char* chars = (char*)(block + 1);
        strncpy(chars, text, len);
        chars[len] = 0;
        data = chars;
    }
}

// FUNCTION: 0x4c9290
StringRef* StringRef::MakeLower()
{
    _strlwr(GetUnique());
    return this;
}

// FUNCTION: 0x4c9310
StringRef* StringRef::MakeUpper()
{
    _strupr(GetUnique());
    return this;
}

extern "C" void __cdecl free(void*);

// FUNCTION: 0x4c9390
void StringRef::ReleaseRef()
{
    ((int*)data)[-1]--;
    int* p = (int*)data - 1;
    if (((int*)data)[-1] == 0) {
        free(p);
    }
}

// FUNCTION: 0x4c93f0
StringRef* StringRef::AssignText(const char* text)
{
    ((int*)data)[-1]--;
    int* old = (int*)data - 1;
    if (((int*)data)[-1] == 0)
        free(old);
    char* chars;
    if (text == 0 || *text == 0) {
        g_emptyStringRep.refs++;
        chars = (char*)&g_emptyStringRep.chars;
    } else {
        int* block = (int*)malloc(strlen(text) + 1 + sizeof(int));
        *block = 1;
        chars = (char*)(block + 1);
        strcpy(chars, text);
    }
    data = chars;
    return this;
}

// FUNCTION: 0x4c9490
StringRef StringRef::SubString(int start, int end) const
{
    int len = strlen(data);
    if (start < 0)
        start = 0;
    if (end > len)
        end = len;
    if (start >= end)
        return StringRef();
    return StringRef(data + start, end - start);
}
