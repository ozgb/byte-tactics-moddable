#ifndef MARKERS_H
#define MARKERS_H

struct Order;
struct Pos;
struct View;

void __stdcall DrawBuildFootprint(void* surface, View* view, Order* order, Pos* out, int unused);
void __stdcall DrawPathAnim(void* surface, View* view, Order* order, Pos* out, int flag);
void __stdcall DrawWeaponCoverage(void* surface, View* view, Order* order, Pos* out, int unused);
void __stdcall DrawOrderRangeRing(void* surface, View* view, Order* order, Pos* out, int unused);

#endif
