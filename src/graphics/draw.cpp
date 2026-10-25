#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "rect.h"

#include "../util/point.h"

#include "../util/vec3.h"

#include "surface.h"

#include "gaf_frame.h"

struct Span {
    int x1;
    int x2;
    char unknown_8[0x18 - 0x8];
    int z1;
    int z2;
    int s1;
    int s2;
};

struct Span_004c0330 {
    int left;
    int right;
    int unknown_8[8];
};

struct Point_004c0c70 {
    int x;
    int y;
    int z;
    int s;
};

struct Segment_004bf060 {
    Point* from;
    Point* to;
};

struct Engine_004bf4d0 {
    char unknown_0[0xc4];
    unsigned char* fade_neg;
    unsigned char* fade_pos;
    char unknown_cc[0xd4 - 0xcc];
    int width;
    int height;
};

struct App_004bec70 {
    char unknown_0[0xc8];
    unsigned int* palette;
};

struct App_004bed70 {
    char unknown_0[0xc0];
    unsigned int* palette;
};

struct App_004c0b10 {
    char unknown_0[0xc4];
    unsigned char* shade;
};

struct Game_004bfe10 {
    char unknown_0[0xcc];
    int grayTable;
    char unknown_d0[0xf1 - 0xd0];
    unsigned char flags;
};

struct Obj_004c13d0 {
    char unknown_0[0x210];
    int keyColor;
};

struct Obj {
    char unknown_0[0x208];
    int foreColor;
    int backColor;
    int keyColor;
};

int __stdcall LockScreen(Surface* out);
int __stdcall UnlockScreen(Surface* s);
#include "draw.h"
void __cdecl BlitLine(Surface* dst, int x0, int y0, int x1, int y1, int color);
void __cdecl BlitLineRemapped(Surface* dst, int x0, int y0, int x1, int y1,
                              int color, unsigned int* palette);
int __stdcall GetDisplay();
void __cdecl FillSolidRect(Surface* s, Rect* r, int color);
void __cdecl RemapRect(int dst, int pitch, int w, int h, int table);
void __cdecl XorRect(Surface* s, Rect* r, int value);
int __cdecl ScaledSin(int angle, int distance);
int __cdecl ScaledCos(int angle, int distance);
void __cdecl BlitRectKeyed(Surface* dst, Surface* src, Rect* rect, Rect* pos,
                           int colour);
void __cdecl BlitRect(Surface* dst, Surface* src, Rect* rect, Rect* pos);

#pragma auto_inline(off)
// FUNCTION: 0x4be950
int __stdcall DrawLine(Surface* surface, int x0, int y0, int x1, int y1,
                           int color)
{
    int ret;
    if (surface == 0) {
        Surface screen;
        ret = LockScreen(&screen);
        if (ret) {
            if (ClipLine(&screen, &x0, &y0, &x1, &y1))
                BlitLine(&screen, x0, y0, x1, y1, color);
            UnlockScreen(&screen);
        }
    } else {
        if (ClipLine(surface, &x0, &y0, &x1, &y1))
            BlitLine(surface, x0, y0, x1, y1, color);
        ret = 1;
    }
    return ret;
}
#pragma auto_inline(on)

// FUNCTION: 0x4bea20
int __stdcall ClipLine(Surface* dst, int* x0, int* y0, int* x1, int* y1)
{
    Rect r;
    int to_right = (*x0 <= *x1);
    int downwards = (*y0 <= *y1);
    int dx = *x1 - *x0;
    int dy = *y1 - *y0;
    dst->GetClipRect(&r);
    if (*x0 < r.left) {
        if (to_right == 0)
            return 0;
        if (dx == 0)
            goto fail;
        *y0 += (r.left - *x0) * dy / dx;
        *x0 = r.left;
    }
    if (*y0 < r.top) {
        if (downwards == 0)
            return 0;
        if (dy == 0)
            goto fail;
        *x0 += (r.top - *y0) * dx / dy;
        *y0 = r.top;
    }
    if (*x0 > r.right) {
        if (to_right != 0)
            return 0;
        if (dx == 0)
            goto fail;
        *y0 += (r.right - *x0) * dy / dx;
        *x0 = r.right;
    }
    if (*y0 > r.bottom) {
        if (downwards != 0)
            return 0;
        if (dy == 0)
            goto fail;
        *x0 += (r.bottom - *y0) * dx / dy;
        *y0 = r.bottom;
    }
    if (*x1 < r.left) {
        if (to_right != 0)
            return 0;
        if (dx == 0)
            goto fail;
        *y1 += (r.left - *x1) * dy / dx;
        *x1 = r.left;
    }
    if (*y1 < r.top) {
        if (downwards != 0)
            return 0;
        if (dy == 0)
            goto fail;
        *x1 += (r.top - *y1) * dx / dy;
        *y1 = r.top;
    }
    if (*x1 > r.right) {
        if (to_right == 0)
            return 0;
        if (dx == 0)
            goto fail;
        *y1 += (r.right - *x1) * dy / dx;
        *x1 = r.right;
    }
    if (*y1 > r.bottom) {
        if (downwards == 0)
            return 0;
        if (dy != 0) {
            *x1 += (r.bottom - *y1) * dx / dy;
            *y1 = r.bottom;
            return 1;
        }
        goto fail;
    }
    return 1;
fail:
    return 0;
}

