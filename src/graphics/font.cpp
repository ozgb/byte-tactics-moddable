#include <string.h>

#include "surface.h"

#include "font.h"

#include "display.h"

#include "../util/registry.h"
int __stdcall LockScreen(Surface* out);
int __stdcall UnlockScreen(Surface* s);
void __cdecl BlitText(unsigned char* pixels, int pitch, Font* font, unsigned char* text,
                          int x, int y, int c1, int c2, int c3);
void __stdcall DrawString(Surface* dst, unsigned char* text, int x, int y, int maxWidth);

// FUNCTION: 0x4c1480
int __stdcall GetTextWidth(Font* font, unsigned char* text)
{
    int width = 0;
    if (text && font) {
        for (; *text && *text != '\n'; text++) {
            unsigned char c = *text;
            if (c >= font->first) {
                int d = c - font->first;
                unsigned int off = 0;
                off = font->offsets[(unsigned short)d];
                if (off)
                    width += ((unsigned char*)font)[off];
            }
        }
    }
    return width;
}

// FUNCTION: 0x4c14f0
void __stdcall DrawString(Surface* dst, unsigned char* text, int x, int y, int maxWidth)
{
    Display* display = GetDisplay();
    int width = GetTextWidth(display->font, text);
    if (maxWidth != -1 && width > maxWidth) {
        unsigned char buf[0x12c];
        strncpy((char*)buf, (char*)text, 0x12b);
        int len = strlen((char*)buf);
        text = buf;
        unsigned char* end = buf + len - 1;
        while (1) {
            *end = 0;
            if (end == buf)
                break;
            end--;
            width = GetTextWidth(display->font, text);
            if (width <= maxWidth)
                break;
        }
    }
    Rect r;
    r.left = x;
    r.top = y;
    r.right = x + width;
    r.bottom = r.top + GetDisplay()->font->glyphs[0];
    if (dst == 0) {
        Surface screen;
        if (LockScreen(&screen) != 0) {
            Rect clip;
            screen.GetClipRect(&clip);
            if (RectInsideRect(&r, &clip))
                BlitText(screen.pixels, screen.pitch, display->font, text, x, y, display->foreColor,
                             display->backColor, display->keyColor);
            UnlockScreen(&screen);
        }
    } else {
        Rect clip;
        dst->GetClipRect(&clip);
        if (RectInsideRect(&r, &clip))
            BlitText(dst->pixels, dst->pitch, display->font, text, x, y, display->foreColor,
                         display->backColor, display->keyColor);
    }
}

// FUNCTION: 0x4c1760
void __stdcall DrawStringCentered(Surface* dst, unsigned char* text, int flag)
{
    int width = GetTextWidth(GetDisplay()->font, text);
    if (dst == 0) {
        Surface r;
        if (LockScreen(&r) != 0) {
            DrawString(&r, text, (r.width - width) >> 1, flag, -1);
            UnlockScreen(&r);
        }
    } else {
        DrawString(dst, text, (dst->width - width) >> 1, flag, -1);
    }
}

static inline int CurrentColour(void)
{
    return GetDisplay()->keyColor;
}

static inline void SetColour(int a, int b)
{
    Display* obj = GetDisplay();
    if (a != -1) {
        obj->foreColor = a;
    }
    if (b != -1) {
        obj->backColor = b;
    }
}

// FUNCTION: 0x4c1830
void __stdcall DrawOutlinedString(Surface* dst, unsigned char* text, int fore,
                            int back, int y)
{
    Display* display = GetDisplay();
    int width = GetTextWidth(display->font, text);
    if (dst == 0) {
        Surface r;
        if (LockScreen(&r) != 0) {
            int x = (r.width - width) >> 1;
            SetColour(fore, CurrentColour());
            DrawString(&r, text, x - 1, y, -1);
            DrawString(&r, text, x + 1, y, -1);
            DrawString(&r, text, x, y - 1, -1);
            DrawString(&r, text, x, y + 1, -1);
            SetColour(back, CurrentColour());
            DrawString(&r, text, x, y, -1);
            UnlockScreen(&r);
            return;
        }
    } else {
        int x = (dst->width - width) >> 1;
        SetColour(fore, CurrentColour());
        DrawString(dst, text, x - 1, y, -1);
        DrawString(dst, text, x + 1, y, -1);
        DrawString(dst, text, x, y - 1, -1);
        DrawString(dst, text, x, y + 1, -1);
        SetColour(back, CurrentColour());
        DrawString(dst, text, x, y, -1);
    }
}
