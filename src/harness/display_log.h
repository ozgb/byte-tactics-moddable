#ifndef DISPLAY_LOG_H
#define DISPLAY_LOG_H

struct DlSurface {
    const unsigned char* pixels;
    int width;
    int height;
    int pitch;
    int clipLeft;
    int clipTop;
    int clipRight;
    int clipBottom;
};

struct DlPicture {
    const void* key;
    const unsigned char* pixels;
    int width;
    int height;
    int compressed;
    int layered;
};

int DlWants();
int DlIsOn();
void DlEnter();
void DlLeave();
void DlLockRegistries();
void DlUnlockRegistries();
int DlOffsetX();
int DlOffsetY();

void DlOpen(int listAll);
void DlClose();
void DlFlush();
void DlFrameEnd(unsigned number, unsigned tick, unsigned steps, const unsigned char* palette, int list, const DlPicture* cursor,
                int cursorX, int cursorY);

void DlRegisterSurface(const void* block, const unsigned char* pixels, int width, int height, int pitch, const char* name);
void DlRegisterFrame(const void* block, const unsigned char* pixels, int width, int height, const char* name);
void DlFree(const void* block);
void DlGafBegin(const char* path, const void* base);
void DlGafFrame(const char* entry, int index, const void* frame);
void DlRegisterModelPiece(const void* object, const char* stem, const char* piece);
void DlRegisterFont(const void* font, const char* name);
const char* DlModelName(const void* object, int* known);
const char* DlPieceName(const void* object);
void DlFontName(const void* font, char* out, int cap);

void DlCutRect(const DlSurface* dst, int* x0, int* y0, int* x1, int* y1);
int DlBegin(const char* kind, const DlSurface* dst, int x0, int y0, int x1, int y1, const unsigned char* reads);
void DlEnd();
void DlPutRaw(const char* text);
void DlPutInt(int value);
void DlPutHex8(unsigned value);
void DlPutName(const char* bytes, int length);
void DlPutPicture(const DlPicture* picture);
void DlPictureReads(const DlPicture* picture, const unsigned char** reads);
void DlPutTarget(const DlSurface* surface);
const unsigned char* DlSourceBase(const DlSurface* surface);
void DlSourceOffset(const DlSurface* surface, int* x, int* y);

#endif