// FUNCTION: 0x4bec70
Surface* __stdcall DrawLitLine(Surface* surface, int x0, int y0,
                                         int x1, int y1, int color)
{
    App_004bec70* app = (App_004bec70*)GetDisplay();
    if (!app->palette)
        return 0;
    Surface* ret;
    if (surface == 0) {
        Surface screen;
        ret = (Surface*)LockScreen(&screen);
        if (ret) {
            if (ClipLine(&screen, &x0, &y0, &x1, &y1))
                BlitLineRemapped(&screen, x0, y0, x1, y1, color, app->palette);
            UnlockScreen(&screen);
        }
    } else {
        if (ClipLine(surface, &x0, &y0, &x1, &y1))
            BlitLineRemapped(surface, x0, y0, x1, y1, color, app->palette);
        ret = (Surface*)1;
    }
    return ret;
}

// FUNCTION: 0x4bed70
Surface* __stdcall DrawBlendedLine(Surface* surface, int x0, int y0,
                                         int x1, int y1, int color)
{
    App_004bed70* app = (App_004bed70*)GetDisplay();
    Surface* ret;
    if (surface == 0) {
        Surface screen;
        ret = (Surface*)LockScreen(&screen);
        if (ret) {
            if (ClipLine(&screen, &x0, &y0, &x1, &y1))
                BlitLineRemapped(&screen, x0, y0, x1, y1, color, app->palette);
            UnlockScreen(&screen);
        }
    } else {
        if (ClipLine(surface, &x0, &y0, &x1, &y1))
            BlitLineRemapped(surface, x0, y0, x1, y1, color, app->palette);
        ret = (Surface*)1;
    }
    return ret;
}

// FUNCTION: 0x4bee60
int __stdcall DrawPixel(Surface* surface, int x, int y, int ch)
{
    int ret;
    if (surface == 0) {
        Surface screen;
        ret = LockScreen(&screen);
        if (ret) {
            int y1 = y, x1 = x, y0 = y, x0 = x;
            if (&screen == 0) {
                Surface other;
                if (LockScreen(&other)) {
                    if (ClipLine(&other, &x0, &y0, &x1, &y1))
                        BlitLine(&other, x0, y0, x1, y1, ch);
                    UnlockScreen(&other);
                }
            } else {
                if (ClipLine(&screen, &x0, &y0, &x1, &y1))
                    BlitLine(&screen, x0, y0, x1, y1, ch);
            }
            UnlockScreen(&screen);
        }
    } else {
        int y1 = y, x1 = x, y0 = y, x0 = x;
        if (ClipLine(surface, &x0, &y0, &x1, &y1))
            BlitLine(surface, x0, y0, x1, y1, ch);
        ret = 1;
    }
    return ret;
}

// FUNCTION: 0x4befe0
unsigned int __stdcall ReadPixel(Surface* surface, int x, int y)
{
    unsigned int color;
    if (surface == 0) {
        Surface screen;
        if (LockScreen(&screen)) {
            color = (unsigned char)screen.pixels[screen.pitch * y + x];
            UnlockScreen(&screen);
        }
    } else {
        color = (unsigned char)surface->pixels[surface->pitch * y + x];
    }
    return color;
}

// FUNCTION: 0x4bf060
int __stdcall DrawPolyline(Surface* surface, Point* points,
                           int count, int color)
{
    int result;
    if (surface == 0) {
        Surface screen;
        result = LockScreen(&screen);
        if (result != 0) {
            Segment_004bf060 seg;
            seg.from = points;
            seg.to = points + 1;
            int n = count - 1;
            while (n > 0) {
                int x0, y0, x1, y1;
                y1 = seg.to->y;
                x1 = seg.to->x;
                y0 = seg.from->y;
                x0 = seg.from->x;
                if (&screen == 0) {
                    Surface inner;
                    if (LockScreen(&inner)) {
                        if (ClipLine(&inner, &x0, &y0, &x1, &y1))
                            BlitLine(&inner, x0, y0, x1, y1, color);
                        UnlockScreen(&inner);
                    }
                } else {
                    if (ClipLine(&screen, &x0, &y0, &x1, &y1))
                        BlitLine(&screen, x0, y0, x1, y1, color);
                }
                seg.from++;
                seg.to++;
                n--;
            }
            UnlockScreen(&screen);
        }
    } else {
        Segment_004bf060 seg;
        seg.from = points;
        seg.to = points + 1;
        int n = count - 1;
        while (n > 0) {
            int x0, y0, x1, y1;
            y1 = seg.to->y;
            x1 = seg.to->x;
            y0 = seg.from->y;
            x0 = seg.from->x;
            if (ClipLine(surface, &x0, &y0, &x1, &y1))
                BlitLine(surface, x0, y0, x1, y1, color);
            seg.from++;
            seg.to++;
            n--;
        }
        result = 1;
    }
    return result;
}

// FUNCTION: 0x4bf260
int __stdcall DrawPolylines(Surface* surface, Point* points, int* counts,
                           int rows, int color)
{
    int result;
    if (surface == 0) {
        Surface screen;
        result = LockScreen(&screen);
        if (result != 0) {
            int r = rows;
            int* c = counts;
            Point* p = points;
            while (r > 0) {
                int j = 0;
                while (j < *c - 1) {
                    int x0, y0, x1, y1;
                    y1 = p[1].y;
                    x1 = p[1].x;
                    y0 = p[0].y;
                    x0 = p[0].x;
                    if (&screen == 0) {
                        Surface inner;
                        if (LockScreen(&inner)) {
                            if (ClipLine(&inner, &x0, &y0, &x1, &y1))
                                BlitLine(&inner, x0, y0, x1, y1, color);
                            UnlockScreen(&inner);
                        }
                    } else {
                        if (ClipLine(&screen, &x0, &y0, &x1, &y1))
                            BlitLine(&screen, x0, y0, x1, y1, color);
                    }
                    j++;
                    p++;
                }
                p++;
                c++;
                r--;
            }
            UnlockScreen(&screen);
        }
    } else {
        int r = rows;
        int* c = counts;
        Point* p = points;
        while (r > 0) {
            int j = 0;
            while (j < *c - 1) {
                int x0, y0, x1, y1;
                y1 = p[1].y;
                x1 = p[1].x;
                y0 = p[0].y;
                x0 = p[0].x;
                if (ClipLine(surface, &x0, &y0, &x1, &y1))
                    BlitLine(surface, x0, y0, x1, y1, color);
                j++;
                p++;
            }
            p++;
            c++;
            r--;
        }
        result = 1;
    }
    return result;
}

