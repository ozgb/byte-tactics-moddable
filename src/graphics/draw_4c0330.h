#ifndef DRAW_4C0330_H
#define DRAW_4C0330_H

struct Point;
struct Surface;

int __stdcall ScanFillPolygon(Surface* surface, Point* points, int n, unsigned char color);

#endif
