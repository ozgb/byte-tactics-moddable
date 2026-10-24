#include "../graphics/rect.h"
#include "../graphics/gaf_frame.h"

struct Bits_004b0230 {
    unsigned short count;
    char unknown_2[0x28 - 0x2];
    GafFrame* child[1];
};

struct Gaf_004b0230 {
    char unknown_0[0x14];
};

struct Cell_004b0230 {
    int step_x;
    int step_y;
};

struct Surface_004b0230 {
    int tiles_x;
    int tiles_y;
    char unknown_8[0xbc - 0x8];
};

struct Holder_004b0230 {
    char unknown_0[4];
    char* entries;
};

struct Object_004b0230 {
    char unknown_0[4];
    Gaf_004b0230* gaf;
    char unknown_8[0x18 - 0x8];
    Holder_004b0230* holder;
    char unknown_1c[0x8b2 - 0x1c];
    unsigned char dark;
    char unknown_8b3[0x8c3 - 0x8b3];
    unsigned char light;
    char unknown_8c4[2];
    unsigned char fill;
};

int __stdcall DrawLine(void* surface, int x0, int y0, int x1, int y1, unsigned char color);
void __stdcall FillRectangle(void* surface, void* rect, int color);
#include "gui.h"
Bits_004b0230* __stdcall FindGafEntry(Gaf_004b0230* gaf, const char* name);
void __stdcall DrawSurface(Surface_004b0230* dst, Cell_004b0230* cell, int x, int y);
GafFrame* __stdcall GetGafFrame(Bits_004b0230* bits, int index);
void __stdcall DrawFrame(Surface_004b0230* dst, GafFrame* pic, int x, int y);

// FUNCTION: 0x4b0090
void __stdcall DrawBevelBorder(void* surface, Rect* rect, int light,
                            unsigned char dark, int unused)
{
    DrawLine(surface, rect->left, rect->top, rect->right, rect->top, light);
    DrawLine(surface, rect->left, rect->top + 1, rect->right - 1, rect->top + 1, light);
    DrawLine(surface, rect->left, rect->top, rect->left, rect->bottom, light);
    DrawLine(surface, rect->left + 1, rect->top, rect->left + 1, rect->bottom - 1, light);
    DrawLine(surface, rect->right, rect->top + 1, rect->right, rect->bottom, dark);
    DrawLine(surface, rect->right - 1, rect->top + 2, rect->right - 1, rect->bottom, dark);
    DrawLine(surface, rect->left + 1, rect->bottom, rect->right, rect->bottom, dark);
    DrawLine(surface, rect->left + 2, rect->bottom - 1, rect->right, rect->bottom - 1, dark);
}

// FUNCTION: 0x4b0160
void __stdcall DrawBevelBorderDarkFirst(void* surface, Rect* rect, int dark, int light, int unused)
{
    DrawLine(surface, rect->left, rect->top, rect->right, rect->top, light);
    DrawLine(surface, rect->left, rect->top + 1, rect->right - 1, rect->top + 1, light);
    DrawLine(surface, rect->left, rect->top, rect->left, rect->bottom, light);
    DrawLine(surface, rect->left + 1, rect->top, rect->left + 1, rect->bottom - 1, light);
    DrawLine(surface, rect->right, rect->top + 1, rect->right, rect->bottom, dark);
    DrawLine(surface, rect->right - 1, rect->top + 2, rect->right - 1, rect->bottom, dark);
    DrawLine(surface, rect->left + 1, rect->bottom, rect->right, rect->bottom, dark);
    DrawLine(surface, rect->left + 2, rect->bottom - 1, rect->right, rect->bottom - 1, dark);
}

