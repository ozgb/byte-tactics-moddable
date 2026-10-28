#ifndef PCX_H
#define PCX_H

struct Bitmap_004caec0;

class BmpWriter {
public:
    int width;
    int height;
    int dataOffset;
    void* file;

    BmpWriter* Init();
    void Close();
    bool Open(const char* name, int w, int h);
    bool WriteRows(Bitmap_004caec0* image, int x, int rows, int unused_4, int y, int unused_6, int srcY);
};

int __stdcall WritePcx(void* filename, unsigned char* data, int width, int height, unsigned char* block);
void* __stdcall Load3do(char* param);

int __stdcall LoadPcxPalette(char* path, unsigned int* out);
int __stdcall SaveScreenshot(char* dirPath, char* namePrefix);

#endif