// FUNCTION: 0x4bf4d0
int __stdcall FadeRectangle(Surface* surface, Rect* rect, int level)
{
    Engine_004bf4d0* engine = (Engine_004bf4d0*)GetDisplay();
    Surface screen;
    Rect r;
    if (surface == 0) {
        if (!LockScreen(&screen))
            return 0;
    } else {
        memcpy(&screen, surface, sizeof(screen));
    }
    if (rect == 0) {
        r.top = 0;
        r.left = 0;
        r.right = engine->width;
        r.bottom = engine->height;
        rect = &r;
    }
    int clipped = ClipRectangle(&screen, rect) != 0;
    if (clipped) {
        int height = rect->bottom - rect->top + 1;
        char* p = (char*)screen.pixels + (screen.width * rect->top + rect->left);
        unsigned char* table;
        unsigned char* t;
        if (level < 0) {
            if (level < -0x20)
                level = -0x20;
            table = engine->fade_neg;
            level += 0x20;
            if (table == 0)
                return 0;
            t = table + (level << 8);
        } else {
            if (level > 0x1f)
                level = 0x1f;
            table = engine->fade_pos;
            if (table == 0)
                return 0;
            t = table + (level << 8);
        }
        if (t == 0)
            return 0;
        while (height--) {
            int w = rect->right - rect->left + 1;
            char* next = p + screen.width;
            while (w--) {
                *p = t[*p];
                p++;
            }
            p = next;
        }
    }
    if (surface == 0)
        UnlockScreen(&screen);
    return 1;
}


// FUNCTION: 0x4bf620
int __stdcall ClipRectangle(Surface* surface, Rect* r)
{
    Rect local;
    surface->GetClipRect(&local);
    if (r->right < local.left)
        return 0;
    if (r->left > local.right)
        return 0;
    if (r->bottom < local.top)
        return 0;
    if (r->top > local.bottom)
        return 0;
    if (r->left < local.left)
        r->left = local.left;
    if (r->top < local.top)
        r->top = local.top;
    if (r->right > local.right)
        r->right = local.right;
    if (r->bottom > local.bottom)
        r->bottom = local.bottom;
    if (r->left > r->right)
        return 0;
    return r->top <= r->bottom;
}

// FUNCTION: 0x4bf6f0
int __stdcall FillRectangle(Surface* surface, Rect* rect, int color)
{
    Rect r = *rect;
    int result;
    if (surface == 0) {
        Surface screen;
        result = LockScreen(&screen);
        if (result != 0) {
            result = ClipRectangle(&screen, &r) != 0;
            if (result)
                FillSolidRect(&screen, &r, color);
            UnlockScreen(&screen);
        }
    } else {
        result = ClipRectangle(surface, &r) != 0;
        if (result)
            FillSolidRect(surface, &r, color);
    }
    return result;
}

// FUNCTION: 0x4bf7b0
Surface* __stdcall DrawLitRectangle(Surface* surface, int* rect,
                                         int color)
{
    Surface* ret = 0;
    if (surface == 0) {
        Surface screen;
        ret = (Surface*)LockScreen(&screen);
        if (ret) {
            DrawLitLine(&screen, rect[0], rect[1], rect[2], rect[1], color);
            DrawLitLine(&screen, rect[2], rect[1], rect[2], rect[3], color);
            DrawLitLine(&screen, rect[0], rect[3], rect[2], rect[3], color);
            DrawLitLine(&screen, rect[0], rect[1], rect[0], rect[3], color);
            UnlockScreen(&screen);
        }
    } else {
        DrawLitLine(surface, rect[0], rect[1], rect[2], rect[1], color);
        DrawLitLine(surface, rect[2], rect[1], rect[2], rect[3], color);
        DrawLitLine(surface, rect[0], rect[3], rect[2], rect[3], color);
        DrawLitLine(surface, rect[0], rect[1], rect[0], rect[3], color);
    }
    return ret;
}

