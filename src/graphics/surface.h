#ifndef SURFACE_H
#define SURFACE_H

#include "rect.h"

struct Surface {
    int width;
    int height;
    int pitch;
    unsigned char* pixels;
    int zPriority;
    int colorKey;
    short x;
    short y;
    Rect clip;
    unsigned int flag0 : 1;
    unsigned int flag1 : 1;

    Surface() {}
    Rect* GetClipRect(Rect* out);
    void SetClipRect(Rect r);
};

#endif
