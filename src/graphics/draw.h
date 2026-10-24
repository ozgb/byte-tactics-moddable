#ifndef DRAW_H
#define DRAW_H

struct GafFrame;
struct Point;
struct Rect;
struct Span;
struct Surface;
struct Vec3;

int __stdcall ClipLine(Surface* dst, int* x0, int* y0, int* x1, int* y1);
unsigned int __stdcall ReadPixel(Surface* surface, int x, int y);
int __stdcall DrawPolyline(Surface* surface, Point* points, int count, int color);
int __stdcall DrawPolylines(Surface* surface, Point* points, int* counts, int rows, int color);
int __stdcall ClipRectangle(Surface* surface, Rect* r);
int __stdcall XorRectangle(Surface* surface, Rect* rect, int value);
int __stdcall DrawPolygon(Surface* surface, Vec3* pts, int count, int color);
void __stdcall FillShadedSpan(int row, Span* span, GafFrame* surf, unsigned char color);
int __stdcall PointInPolygon(Point* pts, int n, int px, int py);
void __stdcall SetTextKeyColor(int value);
int __stdcall GetTextForeColor();
int __stdcall GetTextBackColor();
void __stdcall PlotSpanEnds(int row, Span* span, GafFrame* surf, unsigned char color);
void __stdcall FillFlatSpan(int row, Span* span, GafFrame* surf, unsigned char color);

#endif
