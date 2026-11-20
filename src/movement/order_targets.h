#ifndef ORDER_TARGETS_H
#define ORDER_TARGETS_H

struct Point16;
struct PathfindingEventHeader;

Point16* __stdcall CopyDwordRangeUnchecked(Point16* first, Point16* last, Point16* dest);
enum { PATH_WAYPOINT_CAPACITY = 64 };

void __stdcall ApplyPathEvent(const PathfindingEventHeader* event, unsigned int size, const PathPoint* waypoints);

#endif
