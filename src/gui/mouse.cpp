#include "win_types.h"
#include <string.h>
#include "../../interfaces/host.h"

#include "../graphics/gaf_frame.h"
#include "../util/mouse_event.h"

#pragma pack(push, 2)


struct RestoreCursorBackgroundSurface {
    int data[12];
};


#pragma pack(pop)

#include "../graphics/display.h"
#include "../graphics/surface.h"

#include "../../link/globals.h"

void __cdecl BlitSurface(void* dst, void* src, int x, int y);
void __stdcall DrawFrame(void* dst, GafFrame* bmp, int x, int y);
void __stdcall DrawSurface(Surface* dst, Surface* bmp, int x, int y);
void __stdcall ResetClipRect(void* d);
Surface* __stdcall AllocSurface(char* name, int width, int height);
#include "../debug/debug_lib.h"
#include "../graphics/object_pool.h"

#include "mouse.h"
#include "mouse.h"

static inline void GetRect(MouseEvent* out)
{
    Display* p = GetDisplay();
    memcpy(out, &p->mouseEvent, sizeof(MouseEvent));
}

static inline LONG LockMouse()
{
    while (1) {
        LONG r = host_atomic_exchange((int*)&g_gfxBlitLockHeld, 0x4d4f5553);
        if (r == 0) {
            g_gfxBlitLockOwner = 0x4d4f5553;
            return 0;
        }
        if (g_gfxBlitLockOwner == 0x4d4f5553)
            return r;
        host_event_wait(g_gfxBlitLockEvent, HOST_WAIT_FOREVER);
    }
}

static inline void UnlockMouse(LONG held)
{
    if (held == 0) {
        g_gfxBlitLockOwner = 0;
        host_atomic_exchange((int*)&g_gfxBlitLockHeld, 0);
        host_event_set(g_gfxBlitLockEvent);
    }
}

static inline LONG LockMain()
{
    while (1) {
        LONG r = host_atomic_exchange((int*)&g_gfxBlitLockHeld, 0x4d41494e);
        if (r == 0) {
            g_gfxBlitLockOwner = 0x4d41494e;
            return 0;
        }
        if (g_gfxBlitLockOwner == 0x4d41494e)
            return r;
        host_event_wait(g_gfxBlitLockEvent, HOST_WAIT_FOREVER);
    }
}

static inline void UnlockMain(LONG held)
{
    if (held == 0) {
        g_gfxBlitLockOwner = 0;
        host_atomic_exchange((int*)&g_gfxBlitLockHeld, 0);
        host_event_set(g_gfxBlitLockEvent);
    }
}

// FUNCTION: 0x4c2380
void DrawSoftwareCursor()
{
    Display* obj = GetDisplay();
    if (obj->cursorThreadEnabled != 1 && obj->cursorHideCount <= 0) {
        MouseEvent r;
        GetRect(&r);
        DrawFrame(0, obj->cursorSprite, r.x, r.y);
    }
}

// FUNCTION: 0x4c23e0
void __stdcall RestoreCursorBackground(Display* obj)
{
    if (obj->cursorOverlayEnabled) {
        Surface* bmp = obj->saveMouse1;
        RestoreCursorBackgroundSurface s;
        if (LockPrimary((Surface *)&s)) {
            BlitSurface(&s, bmp, obj->x, obj->y);
            RECT r;
            r.left = obj->x;
            r.top = obj->y;
            r.right = r.left + bmp->width;
            r.bottom = r.top + bmp->height;
            UnlockPrimary((Surface *)&s, &r, 0);
        }
    }
}

// FUNCTION: 0x4c2470
void HideSoftwareCursor()
{
    Display* o = GetDisplay();
    if (o->cursorThreadEnabled != 1 && o->cursorHideCount++ == 0)
        DrawSurface(0, o->saveMouse1, o->x, o->y);
}

// FUNCTION: 0x4c24b0
void __stdcall CaptureBackgroundAndDrawCursor(Display* obj)
{
    if (obj->cursorOverlayEnabled) {
        RestoreCursorBackgroundSurface s;
        if (LockPrimary((Surface *)&s)) {
            RECT r;
            POINT pt;
            host_cursor_get((int*)&pt.x, (int*)&pt.y);
            obj->mouseEvent.x = pt.x;
            obj->mouseEvent.y = pt.y;
            obj->x = pt.x - obj->cursorSprite->xOffset;
            obj->y = pt.y - obj->cursorSprite->yOffset;
            obj->saveMouse1->width = obj->cursorSprite->width;
            obj->saveMouse1->height = obj->cursorSprite->height;
            obj->saveMouse1->pitch = obj->cursorSprite->width;
            BlitSurface(obj->saveMouse1, &s, -obj->x, -obj->y);
            DrawFrame(&s, obj->cursorSprite, obj->mouseEvent.x, obj->mouseEvent.y);
            r.left = obj->x;
            r.top = obj->y;
            r.right = r.left + obj->cursorSprite->width;
            r.bottom = r.top + obj->cursorSprite->height;
            UnlockPrimary((Surface *)&s, &r, 0);
        }
    }
}

