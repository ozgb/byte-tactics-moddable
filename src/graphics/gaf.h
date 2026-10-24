#ifndef GAF_H
#define GAF_H

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

#endif
