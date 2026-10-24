#include "win_types.h"
#include <string.h>
#include "../../interfaces/host.h"

#include "../graphics/gaf_frame.h"

#pragma pack(push, 2)

struct Desc_004c25e0 {
    int width;
    int height;
    int pitch;
};

struct Event_4c2d60 {
    int x;
    int y;
    int data[4];
};

struct Surface_004c23e0 {
    int data[12];
};

struct Obj_004c2380 {
    char unknown_0[0xf0];
    unsigned short bit0_9 : 10;
    unsigned short active : 1;
    unsigned short bit11_15 : 5;
    char unknown_f2[0x186 - 0xf2];
    int capacity;
    Event_4c2d60* entries;
    int head;
    int tail;
    Event_4c2d60 rect;
    int count;
    GafFrame* sprite;
    int x;
    int y;
    Desc_004c25e0* saved;
    Desc_004c25e0* under;
    Desc_004c25e0* work;
    int running;
    int mode;
    int visible;
    int pending;
};

#pragma pack(pop)

Obj_004c2380* GetDisplay(void);

#include "../../link/globals.h"

void __cdecl BlitSurface(void* dst, void* src, int x, int y);
void __stdcall DrawFrame(void* dst, GafFrame* bmp, int x, int y);
void __stdcall DrawSurface(Desc_004c25e0* dst, Desc_004c25e0* bmp, int x, int y);
#include "../graphics/surface.h"
void __stdcall ResetClipRect(void* d);
void* __stdcall AllocSurface(char* name, int width, int height);
#include "../debug/debug_lib.h"
#include "../graphics/object_pool.h"

void __stdcall RedrawMouseCursor(Obj_004c2380* app);
#include "mouse.h"

static inline void GetRect(Event_4c2d60* out)
{
    Obj_004c2380* p = GetDisplay();
    memcpy(out, &p->rect, sizeof(Event_4c2d60));
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
    Obj_004c2380* obj = GetDisplay();
    if (obj->mode != 1 && obj->count <= 0) {
        Event_4c2d60 r;
        GetRect(&r);
        DrawFrame(0, obj->sprite, r.x, r.y);
    }
}