// FUNCTION: 0x4c25e0
void __stdcall RedrawMouseCursor(Display* app)
{
    RestoreCursorBackgroundSurface info;
    RECT r1;
    RECT r2;
    POINT pt;
    if (app->cursorOverlayEnabled == 0)
        return;
    if (LockPrimary((Surface *)&info) == 0)
        return;
    host_cursor_get((int*)&pt.x, (int*)&pt.y);
    int x = pt.x;
    int y = pt.y;
    app->mouseEvent.x = x;
    app->mouseEvent.y = y;
    x -= app->cursorSprite->xOffset;
    y -= app->cursorSprite->yOffset;
    app->saveMouse2->width = app->cursorSprite->width;
    app->saveMouse2->height = app->cursorSprite->height;
    app->saveMouse2->pitch = app->cursorSprite->width;
    app->saveMouse3->width = app->cursorSprite->width;
    app->saveMouse3->height = app->cursorSprite->height;
    app->saveMouse3->pitch = app->cursorSprite->width;
    BlitSurface(app->saveMouse2, &info, -x, -y);
    BlitSurface(app->saveMouse2, app->saveMouse1, app->x - x, app->y - y);
    BlitSurface(app->saveMouse3, app->saveMouse2, 0, 0);
    ResetClipRect(app->saveMouse3);
    DrawFrame(app->saveMouse3, app->cursorSprite, app->cursorSprite->xOffset, app->cursorSprite->yOffset);
    BlitSurface(app->saveMouse1, app->saveMouse3, x - app->x, y - app->y);
    r1.left = app->x;
    r1.top = app->y;
    r1.right = r1.left + app->saveMouse1->width;
    r1.bottom = app->saveMouse1->height + r1.top;
    r2.left = x;
    r2.top = y;
    r2.right = x + app->cursorSprite->width;
    r2.bottom = y + app->cursorSprite->height;
    BlitSurface(&info, app->saveMouse1, app->x, app->y);
    BlitSurface(&info, app->saveMouse3, x, y);
    app->saveMouse1->width = app->cursorSprite->width;
    app->saveMouse1->height = app->cursorSprite->height;
    app->saveMouse1->pitch = app->cursorSprite->width;
    BlitSurface(app->saveMouse1, app->saveMouse2, 0, 0);
    app->x = x;
    app->y = y;
    UnlockPrimary((Surface *)&info, &r1, &r2);
}

// FUNCTION: 0x4c2870
void __fastcall ShowSoftwareCursor(void)
{
    Display* o = GetDisplay();
    if (o->cursorThreadEnabled != 1) {
        if (--o->cursorHideCount <= 0) {
            POINT pt;
            host_cursor_get((int*)&pt.x, (int*)&pt.y);
            o->mouseEvent.x = pt.x;
            o->mouseEvent.y = pt.y;
            if (o->cursorSprite != 0) {
                o->x = pt.x - o->cursorSprite->xOffset;
                o->y = pt.y - o->cursorSprite->yOffset;
                o->saveMouse1->width = o->cursorSprite->width;
                o->saveMouse1->height = o->cursorSprite->height;
                o->saveMouse1->pitch = o->cursorSprite->width;
            }
            DrawSurface(o->saveMouse1, 0, -o->x, -o->y);
            Display* o2 = GetDisplay();
            if (o2->cursorThreadEnabled != 1 && o2->cursorHideCount <= 0) {
                MouseEvent r;
                GetRect(&r);
                DrawFrame(0, o2->cursorSprite, r.x, r.y);
            }
        }
    }
}

// FUNCTION: 0x4c2990
void __cdecl MouseThreadProc(int param)
{
    Display* o = (Display*)param;
    host_thread_set_priority(2);
    while (o->mouseThreadStop == 0) {
        int start = host_time_ms() + 0x21;
        LONG held = LockMouse();
        if (o->cursorSprite != 0)
            RedrawMouseCursor(o);
        UnlockMouse(held);
        host_sleep_ms(max(1, start - (int)host_time_ms()));
    }
    o->mouseThreadStop = 0;
}

