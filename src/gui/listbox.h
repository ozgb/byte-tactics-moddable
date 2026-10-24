#ifndef LISTBOX_H
#define LISTBOX_H

struct Rect;

void __stdcall DrawBevelBorder(void* surface, Rect* rect, int light, unsigned char dark, int unused);
void __stdcall DrawBevelBorderDarkFirst(void* surface, Rect* rect, int dark, int light, int unused);

#endif
