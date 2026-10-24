#ifndef LISTBOX_H
#define LISTBOX_H
struct Rect;

struct Rect;

void __stdcall DrawBevelBorder(void* surface, Rect* rect, int light, unsigned char dark, int unused);
void __stdcall DrawBevelBorderDarkFirst(void* surface, Rect* rect, int dark, int light, int unused);

void __stdcall FillBevelBox(void* surface, Rect* rect, int light, int dark, int fill);
void __stdcall FillBevelBoxDarkFirst(void* surface, Rect* rect, int light, int dark, int fill);
void __stdcall DrawSunkenBox(void* surface, Rect* rect, int light, int dark, int fill);

#endif
