#ifndef DRAW_H
#define DRAW_H
struct GafFrame;
struct Rect;
struct Surface;
struct Vec3;

struct GafFrame;
struct Point;
struct Rect;
struct ScanlineSpan;
struct Surface;
struct Vec3;

int __stdcall ClipLine(Surface* dst, int* x0, int* y0, int* x1, int* y1);
unsigned int __stdcall ReadPixel(Surface* surface, int x, int y);
int __stdcall ScanFillPolygon(Surface* surface, Point* points, int n, unsigned char color);
int __stdcall DrawPolyline(Surface* surface, Point* points, int count, int color);
int __stdcall DrawPolylines(Surface* surface, Point* points, int* counts, int rows, int color);
int __stdcall ClipRectangle(Surface* surface, Rect* r);
int __stdcall FillRectangle(Surface* surface, Rect* rect, int color);
int __stdcall XorRectangle(Surface* surface, Rect* rect, int value);
int __stdcall DrawPolygon(Surface* surface, Vec3* pts, int count, int color);
void __stdcall FillShadedSpan(int row, ScanlineSpan* span, GafFrame* surf, unsigned char color);
int __stdcall PointInPolygon(Point* pts, int n, int px, int py);
void __stdcall SetTextKeyColor(int value);
int __stdcall GetTextForeColor();
int __stdcall GetTextBackColor();
void __stdcall PlotSpanEnds(int row, ScanlineSpan* span, GafFrame* surf, unsigned char color);
void __stdcall FillFlatSpan(int row, ScanlineSpan* span, GafFrame* surf, unsigned char color);

Surface* __stdcall DrawLitLine(Surface* surface, int x0, int y0, int x1, int y1, int color);
Surface* __stdcall DrawBlendedLine(Surface* surface, int x0, int y0, int x1, int y1, int color);
int __stdcall DrawPixel(Surface* surface, int x, int y, int ch);
int __stdcall FadeRectangle(Surface* surface, Rect* rect, int level);
int __stdcall GrayRectangle(Surface* surface, Rect* rect);
int __stdcall DitherRectangle(Surface* surface, Rect* rect, int phase);
void __stdcall DrawCircle(Surface* surface, int xc, int yc, int radius, int color);
void __stdcall DrawDashedCircle(Surface* surface, int cx, int cy, int radius, int color, int n, int start);
int __stdcall DrawPolygonEdges(GafFrame* surf, Vec3* pts, int count, int color);
int __stdcall FillFlatPolygon(GafFrame* surf, Vec3* pts, int n, int color);
int __stdcall GetTextKeyColor();
int __stdcall GetFont();
int __stdcall GetFontHeight();

#endif
