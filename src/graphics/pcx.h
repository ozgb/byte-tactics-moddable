#ifndef PCX_H
#define PCX_H
struct Object3doLoadView;
struct Object3doPrimitive;
struct PcxBitmap;
struct PcxImage;
struct Vec3;

int __stdcall WritePcx(void* filename, unsigned char* data, int width, int height, unsigned char* block);
void* __stdcall Load3do(char* param);

int __stdcall LoadPcxPalette(char* path, unsigned int* out);
int __stdcall SaveScreenshot(char* dirPath, char* namePrefix);

int __stdcall DecodePcx(void* file, PcxImage* pcx);
void __stdcall SaveSurfacePcx(char* name, PcxBitmap* bitmap);
int __stdcall AveragePrimitiveY(Object3doLoadView* table, Object3doPrimitive* list);
void __stdcall SwapPrimitives(Object3doPrimitive* a, Object3doPrimitive* b);
void __stdcall SortPrimitives(Object3doLoadView* node);
void __stdcall RelocateObject(int delta, Object3doLoadView* node);
void __stdcall AddObjectBounds(Object3doLoadView* obj, Vec3* offset, Vec3* lo, Vec3* hi, int arg);
int __stdcall SaveBmp(char* name, PcxBitmap* image);

#endif
