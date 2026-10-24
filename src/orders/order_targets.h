#ifndef ORDER_TARGETS_H
#define ORDER_TARGETS_H

struct Point16;

Point16* __stdcall CopyDwordRangeUnchecked(Point16* first, Point16* last, Point16* dest);
void __stdcall CreatePathfinder();
void __stdcall DestroyPathfinder();

#endif
