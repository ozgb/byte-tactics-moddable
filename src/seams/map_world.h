#ifndef MAP_WORLD_H
#define MAP_WORLD_H

#include "../map/unit_visitor.h"

struct Vec3;

void MapInbox(const void* event);
void __stdcall VisitUnitsInRange(const Vec3* pos, int range, UnitVisitor* visitor);

#endif