// FUNCTION: 0x4bf8c0
int __stdcall DrawRectangle(Surface* surface, Rect* r, int color)
{
#ifdef __clang__
    int result = 0;
#else
    int result;
#endif
    if (surface == 0) {
        Surface screen;
        result = LockScreen(&screen);
        if (result != 0) {
            {
                int x0, y0, x1, y1; y1 = r->top; x1 = r->right; y0 = r->top; x0 = r->left;
                if (&screen == 0) {
                    Surface inner;
                    if (LockScreen(&inner)) {
                        if (ClipLine(&inner, &x0, &y0, &x1, &y1))
                            BlitLine(&inner, x0, y0, x1, y1, color);
                        UnlockScreen(&inner);
                    }
                } else {
                    if (ClipLine(&screen, &x0, &y0, &x1, &y1))
                        BlitLine(&screen, x0, y0, x1, y1, color);
                }
            }
            {
                int x0, y0, x1, y1; y1 = r->bottom; x1 = r->right; y0 = r->top; x0 = r->right;
                if (&screen == 0) {
                    Surface inner;
                    if (LockScreen(&inner)) {
                        if (ClipLine(&inner, &x0, &y0, &x1, &y1))
                            BlitLine(&inner, x0, y0, x1, y1, color);
                        UnlockScreen(&inner);
                    }
                } else {
                    if (ClipLine(&screen, &x0, &y0, &x1, &y1))
                        BlitLine(&screen, x0, y0, x1, y1, color);
                }
            }
            {
                int x0, y0, x1, y1; y1 = r->bottom; x1 = r->right; y0 = r->bottom; x0 = r->left;
                if (&screen == 0) {
                    Surface inner;
                    if (LockScreen(&inner)) {
                        if (ClipLine(&inner, &x0, &y0, &x1, &y1))
                            BlitLine(&inner, x0, y0, x1, y1, color);
                        UnlockScreen(&inner);
                    }
                } else {
                    if (ClipLine(&screen, &x0, &y0, &x1, &y1))
                        BlitLine(&screen, x0, y0, x1, y1, color);
                }
            }
            {
                int x0, y0, x1, y1; y1 = r->bottom; x1 = r->left; y0 = r->top; x0 = r->left;
                if (&screen == 0) {
                    Surface inner;
                    if (LockScreen(&inner)) {
                        if (ClipLine(&inner, &x0, &y0, &x1, &y1))
                            BlitLine(&inner, x0, y0, x1, y1, color);
                        UnlockScreen(&inner);
                    }
                } else {
                    if (ClipLine(&screen, &x0, &y0, &x1, &y1))
                        BlitLine(&screen, x0, y0, x1, y1, color);
                }
            }
            UnlockScreen(&screen);
        }
    } else {
        {
            int x0, y0, x1, y1; y1 = r->top; x1 = r->right; y0 = r->top; x0 = r->left;
            if (ClipLine(surface, &x0, &y0, &x1, &y1))
                BlitLine(surface, x0, y0, x1, y1, color);
        }
        {
            int x0, y0, x1, y1; y1 = r->bottom; x1 = r->right; y0 = r->top; x0 = r->right;
            if (ClipLine(surface, &x0, &y0, &x1, &y1))
                BlitLine(surface, x0, y0, x1, y1, color);
        }
        DrawLine(surface, r->left, r->bottom, r->right, r->bottom, color);
        DrawLine(surface, r->left, r->top, r->left, r->bottom, color);
    }
    return result;
}

// FUNCTION: 0x4bfd60
int __stdcall XorRectangle(Surface* surface, Rect* rect, int value)
{
    Rect r = *rect;
    int result;
    if (surface == 0) {
        Surface screen;
        result = LockScreen(&screen);
        if (result != 0) {
            if (ClipRectangle(&screen, &r))
                XorRect(&screen, &r, value);
            UnlockScreen(&screen);
        }
    } else {
        if (ClipRectangle(surface, &r))
            XorRect(surface, &r, value);
        result = ((int*)&surface)[2];
    }
    return result;
}

// FUNCTION: 0x4bfe10
int __stdcall GrayRectangle(Surface* surface, Rect* rect)
{
    Surface screen;
    Rect r = *rect;
    Game_004bfe10* g = (Game_004bfe10*)GetDisplay();
    if ((g->flags & 1) == 0)
        return 0;
    int result;
#ifdef __clang__
    int status = 0;
#else
    int status;
#endif
    if (surface == 0) {
        result = LockScreen(&screen);
        if (result != 0) {
            if (ClipRectangle(&screen, &r))
                RemapRect(r.top * screen.pitch + r.left + (int)screen.pixels,
                             screen.pitch, r.right - r.left + 1,
                             r.bottom - r.top + 1, g->grayTable);
            UnlockScreen(&screen);
        }
    } else {
        if (ClipRectangle(surface, &r))
            RemapRect((int)surface->pixels + r.top * surface->pitch + r.left,
                         surface->pitch, r.right - r.left + 1,
                         r.bottom - r.top + 1, g->grayTable);
        result = status;
    }
    return result;
}

// FUNCTION: 0x4bff20
int __stdcall DitherRectangle(Surface* surface, Rect* rect, int phase)
{
    Surface screen;
    if (surface == 0) {
        if (LockScreen(&screen))
            surface = &screen;
        else
            return 0;
    }
    Rect r = *rect;
    if (ClipRectangle(surface, &r)) {
        int x1 = (r.left + 3) & ~3;
        int x2 = (r.right + 3) & ~3;
        for (int i = r.top; i <= r.bottom; i++) {
            unsigned char* base = (unsigned char*)surface->pixels + i * surface->pitch + x1;
            unsigned char* end = base + x2 - x1;
            unsigned char* p = base + ((r.left + i + phase) & 1) + r.left - x1;
            while (p < base) {
                *p = 0;
                p += 2;
            }
            unsigned char* pe;
            unsigned char* pb;
            if ((i + phase) & 1) {
                unsigned int* q = (unsigned int*)base;
                unsigned int* qe = (unsigned int*)end;
                while (q < qe) {
                    *q &= 0xff00ff;
                    q++;
                }
                pe = (unsigned char*)q + r.right - x2;
                pb = (unsigned char*)q + 1;
            } else {
                unsigned int* q = (unsigned int*)base;
                unsigned int* qe = (unsigned int*)end;
                while (q < qe) {
                    *q &= 0xff00ff00;
                    q++;
                }
                pe = (unsigned char*)q + r.right - x2;
                pb = (unsigned char*)q;
            }
            while (pb <= pe) {
                *pb = 0;
                pb += 2;
            }
        }
    }
    if (surface == &screen)
        UnlockScreen(&screen);
    return 1;
}


