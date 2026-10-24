#ifndef GAF_H
#define GAF_H
struct GafFrame;
struct Surface;

struct GafFrame;
struct Rect;

void __stdcall ClipRects(Rect* other, Rect* rect, Rect* bounds);
void* __stdcall BuildLensFrame(int w, int h, int lens);
void __stdcall GrabBackground(void* dst, GafFrame* sprite, int x, int y);
void __stdcall ZeroFramePixels(GafFrame* image);
void __stdcall CutOutFrame(GafFrame* src, GafFrame* dst, int x, int y);
int __stdcall CompressFrame(unsigned char* dest, GafFrame* img);
char* __stdcall EmitCopyRun(char* out, int count);
char* __stdcall EmitRepeatRun(char* out, int count, unsigned char a, unsigned char b);
int __stdcall CompressRow(char* dest, char* src, int width, unsigned char key);
void __stdcall DrawFrameDepth(GafFrame* src, GafFrame* dst, int x, int y, int level);
void __stdcall DownsampleFrame(GafFrame* src, GafFrame* dst);

void __stdcall DrawFrameOpaque(Surface* dst, GafFrame* bmp, int x, int y);
void __stdcall DrawFrameGray(Surface* surface, GafFrame* frame, int x, int y);
void __stdcall EraseFrameDithered(Surface* dst, GafFrame* bmp, int x, int y, int parity);
void __stdcall SurfaceFromFrame(Surface* dst, GafFrame* src);
GafFrame* __stdcall AllocFrame(const char* name, int width, int height);
void __stdcall ClearFrame(GafFrame* b, int color);
void __stdcall DrawFrameShadow(Surface* dst, GafFrame* bmp, int x, int y);
void __stdcall DrawLens(void* dst, GafFrame* sprite, int x, int y);
void __stdcall DrawFrameScaledBlended(Surface* dst, GafFrame* bmp, int x, int y, double sx, double sy);
void __stdcall DrawFrameScaled(Surface* dst, GafFrame* bmp, int x, int y, double sx, double sy);
void __stdcall DrawFrameLit(Surface* surface, GafFrame* frame, int x, int y, int paletteRow);

#endif