// FUNCTION: 0x4c23e0
void __stdcall RestoreCursorBackground(Obj_004c2380* obj)
{
    if (obj->visible) {
        Desc_004c25e0* bmp = obj->saved;
        Surface_004c23e0 s;
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
    Obj_004c2380* o = GetDisplay();
    if (o->mode != 1 && o->count++ == 0)
        DrawSurface(0, o->saved, o->x, o->y);
}

// FUNCTION: 0x4c24b0
void __stdcall CaptureBackgroundAndDrawCursor(Obj_004c2380* obj)
{
    if (obj->visible) {
        Surface_004c23e0 s;
        if (LockPrimary((Surface *)&s)) {
            RECT r;
            POINT pt;
            host_cursor_get((int*)&pt.x, (int*)&pt.y);
            obj->rect.x = pt.x;
            obj->rect.y = pt.y;
            obj->x = pt.x - obj->sprite->xOffset;
            obj->y = pt.y - obj->sprite->yOffset;
            obj->saved->width = obj->sprite->width;
            obj->saved->height = obj->sprite->height;
            obj->saved->pitch = obj->sprite->width;
            BlitSurface(obj->saved, &s, -obj->x, -obj->y);
            DrawFrame(&s, obj->sprite, obj->rect.x, obj->rect.y);
            r.left = obj->x;
            r.top = obj->y;
            r.right = r.left + obj->sprite->width;
            r.bottom = r.top + obj->sprite->height;
            UnlockPrimary((Surface *)&s, &r, 0);
        }
    }
}

// FUNCTION: 0x4c25e0
void __stdcall RedrawMouseCursor(Obj_004c2380* app)
{
    Surface_004c23e0 info;
    RECT r1;
    RECT r2;
    POINT pt;
    if (app->visible == 0)
        return;
    if (LockPrimary((Surface *)&info) == 0)
        return;
    host_cursor_get((int*)&pt.x, (int*)&pt.y);
    int x = pt.x;
    int y = pt.y;
    app->rect.x = x;
    app->rect.y = y;
    x -= app->sprite->xOffset;
    y -= app->sprite->yOffset;
    app->under->width = app->sprite->width;
    app->under->height = app->sprite->height;
    app->under->pitch = app->sprite->width;
    app->work->width = app->sprite->width;
    app->work->height = app->sprite->height;
    app->work->pitch = app->sprite->width;
    BlitSurface(app->under, &info, -x, -y);
    BlitSurface(app->under, app->saved, app->x - x, app->y - y);
    BlitSurface(app->work, app->under, 0, 0);
    ResetClipRect(app->work);
    DrawFrame(app->work, app->sprite, app->sprite->xOffset, app->sprite->yOffset);
    BlitSurface(app->saved, app->work, x - app->x, y - app->y);
    r1.left = app->x;
    r1.top = app->y;
    r1.right = r1.left + app->saved->width;
    r1.bottom = app->saved->height + r1.top;
    r2.left = x;
    r2.top = y;
    r2.right = x + app->sprite->width;
    r2.bottom = y + app->sprite->height;
    BlitSurface(&info, app->saved, app->x, app->y);
    BlitSurface(&info, app->work, x, y);
    app->saved->width = app->sprite->width;
    app->saved->height = app->sprite->height;
    app->saved->pitch = app->sprite->width;
    BlitSurface(app->saved, app->under, 0, 0);
    app->x = x;
    app->y = y;
    UnlockPrimary((Surface *)&info, &r1, &r2);
}

// FUNCTION: 0x4c2870
void __fastcall ShowSoftwareCursor(void)
{
    Obj_004c2380* o = GetDisplay();
    if (o->mode != 1) {
        if (--o->count <= 0) {
            POINT pt;
            host_cursor_get((int*)&pt.x, (int*)&pt.y);
            o->rect.x = pt.x;
            o->rect.y = pt.y;
            if (o->sprite != 0) {
                o->x = pt.x - o->sprite->xOffset;
                o->y = pt.y - o->sprite->yOffset;
                o->saved->width = o->sprite->width;
                o->saved->height = o->sprite->height;
                o->saved->pitch = o->sprite->width;
            }
            DrawSurface(o->saved, 0, -o->x, -o->y);
            Obj_004c2380* o2 = GetDisplay();
            if (o2->mode != 1 && o2->count <= 0) {
                Event_4c2d60 r;
                GetRect(&r);
                DrawFrame(0, o2->sprite, r.x, r.y);
            }
        }
    }
}

// FUNCTION: 0x4c2990
void __cdecl MouseThreadProc(int param)
{
    Obj_004c2380* o = (Obj_004c2380*)param;
    host_thread_set_priority(2);
    while (o->pending == 0) {
        int start = host_time_ms() + 0x21;
        LONG held = LockMouse();
        if (o->sprite != 0)
            RedrawMouseCursor(o);
        UnlockMouse(held);
        host_sleep_ms(max(1, start - (int)host_time_ms()));
    }
    o->pending = 0;
}

// FUNCTION: 0x4c2a80
int __stdcall StartMouseThread(Obj_004c2380* s)
{
    s->pending = 0;
    s->running = host_thread_start((void (__cdecl*)(void*))MouseThreadProc, 0x8000, s);
    if (s->running) {
        s->mode = 1;
        return 1;
    }
    return 0;
}

// FUNCTION: 0x4c2ac0
int __stdcall StopMouseThread(Obj_004c2380* s)
{
    if (s->running == 0)
        return 1;
    s->pending = 1;
    int tries = 0;
    while (s->pending != 0) {
        if (++tries > 20)
            return 0;
        host_sleep_ms(100);
    }
    s->running = 0;
    return 1;
}

// FUNCTION: 0x4c2b20
void __stdcall SetCursorSprite(int value)
{
    LONG held = LockMain();
    GetDisplay()->sprite = (GafFrame*)value;
    UnlockMain(held);
}

// FUNCTION: 0x4c2ba0
int GetCursorSprite()
{
    return (int)GetDisplay()->sprite;
}

// FUNCTION: 0x4c2bb0
void ClearMouseEventQueue()
{
    Obj_004c2380* p = GetDisplay();
    p->head = 0;
    p->tail = 0;
}

// FUNCTION: 0x4c2bd0
void __stdcall InitMouse(int count, int start)
{
    Obj_004c2380* p = (Obj_004c2380*)GetDisplay();
    p->capacity = count;
    p->entries = (Event_4c2d60*)GameAllocIgnoreTag("MOUSE EVENTS", count * 0x18);
    Obj_004c2380* q = (Obj_004c2380*)GetDisplay();
    q->head = 0;
    q->tail = 0;
    p->sprite = 0;
    p->saved = (Desc_004c25e0*)AllocSurface("SAVEMOUSE 1", 0x640, 1);
    p->under = (Desc_004c25e0*)AllocSurface("SAVEMOUSE 2", 0x640, 1);
    p->work = (Desc_004c25e0*)AllocSurface("SAVEMOUSE 3", 0x640, 1);
    p->count = 1;
    p->visible = 0;
    p->mode = 0;
    p->pending = 0;
    if (start == 1) {
        p->pending = 0;
        p->running = host_thread_start((void (__cdecl*)(void*))MouseThreadProc, 0x8000, p);
        if (p->running != 0)
            p->mode = 1;
    } else {
        p->running = 0;
    }
}

// FUNCTION: 0x4c2cc0
void ShutdownMouse()
{
    Obj_004c2380* o = GetDisplay();
    if (o->entries) {
        if (o->active) {
            if (o->running) {
                o->pending = 1;
                int i = 0;
                while (1) {
                    if (++i > 20)
                        break;
                    host_sleep_ms(100);
                    if (!o->pending) {
                        o->running = 0;
                        break;
                    }
                }
            }
        }
        FreeSurface((Surface *)o->work);
        FreeSurface((Surface *)o->under);
        FreeSurface((Surface *)o->saved);
        GameFreeThunk(o->entries);
        o->entries = 0;
    }
}

// FUNCTION: 0x4c2d60
int __stdcall PopMouseEvent(Event_4c2d60* out)
{
    Obj_004c2380* q = GetDisplay();
    if (q->head == q->tail) {
        *out = q->rect;
        return 0;
    }
    *out = q->entries[q->tail];
    if (++q->tail == q->capacity) {
        q->tail = 0;
    }
    return 1;
}

// FUNCTION: 0x4c2de0
int __stdcall PeekMouseEvent(Event_4c2d60* out)
{
    Obj_004c2380* q = GetDisplay();
    if (q->head == q->tail) {
        *out = q->rect;
        return 0;
    }
    *out = q->entries[q->tail];
    return 1;
}

// FUNCTION: 0x4c2e30
void __stdcall PushMouseEvent(Event_4c2d60* ev)
{
    Obj_004c2380* q = GetDisplay();
    if ((q->head + 1) % q->capacity != q->tail) {
        q->entries[q->head] = *ev;
        if (++q->head == q->capacity) {
            q->head = 0;
        }
    }
}