// FUNCTION: 0x4b0230
void __stdcall DrawListboxFrame(Object_004b0230* obj, int index, Bits_004b0230* bmp)
{
    char* entries;
    int col;
    Surface_004b0230* surface;
    int height;
    int x;
    int width;
    int x0;
    Rect rect;
    GafFrame* tile;
    int w;
    int row;
    int y;
    int yoff;
    GafFrame* p;
    int y0;
    Cell_004b0230* cell;
    int h;
    GafFrame* sub;
    entries = obj->holder->entries;
    GetGadgetRectByIndex(entries, index, &rect);

    if (bmp == 0) {
        if (obj->gaf == 0)
            return;
        bmp = FindGafEntry(obj->gaf, "Listbox");
        if (bmp == 0)
            return;
        rect.left -= 3;
        rect.top -= 3;
        rect.right += 3;
        rect.bottom += 3;
    }

    surface = *(Surface_004b0230**)(obj->holder->entries + 0xbc);
    cell = (Cell_004b0230*)FindBackgroundCell((Gadget *)entries, index);

    if (cell != 0) {
        for (x = 0; x < surface->tiles_x; x += cell->step_x) {
            for (y = 0; y < surface->tiles_y; y += cell->step_y) {
                DrawSurface(surface, cell, x, y);
            }
        }
        DrawBevelBorderDarkFirst(surface, &rect, obj->dark, obj->light, obj->fill);
    } else if (bmp != 0) {
        if (bmp->count > 1) {
            sub = GetGafFrame(bmp, 0);
            w = sub->width;
            h = sub->height;
            if (index != 0) {
                y0 = rect.top;
                x0 = rect.left;
            } else {
                y0 = 0;
                x0 = 0;
            }
            height = rect.bottom - rect.top + 1;
            width = rect.right - rect.left + 1;
            yoff = 0;
            while (yoff < height) {
                if (yoff == 0)
                    row = 0;
                else
                    row = (yoff < height - h + 1) ? 3 : 6;
                if (yoff + h > height)
                    yoff = height - h;
                for (x = 0; x < width; x += w) {
                    if (x + w >= width) {
                        x = width - w;
                        col = 2;
                    } else {
                        col = (x != 0) ? 1 : 0;
                    }
                    tile = GetGafFrame(bmp, row + col);
                    DrawFrame(surface, tile, x0 + x, y0 + yoff);
                }
                yoff += h;
            }
        } else {
            p = GetGafFrame(bmp, 0);
            DrawFrame(surface, p, 0, 0);
        }
    } else {
        FillRectangle(surface, &rect, obj->fill);
        DrawBevelBorderDarkFirst(surface, &rect, obj->dark, obj->light, obj->fill);
    }
}

// FUNCTION: 0x4b04b0
void __stdcall FillBevelBox(void* surface, Rect* rect, int light, int dark, int fill)
{
    FillRectangle(surface, rect, fill);
    DrawBevelBorder(surface, rect, light, dark, fill);
}

// FUNCTION: 0x4b04e0
void __stdcall FillBevelBoxDarkFirst(void* surface, Rect* rect, int light, int dark, int fill)
{
    FillRectangle(surface, rect, fill);
    DrawBevelBorderDarkFirst(surface, rect, light, dark, fill);
}

// FUNCTION: 0x4b0510
void __stdcall DrawRaisedBox(void* surface, Rect* rect, int light, unsigned char dark, int fill)
{
    FillRectangle(surface, rect, fill);
    DrawLine(surface, rect->left, rect->top, rect->right, rect->top, light);
    DrawLine(surface, rect->left, rect->top, rect->left, rect->bottom, light);
    DrawLine(surface, rect->right, rect->top + 1, rect->right, rect->bottom, dark);
    DrawLine(surface, rect->left + 1, rect->bottom, rect->right, rect->bottom, dark);
}

// FUNCTION: 0x4b0590
void __stdcall DrawSunkenBox(void* surface, Rect* rect, int light, int dark, int fill)
{
    FillRectangle(surface, rect, fill);
    DrawLine(surface, rect->left, rect->top, rect->right, rect->top, dark);
    DrawLine(surface, rect->left, rect->top, rect->left, rect->bottom, dark);
    DrawLine(surface, rect->right, rect->top + 1, rect->right, rect->bottom, light);
    DrawLine(surface, rect->left + 1, rect->bottom, rect->right, rect->bottom, light);
}
