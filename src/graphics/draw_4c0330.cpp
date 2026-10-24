#include <memory.h>

#include "surface.h"
#include "../util/point.h"

struct Span_004c0330 {
    int left;
    int right;
    int unknown_8[8];
};

int __stdcall LockScreen(Surface* out);
int __stdcall UnlockScreen(Surface* s);

void __cdecl BlitLine(Surface* dst, int x0, int y0, int x1, int y1, int color);

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