static inline void Draw_004c0070(Surface* surface, int x0, int y0,
                                 int x1, int y1, int color)
{
    if (surface == 0) {
        Surface screen;
        if (LockScreen(&screen)) {
            if (ClipLine(&screen, &x0, &y0, &x1, &y1))
                BlitLine(&screen, x0, y0, x1, y1, color);
            UnlockScreen(&screen);
        }
    } else {
        if (ClipLine(surface, &x0, &y0, &x1, &y1))
            BlitLine(surface, x0, y0, x1, y1, color);
    }
}

// FUNCTION: 0x4c0070
void __stdcall DrawCircle(Surface* surface, int xc, int yc, int radius,
                            int color)
{
    int x0 = xc + radius;
    int y0 = yc;
    for (int angle = 0x800; angle <= 0x10000; angle += 0x800) {
        int x1 = ScaledCos(angle, radius) + xc;
        int y1 = ScaledSin(angle, radius) + yc;
        Draw_004c0070(surface, x0, y0, x1, y1, color);
        x0 = x1;
        y0 = y1;
    }
}

// FUNCTION: 0x4c01a0
void __stdcall DrawDashedCircle(Surface* surface, int cx, int cy, int radius,
                            int color, int n, int start)
{
    Surface screen;
    int px = cx + radius;
    int py = cy;
    int angle = 0x10000 / n;
    int step = 0x10000 / n;
    int i;
    if (angle > 0x10000)
        return;
    i = start;
    while (angle <= 0x10000) {
        int x = ScaledCos(angle, radius) + cx;
        int y = ScaledSin(angle, radius) + cy;
        if (i & 1) {
            int y1 = y;
            int x1 = x;
            int y0 = py;
            int x0 = px;
            if (surface == 0) {
                if (LockScreen(&screen)) {
                    if (ClipLine(&screen, &x0, &y0, &x1, &y1))
                        BlitLine(&screen, x0, y0, x1, y1, color);
                    UnlockScreen(&screen);
                }
            } else {
                if (ClipLine(surface, &x0, &y0, &x1, &y1))
                    BlitLine(surface, x0, y0, x1, y1, color);
            }
        }
        px = x;
        py = y;
        i++;
        angle += step;
    }
}


// FUNCTION: 0x4c0310
void __stdcall FillPolygon(Surface* surface, Point* points, int n, unsigned char color)
{
    ScanFillPolygon(surface, points, n, color);
}

// FUNCTION: 0x4c07b0
int __stdcall DrawPolygon(Surface* surface, Vec3* pts, int count, int color)
{
    for (int i = 0; i < count - 1; i++)
        DrawLine(surface, pts[i].x, pts[i].y, pts[i + 1].x, pts[i + 1].y, color);
    DrawLine(surface, pts[count - 1].x, pts[count - 1].y, pts[0].x, pts[0].y, color);
    return 1;
}


// FUNCTION: 0x4c0820
int __stdcall DrawPolygonEdges(GafFrame* surf, Vec3* pts, int count,
                           int color) {
    Span spans[2048];
    Span* out;
    Vec3* b;
    int x, dx;
    int ymin = 999999;
    int xmax = -999999;
    int ymax = -999999;
    int xmin = 999999;
    int iymin, iymax;
    int y;
    Span* s;
    int i;
    Vec3* p;
    i = 0;
    if (count > 0) {
        p = pts;
        do {
            int py = p->y;
            if (py < ymin) {
                ymin = py;
                iymin = i;
            }
            if (p->y > ymax) {
                ymax = p->y;
                iymax = i;
            }
            int px = p->x;
            if (px > xmax)
                xmax = px;
            if (p->x < xmin)
                xmin = p->x;
            i++;
            p++;
        } while (i < count);
    }
    if (ymax == ymin)
        return 0;
    {
        out = spans;
        int i = iymin;
        do {
            int j = i - 1;
            if (j < 0) {
                j = count;
                j--;
            }
            int y0 = pts[i].y;
            b = &pts[j];
            int y1 = b->y;
            if (y0 < y1) {
                int h = y1 - y0;
                x = pts[i].x;
                dx = ((b->x - x) << 16) / h;
                x = (x << 16) + 0xffff;
                int y = pts[i].z << 16;
                int dy = ((b->z << 16) - y) / h;
                int rows = b->y - y0;
                do {
                    out->x1 = x >> 16;
                    out->z1 = y;
                    x += dx;
                    y += dy;
                    out++;
                } while (--rows);
            }
            i = i - 1;
            if (i < 0) {
                i = count;
                i--;
            }
        } while (i != iymax);
    }
    {
        int i = iymin;
        out = spans;
        do {
            int j = i + 1;
            if (j >= count)
                j = 0;
            int y0 = pts[i].y;
            b = &pts[j];
            int y1 = b->y;
            if (y0 < y1) {
                int h = y1 - y0;
                x = pts[i].x;
                dx = ((b->x - x) << 16) / h;
                x = (x << 16) + 0xffff;
                int y = pts[i].z << 16;
                int dy = ((b->z << 16) - y) / h;
                int rows = b->y - y0;
                do {
                    out->x2 = x >> 16;
                    out->z2 = y;
                    x += dx;
                    y += dy;
                    out++;
                } while (--rows);
            }
            i = i + 1;
            if (i >= count)
                i = 0;
        } while (i != iymax);
    }
    {
        s = spans;
        y = ymin;
        while (y < ymax) {
            if (s->x2 - s->x1 > 0)
                PlotSpanEnds(y, s, surf, color);
            s++;
            y++;
        }
    }
    return 1;
}

