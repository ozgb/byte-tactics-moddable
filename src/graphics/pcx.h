#ifndef PCX_H
#define PCX_H
struct Object3do;
struct PieceFrame;
struct PcxBitmap;
struct PcxImage;
struct Vec3;
struct Surface;

class BmpWriter {
public:
    int width;
    int height;
    int dataOffset;
    void* file;

    BmpWriter* Init();
    void Close();
    bool Open(const char* name, int w, int h);
    bool WriteRows(Surface* image, int x, int rows, int unused_4, int y,
                      int unused_6, int srcY);
};

int __stdcall WritePcx(void* filename, unsigned char* data, int width, int height, unsigned char* block);
void* __stdcall Load3do(char* param);

int __stdcall LoadPcxPalette(char* path, unsigned int* out);
int __stdcall SaveScreenshot(char* dirPath, char* namePrefix);

int __stdcall DecodePcx(void* file, PcxImage* pcx);
void __stdcall SaveSurfacePcx(char* name, PcxBitmap* bitmap);
int __stdcall AveragePrimitiveY(Object3do* table, PieceFrame* list);
void __stdcall SwapPrimitives(PieceFrame* a, PieceFrame* b);
void __stdcall SortPrimitives(Object3do* node);
void __stdcall RelocateObject(int delta, Object3do* node);
void __stdcall GetObjectBounds(Object3do* obj, Vec3* lo, Vec3* hi, int arg);
void __stdcall AddObjectBounds(Object3do* obj, Vec3* offset, Vec3* lo, Vec3* hi, int arg);
int __stdcall SaveBmp(char* name, PcxBitmap* image);

int __stdcall ExceptionFilter(void* exception, const char* thread);

#endif
