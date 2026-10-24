#pragma pack(push, 1)
struct Gadget_004ac970 {
    char unknown_0[0x13];
    short x;
    short y;
    short width;
    short height;
    int attribs;
    char unknown_1f[0xb6 - 0x1f];
    short count;
    char unknown_b8[0xba - 0xb8];
    short selected;
    void* surface;
    char unknown_c0[0x15b - 0xc0];
};
#pragma pack(pop)

struct Holder_004ac970 {
    int next;
    Gadget_004ac970* gadgets;
};

struct Object_004ac970 {
    char unknown_0[0x18];
    Holder_004ac970* holder;
};

#include "../graphics/rect.h"

int __stdcall FindGadgetIndex(void* gadgets, const char* name, int flag);
void __stdcall DrawRectangle(void* surface, void* rect, int color);

static inline int CellX(Gadget_004ac970* gadgets, int index)
{
    return gadgets[index].x + gadgets->x;
}

// FUNCTION: 0x4ac970
void __stdcall DrawColorCellOutline(Object_004ac970* obj, int cell, int color)
{
    Gadget_004ac970* gadgets = obj->holder->gadgets;
    int index, found = FindGadgetIndex(gadgets, "COLS", 6);
    void* surface = gadgets->surface;
    index = found;
    short cellY = gadgets[index].y;
    int y = gadgets->y + cellY;
    Rect rect;
    rect.left = (unsigned int)CellX(gadgets, index) + (cell % 16) * 8;
    rect.top = y + (cell / 16) * 8;
    rect.right = rect.left + 7;
    rect.bottom = rect.top + 7;
    DrawRectangle(surface, &rect, color);
}