// FUNCTION: 0x4c0b10
void __stdcall FillShadedSpan(int row, Span* span, GafFrame* surf, unsigned char color)
{
    unsigned char* p = surf->pixelsOrLayers;
    unsigned char* d = surf->scratch;
    App_004c0b10* app = (App_004c0b10*)GetDisplay();
    int w = span->x2 - span->x1;
    int dz = (span->z2 - span->z1) / w;
    int ds = (span->s2 - span->s1) / w;
    if (span->x1 < 0) {
        span->z1 = span->z1 - dz * span->x1;
        span->s1 = span->s1 - ds * span->x1;
        span->x1 = 0;
    }
    if (span->x2 > (int)surf->width - 1)
        span->x2 = surf->width - 1;
    int start = span->x1;
    int count = span->x2 - start;
    if (count > 0) {
        int z = span->z1;
        int s = span->s1;
        p += row * surf->width;
        p += start;
        if (d != 0) {
            d += row * surf->width + span->x1;
            while (count--) {
                unsigned char zi = z >> 16;
                if (*d <= zi) {
                    int si = s >> 16;
                    *p = app->shade[(si << 8) + color];
                    *d = zi;
                }
                p++;
                d++;
                z += dz;
                s += ds;
            }
        } else {
            while (count--) {
                int si = s >> 16;
                *p++ = app->shade[(si << 8) + color];
                s += ds;
            }
        }
    }
}


// FUNCTION: 0x4c0c70
int __stdcall FillShadedPolygon(GafFrame* surf, Point_004c0c70* pts, int n,
                           unsigned char color) {
    int y0, y1, x, dx, dz;
    Span spans[2048];
    int maxX = -999999;
    int minY = 999999;
    int maxY = -999999;
    int minX = 999999;
    int minYi;
    int maxYi;
    int i;
    GafFrame* sf = surf;
    int scanIndex = 0;
    if (n > 0) {
        Point_004c0c70* p = pts;
        do {
            if (p->y < minY) {
                minY = p->y;
                minYi = scanIndex;
            }
            if (p->y > maxY) {
                maxY = p->y;
                maxYi = scanIndex;
            }
            if (p->x > maxX)
                maxX = p->x;
            if (p->x < minX)
                minX = p->x;
            scanIndex++;
            p++;
        } while (scanIndex < n);
    }
    if (minX > (int)sf->width - 1)
        return 0;
    if (maxY < 0)
        return 0;
    if (minY > (int)sf->height - 1)
        return 0;
    int maxRow = (int)sf->height - 1;
    if (minY < 0)
        minY = 0;
    if (maxY > maxRow)
        maxY = maxRow;
    if (maxY == minY)
        return 0;
    {
        Span* sp = spans;
        i = minYi;
        for (;;) {
            int j = i - 1;
            if (j < 0)
                j = n - 1;
            Point_004c0c70* p = &pts[i];
            Point_004c0c70* q = &pts[j];
            y0 = p->y;
            y1 = q->y;
            if (y0 < y1) {
                int dy = y1 - y0;
                x = p->x;
                dx = ((q->x - x) << 16) / dy;
                x = (x << 16) + 0xffff;
                int z = p->z << 16;
                int s = p->s << 16;
                dz = ((q->z << 16) - z) / dy;
                int ds = ((q->s << 16) - s) / dy;
                if (y0 < 0) {
                    x -= dx * y0;
                    z -= dz * y0;
                    s -= ds * y0;
                    y0 = 0;
                }
                if (y1 > maxRow)
                    y1 = maxRow;
                if (y0 < y1) {
                    int count = y1 - y0;
                    do {
                        sp->x1 = x >> 16;
                        x += dx;
                        sp->z1 = z;
                        sp->s1 = s;
                        z += dz;
                        s += ds;
                        sp++;
                    } while (--count);
                }
            }
            i = i - 1;
            if (i < 0)
                i = n - 1;
            if (i == maxYi)
                break;
        }
        sp = spans;
        i = minYi;
        for (;;) {
            int j = i + 1;
            if (j >= n)
                j = 0;
            Point_004c0c70* p = &pts[i];
            Point_004c0c70* q = &pts[j];
            y0 = p->y;
            y1 = q->y;
            if (y0 < y1) {
                int dy = y1 - y0;
                x = p->x;
                dx = ((q->x - x) << 16) / dy;
                x = (x << 16) + 0xffff;
                int z = p->z << 16;
                int s = p->s << 16;
                dz = ((q->z << 16) - z) / dy;
                int ds = ((q->s << 16) - s) / dy;
                if (y0 < 0) {
                    x -= dx * y0;
                    z -= dz * y0;
                    s -= ds * y0;
                    y0 = 0;
                }
                if (y1 > maxRow)
                    y1 = maxRow;
                if (y0 < y1) {
                    int count = y1 - y0;
                    do {
                        sp->x2 = x >> 16;
                        x += dx;
                        sp->z2 = z;
                        sp->s2 = s;
                        z += dz;
                        s += ds;
                        sp++;
                    } while (--count);
                }
            }
            i = i + 1;
            if (i >= n)
                i = 0;
            if (i == maxYi)
                break;
        }
    }
    {
        Span* sp = spans;
        for (i = minY; i < maxY; i++) {
            if (sp->x2 - sp->x1 > 0)
                FillShadedSpan(i, sp, surf, color);
            sp++;
        }
    }
    return 1;
}

