#ifndef ORDER_TARGETS_H
#define ORDER_TARGETS_H

struct Point16;
struct PathEvent;

Point16* __stdcall CopyDwordRangeUnchecked(Point16* first, Point16* last, Point16* dest);
void __stdcall ApplyPathEvent(const PathEvent* event);

#endif
