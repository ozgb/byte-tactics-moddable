#ifndef SURFACE_H
#define SURFACE_H

#include "rect.h"
class HapiBank;
struct GafFrame;
struct Point;
struct tagRECT;

struct Display;
struct GafFrame;
struct Point;
struct Range;

struct Surface {
    int width;
    int height;
    int pitch;
    unsigned char* pixels;
    int zPriority;
    int colorKey;
    short x;
    short y;
    Rect clip;
    unsigned int flag0 : 1;
    unsigned int flag1 : 1;

    Surface() {}
    Rect* GetClipRect(Rect* out);
    void SetClipRect(Rect r);
};

void __stdcall SetLockEntry(Surface* surface, char flag);
void __stdcall PopScreenLock(int, int);
int __stdcall GetScreenLockCount(void);
void __stdcall UnlockAllScreens(void);
void __stdcall SetRestoreSurface(int surface);
void __stdcall DrawCursor(Display* obj, Surface* dst);
void __stdcall InitSurface(Surface* s, int width, int height, int a, int b);
void __stdcall SetDisplayFieldE4(int value);
void __stdcall DrawSurfaceKeyed(Surface* dst, Surface* src, int x, int y, int color);
void __stdcall CopySurfaceRectKeyed(Surface* dst, Surface* src, Rect* rect, Point* pos, unsigned char transparent);
void __stdcall InitSurfaceIndexFreeList(int count);
void __stdcall InterpEdgeUvFromScanline(int value, Range* out, int at_low, int at_high);
void __stdcall InterpEdgeUvFromScanline_B(int value, Range* out, int at_low, int at_high);
void __stdcall DrawTexturedSpan(int row, int* span, GafFrame* surf, GafFrame* info);
void __stdcall DrawLitTexturedSpan(int row, int* span, GafFrame* target, GafFrame* texture);

int __stdcall LockPrimary(Surface* out);
int __stdcall UnlockPrimary(Surface* unused, tagRECT* r1, tagRECT* r2);
int __stdcall SetPageFlipping(int enable);
void __stdcall RestoreScreen(void);
int __stdcall FillSurface(Surface* surface, int colour);
void __stdcall FreeSurface(Surface* obj);
int __stdcall GetDisplayFieldE4();
void __stdcall CopySurfaceRect(Surface* dst, Surface* src, Rect* rect, Point* pos);
void __stdcall SaveSurface(Surface* surface, HapiBank* file);
Surface* __stdcall LoadSurface(HapiBank* file);
void __stdcall DrawQuadRow(int row, int* rect, Surface* surf, GafFrame* info);
void __stdcall DrawTexturedPolygon(GafFrame* target, GafFrame* texture, int* vertices, int* coords);
void __stdcall DrawLitTexturedPolygon(GafFrame* target, GafFrame* texture, int* vertices, int* coords);

#endif