// FUNCTION: 0x4c1000
int __stdcall FillFlatPolygon(GafFrame* surf, Vec3* pts, int n, int color)
{
    int y0, y1, x, dx, dz;
    Span spans[2048];
    int maxX = -999999;
    int minY = 999999;
    int maxY = -999999;
    int minX = 999999;
    int minYi;
    int maxYi;
    int i;
    int j;
    GafFrame* sf = surf;
    int scanIndex = 0;
    if (n > 0) {
        Vec3* p = pts;
        do {
            if (p->y < minY) {
                minY = p->y;
                minYi = scanIndex;
            }
            if (p->y > maxY) {
                maxY = p->y;
                maxYi = scanIndex;
            }
            if (p->x > maxX)
                maxX = p->x;
            if (p->x < minX)
                minX = p->x;
            scanIndex++;
            p++;
        } while (scanIndex < n);
    }
    if (minX > (int)sf->width - 1)
        return 0;
    if (maxY < 0)
        return 0;
    if (minY > (int)sf->height - 1)
        return 0;
    int maxRow = (int)sf->height - 1;
    if (minY < 0)
        minY = 0;
    if (maxY > maxRow)
        maxY = maxRow;
    if (maxY == minY)
        return 0;
    {
        Span* sp = spans;
        {
            int i = minYi;
            for (;;) {
                j = i - 1;
                if (j < 0)
                    j = n - 1;
                y0 = pts[i].y;
                y1 = pts[j].y;
                if (y0 < y1) {
                    int dy = y1 - y0;
                    x = pts[i].x;
                    dx = ((pts[j].x - x) << 16) / dy;
                    x = (x << 16) + 0xffff;
                    int z = pts[i].z << 16;
                    dz = ((pts[j].z << 16) - z) / dy;
                    if (y0 < 0) {
                        x -= dx * y0;
                        z -= dz * y0;
                        y0 = 0;
                    }
                    if (y1 > maxRow)
                        y1 = maxRow;
                    if (y0 < y1) {
                        int count = y1 - y0;
                        do {
                            sp->x1 = x >> 16;
                            sp->z1 = z;
                            x += dx;
                            z += dz;
                            sp++;
                        } while (--count);
                    }
                }
                i = i - 1;
                if (i < 0)
                    i = n - 1;
                if (i == maxYi)
                    break;
            }
        }
        sp = spans;
        {
            int i = minYi;
            for (;;) {
                j = i + 1;
                if (j >= n)
                    j = 0;
                y0 = pts[i].y;
                y1 = pts[j].y;
                if (y0 < y1) {
                    int dy = y1 - y0;
                    x = pts[i].x;
                    dx = ((pts[j].x - x) << 16) / dy;
                    x = (x << 16) + 0xffff;
                    int z = pts[i].z << 16;
                    dz = ((pts[j].z << 16) - z) / dy;
                    if (y0 < 0) {
                        x -= dx * y0;
                        z -= dz * y0;
                        y0 = 0;
                    }
                    if (y1 > maxRow)
                        y1 = maxRow;
                    if (y0 < y1) {
                        int count = y1 - y0;
                        do {
                            sp->x2 = x >> 16;
                            sp->z2 = z;
                            x += dx;
                            z += dz;
                            sp++;
                        } while (--count);
                    }
                }
                i = i + 1;
                if (i >= n)
                    i = 0;
                if (i == maxYi)
                    break;
            }
        }
    }
    {
        Span* sp = spans;
        for (i = minY; i < maxY; i++) {
            if (sp->x2 - sp->x1 > 0)
                FillFlatSpan(i, sp, surf, color);
            sp++;
        }
    }
    return 1;
}

// FUNCTION: 0x4c1320
int __stdcall PointInPolygon(Point* pts, int n, int px, int py)
{
    if (n < 3)
        return 0;
    for (int i = 0; i < n; i++) {
        Point* a = &pts[i];
        Point* b = &pts[(i + 1) % n];
        if ((b->y - a->y) * (px - a->x) <= (b->x - a->x) * (py - a->y))
            return 0;
    }
    return 1;
}


// FUNCTION: 0x4c13a0
void __stdcall SetTextColors(int param_1, int param_2)
{
    void* eax = (void*)GetDisplay();
    if (param_1 != -1) {
        *(int*)((unsigned char*)eax + 0x208) = param_1;
    }
    if (param_2 != -1) {
        *(int*)((unsigned char*)eax + 0x20c) = param_2;
    }
}

// FUNCTION: 0x4c13d0
void __stdcall SetTextKeyColor(int value)
{
    Obj_004c13d0* obj = (Obj_004c13d0*)GetDisplay();
    obj->keyColor = value;
}


// FUNCTION: 0x4c13f0
int GetTextKeyColor()
{
    Obj* obj = (Obj*)GetDisplay();
    return obj->keyColor;
}


// FUNCTION: 0x4c1400
int GetTextForeColor()
{
    Obj* obj = (Obj*)GetDisplay();
    return obj->foreColor;
}


// FUNCTION: 0x4c1410
int GetTextBackColor()
{
    Obj* obj = (Obj*)GetDisplay();
    return obj->backColor;
}


// FUNCTION: 0x4c1420
void __stdcall SetFont(int param_1)
{
    if (param_1 != 0) {
        int eax = GetDisplay();
        *(int*)(eax + 0x204) = param_1;
    }
}


// FUNCTION: 0x4c1440
int GetFont()
{
    return *(int*)(GetDisplay() + 0x204);
}


// FUNCTION: 0x4c1450
int GetFontHeight()
{
    int temp = GetDisplay();
    int ptr = *(int*)(temp + 0x204);
    unsigned int result = 0;
    result = *(unsigned char*)ptr;
    return result;
}


// FUNCTION: 0x4c1470
unsigned char __stdcall FontHeight(void* ptr)
{
    unsigned int result = 0;
    result = *(unsigned char*)ptr;
    return (unsigned char)result;
}

