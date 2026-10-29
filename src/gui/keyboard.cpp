#include "win_types.h"
#include "../graphics/display_context.h"
#include <string.h>
#include "../../interfaces/host.h"

#pragma pack(push, 2)
struct KeyQueueDisplay {
    char unknown_0[0xf2];
    int size;
    int entries[30];
    int head;
    int tail;
};
#pragma pack(pop)


KeyQueueDisplay* GetDisplay(void);

extern int g_gfxBlitLockHeld;
extern HANDLE g_gfxBlitLockEvent;

// FUNCTION: 0x4c1a40
void ClearKeyQueue(void)
{
    KeyQueueDisplay* q = GetDisplay();
    q->head = 0;
    q->tail = 0;
}

// FUNCTION: 0x4c1a60
void __stdcall InitKeyQueue(int size)
{
    KeyQueueDisplay* q = GetDisplay();
    if (size <= 0x1e)
        q->size = size;
    else
        q->size = 0x1e;

    q = GetDisplay();
    q->head = 0;
    q->tail = 0;
}

// FUNCTION: 0x4c1aa0
void DestroyPreCleanup(void)
{
}

// FUNCTION: 0x4c1ab0
int PopKey(void)
{
    KeyQueueDisplay* q = GetDisplay();
    int v;
    if (q->head == q->tail) {
        v = 0;
    } else {
        int next = q->tail + 1;
        v = q->entries[next - 1];
        q->tail++;
        if (q->tail == q->size)
            q->tail = 0;
    }
    return v;
}

// FUNCTION: 0x4c1b00
int __cdecl PeekKey()
{
    KeyQueueDisplay* q = GetDisplay();
    int head = q->head;
    int tail = q->tail;
    if (head == tail) {
        return 0;
    }
    return q->entries[tail];
}

#pragma auto_inline(off)
// FUNCTION: 0x4c1b20
void __stdcall PushKeyCode(int v)
{
    KeyQueueDisplay* q = GetDisplay();
    int next = q->head + 1;
    if ((q->head + 1) % q->size != q->tail) {
        q->entries[next - 1] = v;
        q->head++;
        if (q->head == q->size)
            q->head = 0;
    }
}
#pragma auto_inline(on)

static short key_state(int vk)
{
    return (short)(host_key_state(vk) & 0xfffe);
}

// FUNCTION: 0x4c1b80
int __stdcall IsKeyDown(int key)
{
    switch (key)
    {
    case 0xfb:  return key_state(0x12) != 0;
    case 0xf9:  return key_state(0x10) != 0;
    case 0xfa:  return key_state(0x11) != 0;
    case 0xf5:  return key_state(0x26) != 0;
    case 0xf7:  return key_state(0x28) != 0;
    case 0xf4:  return key_state(0x25) != 0;
    case 0xf6:  return key_state(0x27) != 0;
    case 0x20:  return key_state(0x20) != 0;
    }
    return 0;
}

static inline void PushKey(int v)
{
    KeyQueueDisplay* q = GetDisplay();
    int next = q->head + 1;
    if ((q->head + 1) % q->size == q->tail)
        return;
    q->entries[next - 1] = v;
    q->head++;
    if (q->head == q->size)
        q->head = 0;
}

// FUNCTION: 0x4c1d50
void __stdcall HandleVirtualKey(int key, int flag)
{
    int ctrl = key_state(VK_CONTROL) != 0;

    switch (key) {
    case VK_UP:     PushKey(0xf5); return;
    case VK_DOWN:   PushKey(0xf7); return;
    case VK_LEFT:   PushKey(0xf4); return;
    case VK_RIGHT:  PushKey(0xf6); return;
    case VK_INSERT: PushKey(0xee); return;
    case VK_DELETE: PushKey(0xef); return;
    case VK_HOME:   PushKey(0xf0); return;
    case VK_END:    PushKey(0xf1); return;
    case VK_PRIOR:  PushKey(0xf2); return;
    case VK_NEXT:   PushKey(0xf3); return;
    case VK_PAUSE:  PushKeyCode(0xf8); return;
    case VK_F1:     PushKeyCode(ctrl ? 0xce : 0xe2); return;
    case VK_F2:     PushKeyCode(ctrl ? 0xcf : 0xe3); return;
    case VK_F3:     PushKeyCode(ctrl ? 0xd0 : 0xe4); return;
    case VK_F4:     PushKeyCode(ctrl ? 0xd1 : 0xe5); return;
    case VK_F5:     PushKeyCode(ctrl ? 0xd2 : 0xe6); return;
    case VK_F6:     PushKeyCode(ctrl ? 0xd3 : 0xe7); return;
    case VK_F7:     PushKeyCode(ctrl ? 0xd4 : 0xe8); return;
    case VK_F8:     PushKeyCode(ctrl ? 0xd5 : 0xe9); return;
    case VK_F9:     PushKeyCode(ctrl ? 0xd6 : 0xea); return;
    case VK_F10:    PushKeyCode(ctrl ? 0xd7 : 0xeb); return;
    case VK_F11:    PushKeyCode(ctrl ? 0xd8 : 0xec); return;
    case VK_F12:    PushKeyCode(ctrl ? 0xd9 : 0xed); return;
    }

    if (flag == 0 && ctrl == 0)
        return;

    if (key >= '0' && key <= '9') {
        if (ctrl)
            PushKeyCode(key + 0x94);
        else
            PushKeyCode(key);
        return;
    }
    if (key >= 'A' && key <= 'Z') {
        if (ctrl)
            PushKeyCode(key + 0x69);
        else
            PushKeyCode(key + 0x20);
        return;
    }
    if (key >= 0xba && key <= 0xc0) {
        char tbl[7] = {0x3b, 0x3d, 0x2c, 0x2d, 0x2e, 0x2f, 0x60};
        PushKeyCode(tbl[key - 0xba]);
        return;
    }
    if (key >= 0xdb && key <= 0xde) {
        char tbl[4] = {0x5b, 0x5c, 0x5d, 0x27};
        PushKeyCode(tbl[key - 0xdb]);
        return;
    }
}

// FUNCTION: 0x4c22a0
void InitBlitLock()
{
    g_gfxBlitLockHeld = 0;
    if (g_gfxBlitLockEvent) {
        host_event_reset(g_gfxBlitLockEvent);
        return;
    }
    g_gfxBlitLockEvent = host_event_create(0, 0);
}

// FUNCTION: 0x4c22d0
void __stdcall SetCursorOverlayEnabled(int param)
{
    DisplayContext* obj = (DisplayContext*)GetDisplay();
    obj->cursorOverlayEnabled = param;
}

// FUNCTION: 0x4c22f0
void __stdcall SetCursorPosition(int x, int y)
{
    host_cursor_set(x, y);
}

// FUNCTION: 0x4c2310
void __stdcall GetCursorPosition(int* x, int* y)
{
    POINT pt;
    host_cursor_get((int*)&pt.x, (int*)&pt.y);
    *x = pt.x;
    *y = pt.y;
}

#include "../../include/mouse_event.h"

// FUNCTION: 0x4c2340
void __stdcall GetCurrentMouseEvent(MouseEvent* event)
{
    int ptr = (int)GetDisplay();
    memcpy(event, (void*)(ptr + 0x196), 6 * 4);
}

// FUNCTION: 0x4c2360
void __stdcall SetCurrentMouseEvent(MouseEvent* event)
{
    int eax = (int)GetDisplay();
    int edi = eax + 0x196;
    memcpy((void*)edi, event, 24);
}