// FUNCTION: 0x4c2a80
int __stdcall StartMouseThread(Display* s)
{
    s->mouseThreadStop = 0;
    s->mouseThread = host_thread_start((void (__cdecl*)(void*))MouseThreadProc, 0x8000, s);
    if (s->mouseThread) {
        s->cursorThreadEnabled = 1;
        return 1;
    }
    return 0;
}

// FUNCTION: 0x4c2ac0
int __stdcall StopMouseThread(Display* s)
{
    if (s->mouseThread == 0)
        return 1;
    s->mouseThreadStop = 1;
    int tries = 0;
    while (s->mouseThreadStop != 0) {
        if (++tries > 20)
            return 0;
        host_sleep_ms(100);
    }
    s->mouseThread = 0;
    return 1;
}

// FUNCTION: 0x4c2b20
void __stdcall SetCursorSprite(int value)
{
    LONG held = LockMain();
    GetDisplay()->cursorSprite = (GafFrame*)value;
    UnlockMain(held);
}

// FUNCTION: 0x4c2ba0
int GetCursorSprite()
{
    return (int)GetDisplay()->cursorSprite;
}

// FUNCTION: 0x4c2bb0
void ClearMouseEventQueue()
{
    Display* p = GetDisplay();
    p->mouseQueueHead = 0;
    p->mouseQueueTail = 0;
}

// FUNCTION: 0x4c2bd0
void __stdcall InitMouse(int count, int start)
{
    Display* p = GetDisplay();
    p->mouseQueueSize = count;
    p->mouseQueue = (MouseEvent*)GameAllocIgnoreTag("MOUSE EVENTS", count * 0x18);
    Display* q = GetDisplay();
    q->mouseQueueHead = 0;
    q->mouseQueueTail = 0;
    p->cursorSprite = 0;
    p->saveMouse1 = AllocSurface("SAVEMOUSE 1", 0x640, 1);
    p->saveMouse2 = AllocSurface("SAVEMOUSE 2", 0x640, 1);
    p->saveMouse3 = AllocSurface("SAVEMOUSE 3", 0x640, 1);
    p->cursorHideCount = 1;
    p->cursorOverlayEnabled = 0;
    p->cursorThreadEnabled = 0;
    p->mouseThreadStop = 0;
    if (start == 1) {
        p->mouseThreadStop = 0;
        p->mouseThread = host_thread_start((void (__cdecl*)(void*))MouseThreadProc, 0x8000, p);
        if (p->mouseThread != 0)
            p->cursorThreadEnabled = 1;
    } else {
        p->mouseThread = 0;
    }
}

// FUNCTION: 0x4c2cc0
void ShutdownMouse()
{
    Display* o = GetDisplay();
    if (o->mouseQueue) {
        if (o->flags.bits.soundOpt) {
            if (o->mouseThread) {
                o->mouseThreadStop = 1;
                int i = 0;
                while (1) {
                    if (++i > 20)
                        break;
                    host_sleep_ms(100);
                    if (!o->mouseThreadStop) {
                        o->mouseThread = 0;
                        break;
                    }
                }
            }
        }
        FreeSurface(o->saveMouse3);
        FreeSurface(o->saveMouse2);
        FreeSurface(o->saveMouse1);
        GameFreeThunk(o->mouseQueue);
        o->mouseQueue = 0;
    }
}

// FUNCTION: 0x4c2d60
int __stdcall PopMouseEvent(MouseEvent* out)
{
    Display* q = GetDisplay();
    if (q->mouseQueueHead == q->mouseQueueTail) {
        *out = q->mouseEvent;
        return 0;
    }
    *out = q->mouseQueue[q->mouseQueueTail];
    if (++q->mouseQueueTail == q->mouseQueueSize) {
        q->mouseQueueTail = 0;
    }
    return 1;
}

// FUNCTION: 0x4c2de0
int __stdcall PeekMouseEvent(MouseEvent* out)
{
    Display* q = GetDisplay();
    if (q->mouseQueueHead == q->mouseQueueTail) {
        *out = q->mouseEvent;
        return 0;
    }
    *out = q->mouseQueue[q->mouseQueueTail];
    return 1;
}

// FUNCTION: 0x4c2e30
void __stdcall PushMouseEvent(MouseEvent* ev)
{
    Display* q = GetDisplay();
    if ((q->mouseQueueHead + 1) % q->mouseQueueSize != q->mouseQueueTail) {
        q->mouseQueue[q->mouseQueueHead] = *ev;
        if (++q->mouseQueueHead == q->mouseQueueSize) {
            q->mouseQueueHead = 0;
        }
    }
}