// FUNCTION: 0x4c0a90
void __stdcall PlotSpanEnds(int row, Span* span, GafFrame* surf, unsigned char color)
{
    unsigned char* d = surf->scratch;
    unsigned char* p = surf->pixelsOrLayers;
    int w = span->x2 - span->x1;
    if (w > 0) {
        int start = span->x1;
        int z = span->z1;
        p += row * surf->width + start;
        if (d != 0) {
            d += row * surf->width + span->x1;
            unsigned char z1 = z >> 16;
            if (*d <= z1) {
                *p = color;
                *d = z1;
            }
            d += w;
            p += w;
            unsigned char z2 = span->z2 >> 16;
            if (*d <= z2) {
                *p = color;
                *d = z2;
            }
        } else {
            *p = color;
            p[w] = color;
        }
    }
}

// FUNCTION: 0x4c06e0
void __stdcall FillFlatSpan(int row, Span* span, GafFrame* surf, unsigned char color)
{
    unsigned char* p = surf->pixelsOrLayers;
    unsigned char* d = surf->scratch;
    int slope = (span->z2 - span->z1) / (span->x2 - span->x1);
    if (span->x1 < 0) {
        span->z1 = span->z1 - span->x1 * slope;
        span->x1 = 0;
    }
    if (span->x2 > (int)surf->width - 1)
        span->x2 = surf->width - 1;
    int n = span->x2 - span->x1;
    if (n > 0) {
        int start = span->x1;
        int off = start + row * surf->width;
        int z = span->z1;
        p = p + (start + row * surf->width);
        if (d != 0) {
            d = d + off;
            while (n--) {
                if (*d <= (unsigned char)(z >> 16)) { *p = color; *d = (unsigned char)(z >> 16); }
                p++; d++; z += slope;
            }
        } else {
            while (n--) *p++ = color;
        }
    }
}

// FUNCTION: 0x4c0330
int __stdcall ScanFillPolygon(Surface* surface, Point* points, int n, unsigned char color)
{
    Surface screen;
    Rect clip;
    Span_004c0330 span[0x800];
    int locked;
    int minY;
    int maxY;
    int minX;
    int maxX;
    int minIdx;
    int maxIdx;
    int i;
    int j;
    int dy;

    if (surface == 0) {
        if (LockScreen(&screen) == 0)
            return 0;
        locked = 1;
        surface = &screen;
    } else {
        locked = 0;
    }

    minY = 999999;
    maxY = -999999;
    minX = 999999;
    maxX = -999999;
    for (i = 0; i < n; i++) {
        int y = points[i].y;
        if (y < minY) {
            minY = y;
            minIdx = i;
        }
        if (y > maxY) {
            maxY = y;
            maxIdx = i;
        }
        int x = points[i].x;
        if (x > maxX)
            maxX = x;
        if (x < minX)
            minX = x;
    }
    surface->GetClipRect(&clip);
    if (maxX < clip.left) {
        if (locked)
            UnlockScreen(&screen);
        return 0;
    }
    if (minX > clip.right) {
        if (locked)
            UnlockScreen(&screen);
        return 0;
    }
    if (maxY < clip.top) {
        if (locked)
            UnlockScreen(&screen);
        return 0;
    }
    if (minY > clip.bottom) {
        if (locked)
            UnlockScreen(&screen);
        return 0;
    }
    if (minY < clip.top)
        minY = clip.top;
    if (maxY > clip.bottom)
        maxY = clip.bottom;
    if (maxY == minY) {
        if (locked)
            UnlockScreen(&screen);
        return 0;
    }

    Span_004c0330* sp = span;
    i = minIdx;
    do {
        j = i - 1;
        if (j < 0)
            j = n - 1;
        int y1 = points[j].y;
        int y0 = points[i].y;
        if (y1 > clip.top && y0 < y1) {
            int x0 = points[i].x;
            int x1 = points[j].x;
            dy = y1 - y0;
            int slope = ((x1 - x0) << 16) / dy;
            int fx = (x0 << 16) + 0xffff;
            if (y0 < clip.top) {
                fx += (clip.top - y0) * slope;
                y0 = clip.top;
            }
            if (y1 > clip.bottom)
                y1 = clip.bottom;
            for (int y = y0; y < y1; y++) {
                sp->left = fx >> 16;
                fx += slope;
                sp++;
            }
        }
        i = i - 1;
        if (i < 0)
            i = n - 1;
    } while (i != maxIdx);

    sp = span;
    i = minIdx;
    do {
        j = i + 1;
        if (j >= n)
            j = 0;
        int y1 = points[j].y;
        int y0 = points[i].y;
        if (y1 > clip.top && y0 < y1) {
            int x0 = points[i].x;
            int x1 = points[j].x;
            dy = y1 - y0;
            int slope = ((x1 - x0) << 16) / dy;
            int fx = (x0 << 16) + 0xffff;
            if (y0 < clip.top) {
                fx += (clip.top - y0) * slope;
                y0 = clip.top;
            }
            if (y1 > clip.bottom)
                y1 = clip.bottom;
            for (int y = y0; y < y1; y++) {
                sp->right = fx >> 16;
                fx += slope;
                sp++;
            }
        }
        i = i + 1;
        if (i >= n)
            i = 0;
    } while (i != maxIdx);

    Span_004c0330* s = span;
    for (i = minY; i < maxY; i++) {
        if (s->right > clip.right)
            s->right = clip.right;
        if (s->left < clip.left)
            s->left = clip.left;
        int w = s->right - s->left;
        if (w > 0)
            memset(surface->pixels + surface->pitch * i + s->left, *(int *)&color, w);
        s++;
    }
    if (locked)
        UnlockScreen(&screen);
    return 1;
}
