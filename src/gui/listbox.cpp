#include "../graphics/rect.h"
#include "../graphics/gaf_frame.h"

struct DrawListboxFrameGafEntry {
    unsigned short count;
    char unknown_2[0x28 - 0x2];
    GafFrame* child[1];
};

struct DrawListboxFrameCell {
    int step_x;
    int step_y;
};

struct DrawListboxFrameSurface {
    int tiles_x;
    int tiles_y;
    char unknown_8[0xbc - 0x8];
};

int __stdcall DrawLine(void* surface, int x0, int y0, int x1, int y1, unsigned char color);
#include "gadget.h"
#include "layer.h"
#include "gui.h"
#include "../graphics/draw.h"
DrawListboxFrameGafEntry* __stdcall FindGafEntry(void* gaf, const char* name);
void __stdcall DrawSurface(DrawListboxFrameSurface* dst, DrawListboxFrameCell* cell, int x, int y);
GafFrame* __stdcall GetGafFrame(DrawListboxFrameGafEntry* bits, int index);
void __stdcall DrawFrame(DrawListboxFrameSurface* dst, GafFrame* pic, int x, int y);

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
void __stdcall DrawListboxFrame(Gui* obj, int index, DrawListboxFrameGafEntry* bmp)
{
    Gadget* entries;
    int col;
    DrawListboxFrameSurface* surface;
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
    DrawListboxFrameCell* cell;
    int h;
    GafFrame* sub;
    entries = obj->layer->entries;
    GetGadgetRectByIndex((char*)entries, index, &rect);

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

    surface = (DrawListboxFrameSurface*)entries->u.assets.surface;
    cell = (DrawListboxFrameCell*)FindBackgroundCell(entries, index);

    if (cell != 0) {
        for (x = 0; x < surface->tiles_x; x += cell->step_x) {
            for (y = 0; y < surface->tiles_y; y += cell->step_y) {
                DrawSurface(surface, cell, x, y);
            }
        }
        DrawBevelBorderDarkFirst(surface, &rect, obj->colours[0], obj->colours[0x11], obj->colours[0x14]);
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
        FillRectangle((Surface*)surface, &rect, obj->colours[0x14]);
        DrawBevelBorderDarkFirst(surface, &rect, obj->colours[0], obj->colours[0x11], obj->colours[0x14]);
    }
}

// FUNCTION: 0x4b04b0
void __stdcall FillBevelBox(void* surface, Rect* rect, int light, int dark, int fill)
{
    FillRectangle((Surface*)surface, rect, fill);
    DrawBevelBorder(surface, rect, light, dark, fill);
}

// FUNCTION: 0x4b04e0
void __stdcall FillBevelBoxDarkFirst(void* surface, Rect* rect, int light, int dark, int fill)
{
    FillRectangle((Surface*)surface, rect, fill);
    DrawBevelBorderDarkFirst(surface, rect, light, dark, fill);
}

// FUNCTION: 0x4b0510
void __stdcall DrawRaisedBox(void* surface, Rect* rect, int light, unsigned char dark, int fill)
{
    FillRectangle((Surface*)surface, rect, fill);
    DrawLine(surface, rect->left, rect->top, rect->right, rect->top, light);
    DrawLine(surface, rect->left, rect->top, rect->left, rect->bottom, light);
    DrawLine(surface, rect->right, rect->top + 1, rect->right, rect->bottom, dark);
    DrawLine(surface, rect->left + 1, rect->bottom, rect->right, rect->bottom, dark);
}

// FUNCTION: 0x4b0590
void __stdcall DrawSunkenBox(void* surface, Rect* rect, int light, int dark, int fill)
{
    FillRectangle((Surface*)surface, rect, fill);
    DrawLine(surface, rect->left, rect->top, rect->right, rect->top, dark);
    DrawLine(surface, rect->left, rect->top, rect->left, rect->bottom, dark);
    DrawLine(surface, rect->right, rect->top + 1, rect->right, rect->bottom, light);
    DrawLine(surface, rect->left + 1, rect->bottom, rect->right, rect->bottom, light);
}
