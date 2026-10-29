#include <string.h>
#include "../../interfaces/host.h"

struct PcxImage {
    unsigned char* data;
    unsigned char* palette;
    int width;
    int height;
};

#include "../files/files_api.h"
#include "../debug/debug_lib.h"

static inline void FUN_row(void* file, unsigned char* p, int x)
{
    unsigned char b;
    unsigned char run;
    while (x > 0) {
        HAPI_readfromfile(file, &b, 1);
        if ((b & 0xc0) == 0xc0) {
            run = b & 0x3f;
            x -= run;
            if (x < 0)
                run += x;
            HAPI_readfromfile(file, &b, 1);
            if (run == 1) {
                *p = b;
                p++;
            } else {
                memset(p, b, run);
                p += run;
            }
        } else {
            *p = b;
            p++;
            x--;
        }
    }
}

// FUNCTION: 0x4caa40
int __stdcall DecodePcx(void* file, PcxImage* pcx)
{
    unsigned char header[0x80];
    memset(pcx, 0, 16);
    if (HAPI_readfromfile(file, header, 0x80) == 0x80 && header[0] == 0x0a && header[1] == 5) {
        pcx->width = *(unsigned short*)(header + 8) - *(unsigned short*)(header + 4) + 1;
        pcx->height = *(unsigned short*)(header + 10) - *(unsigned short*)(header + 6) + 1;
        pcx->data = (unsigned char*)GameAllocIgnoreTag("PCX BODY", pcx->width * pcx->height);
        pcx->palette = (unsigned char*)GameAllocIgnoreTag("COLOR MAP", 0x300);
        HAPI_SeekFile(file, HAPI_FileLength(file) - 0x300);
        HAPI_readfromfile(file, pcx->palette, 0x300);
        HAPI_SeekFile(file, 0x80);
        {
            unsigned char* p = pcx->data;
            int rows;
            int w;
            w = pcx->width;
            for (rows = pcx->height - 1; rows >= 0; rows--) {
                FUN_row(file, p, w);
                p += w;
            }
        }
        return 1;
    }
    return 0;
}

#include <stdio.h>

struct PcxHeader {
    unsigned char a;
    unsigned char b;
    unsigned char c;
    unsigned char d;
    unsigned short e;
    unsigned short f;
    short g;
    short h;
    short i;
    short j;
    unsigned char palette[0x30];
    unsigned char k;
    unsigned char l;
    unsigned short m;
    unsigned short n;
    unsigned char rest[0x3a];
};

// FUNCTION: 0x4cac40
int __stdcall WritePcx(void* filename, unsigned char* data, int width, int height, unsigned char* block)
{
    PcxHeader hdr;
    FileHandle* file = HAPI_OpenFileAppend((char*)filename);
    int total;
    int rows;
    int n;
    unsigned char* row;
    unsigned char* p;
    int run;
    int wrote;
    unsigned char cur;
    unsigned char next;
    unsigned char rep;
    unsigned char lit;
    unsigned char cnt;

    if (file == 0)
        goto out;

    memset(&hdr, 0, 0x80);
    hdr.a = 10;
    hdr.b = 5;
    hdr.c = 1;
    hdr.d = 8;
    hdr.e = 0;
    hdr.f = 0;
    hdr.g = (short)(width - 1);
    hdr.h = (short)(height - 1);
    hdr.i = (short)width;
    hdr.j = (short)height;
    memcpy(hdr.palette, block, 0x30);
    hdr.n = 0;
    hdr.l = 1;
    hdr.m = (unsigned short)width;
    if (HAPI_WriteFile(file, &hdr, 0x80) != 0x80)
        goto out;

    rows = height - 1;
    row = data;
    if (rows >= 0) {
        ++rows;
        do {
            unsigned char t;
            total = 0;
            cur = *row;
            run = 1;
            p = row + 1;
            if (width > 1) {
                n = width - 1;
                do {
                    next = *p++;
                    if (cur == next) {
                        run++;
                    } else {
                        wrote = 0;
                        if (run == 1 && (cur & 0xc0) != 0xc0) {
                            lit = cur;
                            HAPI_WriteFile(file, &lit, 1);
                            wrote = run;
                        } else {
                            while (run > 0) {
                                int chunk = run > 0x3f ? 0x3f : run;
                                cnt = (unsigned char)(chunk | 0xc0);
                                HAPI_WriteFile(file, &cnt, 1);
                                rep = cur;
                                HAPI_WriteFile(file, &rep, 1);
                                run -= chunk;
                                wrote += 2;
                            }
                        }
                        total += wrote;
                        cur = next;
                        run = 1;
                    }
                } while (--n);
            }
            if (run == 1 && (cur & 0xc0) != 0xc0) {
                t = cur;
                HAPI_WriteFile(file, &t, 1);
            } else {
                unsigned char ocnt;
                while (run > 0) {
                    int chunk = run > 0x3f ? 0x3f : run;
                    ocnt = (unsigned char)(chunk | 0xc0);
                    HAPI_WriteFile(file, &ocnt, 1);
                    rep = cur;
                    HAPI_WriteFile(file, &rep, 1);
                    run -= chunk;
                }
            }
            row += width;
        } while (--rows);
    }

    {
        unsigned char marker = 0x0c;
        HAPI_WriteFile(file, &marker, 1);
    }
    if (HAPI_WriteFile(file, block, 0x300) != 0x300)
        goto out;
    HAPI_CloseFile(file);
    return 1;

out:
    if (file)
        HAPI_CloseFile(file);
    return 0;
}

#include "win_types.h"

struct PcxBitmap {
    int width;
    int height;
    int pitch;
    unsigned char* data;
};

#pragma pack(push, 1)
struct DisplayScreenshotView {
    char unknown_0[0x40];
    HWND hwnd;
    char unknown_44[0xbc - 0x44];
    PcxBitmap* bitmap;
    char unknown_c0[0x1c];
    int useOverrideSurface;
    char unknown_e0[0x1b2 - 0xe0];
    int cursorFrame;
    char unknown_1b6[0x214 - 0x1b6];
    PALETTEENTRY palette[256];
};
#pragma pack(pop)
static_assert(sizeof(DisplayScreenshotView) == 0x614, "DisplayScreenshotView size");

DisplayScreenshotView* GetDisplay();

// FUNCTION: 0x4caec0
void __stdcall SaveSurfacePcx(char* name, PcxBitmap* bitmap)
{
    unsigned char pal[256 * 3];
    DisplayScreenshotView* game = GetDisplay();
    for (int i = 0; i < 256; i++) {
        pal[i * 3] = game->palette[i].peRed;
        pal[i * 3 + 1] = game->palette[i].peGreen;
        pal[i * 3 + 2] = game->palette[i].peBlue;
    }
    WritePcx(name, bitmap->data, bitmap->width, bitmap->height, pal);
}

PcxBitmap* __stdcall AllocSurface(char* name, int width, int height);
#include "surface.h"
#include "../util/vec3.h"
#include "object_pool.h"

// FUNCTION: 0x4caf30
PcxBitmap* __stdcall LoadPcx(char* path, unsigned char* outPalette)
{
    FileHandle* file = HAPI_OpenFileRead(path);
    if (file == 0)
        return 0;
    int ok;
    PcxImage pcx;
    unsigned char header[0x80];
    if (HAPI_readfromfile(file, header, 0x80) != 0x80 || header[0] != 0x0a || header[1] != 5) {
        HAPI_CloseFile(file);
        return 0;
    }
    int width = *(unsigned short*)(header + 8) - *(unsigned short*)(header + 4) + 1;
    int height = *(unsigned short*)(header + 10) - *(unsigned short*)(header + 6) + 1;
    PcxBitmap* image = AllocSurface(path, width, height);
    if (image == 0) {
        HAPI_CloseFile(file);
        return 0;
    }
    HAPI_SeekFile(file, 0);
    ok = DecodePcx(file, &pcx);
    HAPI_CloseFile(file);
    if (ok) {
        memcpy(image->data, pcx.data, height * width);
    }
    if (outPalette != 0) {
        unsigned char* s = pcx.palette;
        for (int i = 0; i < 0x100; i++) {
            outPalette[0] = s[0];
            outPalette[1] = s[1];
            outPalette[2] = s[2];
            outPalette[3] = 0;
            outPalette += 4;
            s += 3;
        }
    }
    GameFreeThunk(pcx.palette);
    GameFreeThunk(pcx.data);
    if (!ok) {
        FreeSurface((Surface *)image);
    }
    return image;
}

// FUNCTION: 0x4cb080
int __stdcall LoadPcxPalette(char* path, unsigned int* out)
{
    FileHandle* file = HAPI_OpenFileRead(path);
    if (file == 0)
        return 0;
    PcxImage pcx;
    unsigned char header[0x80];
    PALETTEENTRY pal[0x100];
    if (HAPI_readfromfile(file, header, 0x80) != 0x80) {
        HAPI_CloseFile(file);
        return 0;
    }
    if (header[0] != 0x0a || header[1] != 5) {
        HAPI_CloseFile(file);
        return 0;
    }
    HAPI_SeekFile(file, 0);
    DecodePcx(file, &pcx);
    HAPI_CloseFile(file);
    unsigned char* s = pcx.palette;
    for (int i = 0; i < 0x100; i++) {
        pal[i].peRed = s[0];
        pal[i].peGreen = s[1];
        pal[i].peBlue = s[2];
        pal[i].peFlags = 0;
        s += 3;
    }
    memcpy(out, pal, 0x400);
    GameFreeThunk(pcx.palette);
    GameFreeThunk(pcx.data);
    return 1;
}

#include <io.h>
#include <stdlib.h>
#include "../state/sim_globals.h"

struct ScreenshotFileFindData {
    char unknown_0[0x14];
    char name[260];
};

#include "../../link/globals.h"


// FUNCTION: 0x4cb170
int __stdcall SaveScreenshot(char* dirPath, char* namePrefix)
{
    DisplayScreenshotView* game = GetDisplay();
    int flag = 0;
    char filename[260];
    ScreenshotFileFindData fd;
    unsigned char pal[768];
    int best = 0;

    char c = dirPath[0];
    if (c != '\0') {
        int len = strlen(dirPath);
        if (dirPath[len - 1] != '\\')
            flag = 1;
    }
    if (game->useOverrideSurface == 0)
        return 0;

    const char* sep = flag ? g_pathSepBackslash : DAT_005119b8;
    sprintf(filename, "%s%s%s*.pcx", dirPath, sep, namePrefix);

    int handle = HAPI_FindFirst(filename, &fd, -1, 1);
    if (handle >= 0) {
        do {
            int n = atoi(fd.name + strlen(namePrefix));
            if (n > best)
                best = n;
        } while (HAPI_FindNext(handle, &fd) == 0);
        HAPI_FindClose(handle);
    }

    sep = flag ? g_pathSepBackslash : DAT_005119b8;
    sprintf(filename, "%s%s%s%04i.pcx", dirPath, sep, namePrefix, best + 1);

    PcxBitmap* bitmap = game->bitmap;
    DisplayScreenshotView* g = GetDisplay();
    for (int i = 0; i < 256; i++) {
        pal[i * 3] = g->palette[i].peRed;
        pal[i * 3 + 1] = g->palette[i].peGreen;
        pal[i * 3 + 2] = g->palette[i].peBlue;
    }
    return WritePcx(filename, bitmap->data, bitmap->width, bitmap->height, pal);
}

struct Object3doPrimitive {
    int colorIndex;
    int count;
    int always0;
    union {
        int ptr_c;
        unsigned short* indices;
    };
    int ptr_10;
    int unknown_14[3];
};

struct Object3doLoadView {
    int version;
    int vertexCount;
    int count;
    int index;
    int x;
    int y;
    int z;
    int name;
    int always0;
    Vec3* vertices;
    Object3doPrimitive* elems;
    Object3doLoadView* sibling;
    Object3doLoadView* child;
};

// FUNCTION: 0x4cb2f0
int __stdcall AveragePrimitiveY(Object3doLoadView* table, Object3doPrimitive* list)
{
    int sum = 0;
    int n = list->count;
    unsigned short* p = list->indices;
    for (int i = 0; i < n; i++) {
        sum += table->vertices[*p++].y;
    }
    return sum / n;
}

// FUNCTION: 0x4cb330
void __stdcall SwapPrimitives(Object3doPrimitive* a, Object3doPrimitive* b)
{
    Object3doPrimitive tmp = *a;
    *a = *b;
    *b = tmp;
}

// FUNCTION: 0x4cb370
void __stdcall SortPrimitives(Object3doLoadView* node)
{
    if (node->index != -1 && node->count > 0) {
        Object3doPrimitive* elems = node->elems;
        Object3doPrimitive temp = elems[node->index];
        elems[node->index] = elems[0];
        elems[0] = temp;
        node->index = 0;
    }
    Object3doPrimitive* e;
    int i;
    int swapped;
    do {
        swapped = 0;
        e = node->elems + 1;
        for (i = 1; i < node->count - 1; i++, e++) {
            unsigned short* q = e->indices;
            int a = 0;
            for (int k = e->count; k > 0; k--)
                a += node->vertices[*q++].y;
            a /= e->count;
            Object3doPrimitive* p = e + 1;
            unsigned short* r = p->indices;
            int b = 0;
            for (int j = p->count; j > 0; j--)
                b += node->vertices[*r++].y;
            b /= p->count;
            if (a > b) {
                Object3doPrimitive t = *e;
                *e = *p;
                *p = t;
                swapped = 1;
            }
        }
    } while (swapped);
}

// FUNCTION: 0x4cb4c0
void __stdcall RelocateObject(int delta, Object3doLoadView* node)
{
    if (node->name)
        node->name += delta;
    if (node->always0)
        node->always0 += delta;
    node->vertices = (Vec3*)((char*)node->vertices + delta);
    node->elems = (Object3doPrimitive*)((char*)node->elems + delta);
    if (node->sibling) {
        node->sibling = (Object3doLoadView*)((char*)node->sibling + delta);
        RelocateObject(delta, node->sibling);
    }
    if (node->child) {
        node->child = (Object3doLoadView*)((char*)node->child + delta);
        RelocateObject(delta, node->child);
    }
    Object3doPrimitive* e = node->elems;
    for (int i = 0; i < node->count; i++, e++) {
        if (e->always0)
            e->always0 += delta;
        e->ptr_c += delta;
        if (e->ptr_10)
            e->ptr_10 += delta;
    }
    SortPrimitives(node);
}


// FUNCTION: 0x4cb560
void* __stdcall Load3do(char* param)
{
    void* result = HAPI_LoadFile(param, 0);
    if (result == 0) {
        return 0;
    }
    RelocateObject((int)result, (Object3doLoadView*)result);
    return result;
}

// FUNCTION: 0x4cb590
void __stdcall MirrorObject(Object3doLoadView* obj)
{
    for (int i = 0; i < obj->vertexCount; i++) {
        obj->vertices[i].x = -obj->vertices[i].x;
        obj->vertices[i].z = -obj->vertices[i].z;
    }
    obj->x = -obj->x;
    obj->z = -obj->z;
    if (obj->sibling)
        MirrorObject(obj->sibling);
    if (obj->child)
        MirrorObject(obj->child);
}

// FUNCTION: 0x4cb5f0
int __stdcall GetObjectHeight(Object3doLoadView* node)
{
    int best = 0;
    for (; node; node = node->sibling) {
        for (int i = 0; i < node->vertexCount; i++) {
            int v = node->vertices[i].y + node->y;
            if (v > best)
                best = v;
        }
        if (node->child) {
            int v = GetObjectHeight(node->child) + node->y;
            if (v > best)
                best = v;
        }
    }
    return best;
}

#include "pcx.h"

// FUNCTION: 0x4cb650
void __stdcall GetObjectBounds(Object3doLoadView* obj, Vec3* lo,
                            Vec3* hi, int arg)
{
    Vec3 offset;
    lo->x = 0;
    lo->y = 0;
    lo->z = 0;
    hi->x = 0;
    hi->y = 0;
    hi->z = 0;
    offset.x = 0;
    offset.y = 0;
    offset.z = 0;
    AddObjectBounds(obj, &offset, lo, hi, arg);
}

// FUNCTION: 0x4cb6a0
void __stdcall AddObjectBounds(Object3doLoadView* obj, Vec3* offset,
                            Vec3* lo, Vec3* hi, int arg)
{
    Vec3 local;
    local.x = obj->x + offset->x;
    local.y = offset->y + obj->y;
    local.z = obj->z + offset->z;
    if (obj->vertexCount > 2) {
        for (int i = 0; i < obj->vertexCount; i++) {
            Vec3* v = &obj->vertices[i];
            if (v->x + local.x > hi->x) hi->x = v->x + local.x;
            if (v->x + local.x < lo->x) lo->x = v->x + local.x;
            if (v->y + local.y > hi->y) hi->y = v->y + local.y;
            if (v->y + local.y < lo->y) lo->y = v->y + local.y;
            if (v->z + local.z > hi->z) hi->z = v->z + local.z;
            if (v->z + local.z < lo->z) lo->z = v->z + local.z;
        }
    }
    if (arg != 0) {
        if (obj->child != 0)
            AddObjectBounds(obj->child, &local, lo, hi, arg);
        if (obj->sibling != 0)
            AddObjectBounds(obj->sibling, offset, lo, hi, arg);
    }
}

class BmpWriter {
public:
    int width;
    int height;
    int dataOffset;
    void* file;

    BmpWriter* Init();
    void Close();
    bool Open(const char* name, int w, int h);
    bool WriteRows(PcxBitmap* image, int x, int rows, int unused_4, int y,
                      int unused_6, int srcY);
};

// FUNCTION: 0x4cb7c0
BmpWriter* BmpWriter::Init()
{
    file = 0;
    return this;
}

// FUNCTION: 0x4cb7d0
void BmpWriter::Close()
{
    if (file != NULL) {
        host_file_close(file);
    }
}

#pragma pack(push, 2)
struct BmpFileHeader {
    unsigned short bfType;
    unsigned int bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int bfOffBits;
};
#pragma pack(pop)

struct BmpInfoHeader {
    unsigned int biSize;
    int biWidth;
    int biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned int biCompression;
    unsigned int biSizeImage;
    int biXPelsPerMeter;
    int biYPelsPerMeter;
    unsigned int biClrUsed;
    unsigned int biClrImportant;
};

struct RgbQuad {
    unsigned char blue;
    unsigned char green;
    unsigned char red;
    unsigned char reserved;
};

struct BmpInfo {
    BmpInfoHeader header;
    RgbQuad colors[256];
};

// FUNCTION: 0x4cb7f0
bool BmpWriter::Open(const char* name, int w, int h)
{
    void* palbase = GetDisplay();

    width = w;
    height = h;
    file = host_file_create(name, 0);
    if (file == NULL) {
        return false;
    }
    BmpFileHeader fh;
    fh.bfType = 0x4d42;
    fh.bfSize = 0;
    fh.bfReserved1 = 0;
    fh.bfReserved2 = 0;
    fh.bfOffBits = 0x436;
    if (host_file_write(file, &fh, sizeof(fh)) != sizeof(fh)) {
        return false;
    }
    BmpInfo info;
    info.header.biSize = 0x28;
    info.header.biWidth = w;
    info.header.biHeight = h;
    info.header.biPlanes = 1;
    info.header.biBitCount = 8;
    info.header.biCompression = 0;
    info.header.biSizeImage = 0;
    info.header.biXPelsPerMeter = 3000;
    info.header.biYPelsPerMeter = 3000;
    info.header.biClrUsed = 0x100;
    info.header.biClrImportant = 0x100;
    unsigned char* dp = (unsigned char*)info.colors + 1;
    unsigned char* sp = (unsigned char*)palbase + 0x215;
    for (int i = 0; i < 256; i++) {
        dp[1] = sp[-1];
        dp[0] = sp[0];
        dp[-1] = sp[1];
        dp += 4;
        sp += 4;
    }
    if (host_file_write(file, &info, sizeof(info)) != sizeof(info)) {
        return false;
    }
    dataOffset = host_file_seek(file, 0, HOST_SEEK_CUR);
    return true;
}

// FUNCTION: 0x4cb940
bool BmpWriter::WriteRows(PcxBitmap* image, int x, int rows, int unused_4,
                                  int y, int unused_6, int srcY)
{
    int stride = (image->width + 3) & ~3;
    if (host_file_seek(file, (height - rows - y) * stride + dataOffset, HOST_SEEK_SET) == -1) {
        return false;
    }
    for (int i = rows - 1; i >= 0; i--) {
        if (host_file_write(file, image->data + image->pitch * (i + srcY), stride) != stride) {
            return false;
        }
    }
    return true;
}

// FUNCTION: 0x4cb9e0
int __stdcall SaveBmp(char* name, PcxBitmap* image)
{
    BmpWriter bmp;
    bool ok;
    bmp.file = 0;
    if (!bmp.Open(name, image->width, image->height)) {
        if (bmp.file) host_file_close(bmp.file);
        return 0;
    }
    {
        int stride = (image->width + 3) & ~3;
        int n = image->height;
        if (host_file_seek(bmp.file, (bmp.height - n) * stride + bmp.dataOffset, HOST_SEEK_SET) == -1) {
            ok = false;
            goto done;
        }
        for (int i = n - 1; i >= 0; i--) {
            if (host_file_write(bmp.file, image->data + i * image->pitch, stride) != stride) {
                ok = false;
                goto done;
            }
        }
        ok = true;
    }
done:
    if (!ok) {
        if (bmp.file) host_file_close(bmp.file);
        return 0;
    }
    if (bmp.file) host_file_close(bmp.file);
    return 1;
}


#include "display.h"
#include "surface.h"

static inline LONG Lock()
{
    while (1) {
        LONG r = host_atomic_exchange((int*)&g_gfxBlitLockHeld, 0x4d41494e);
        if (r == 0) {
            g_gfxBlitLockOwner = 0x4d41494e;
            return 0;
        }
        if (g_gfxBlitLockOwner == 0x4d41494e)
            return r;
        host_event_wait(g_gfxBlitLockEvent, HOST_WAIT_FOREVER);
    }
}

static inline void Unlock(LONG held)
{
    if (held == 0) {
        g_gfxBlitLockOwner = 0;
        host_atomic_exchange((int*)&g_gfxBlitLockHeld, 0);
        host_event_set(g_gfxBlitLockEvent);
    }
}

// FUNCTION: 0x4cbab0
int __stdcall ExceptionFilter(void* exception, const char* thread)
{
    host_crash_report(exception, thread);
    DisplayScreenshotView* app = GetDisplay();
    if (app != 0) {
        LONG held = Lock();
        app->cursorFrame = 0;
        UnlockAllScreens();
        Unlock(held);
        ReleaseDirectDraw((DisplayAppView *)app);
        if (app->hwnd != 0) {
            host_window_set_topmost(0);
        }
    }
    return 0;
}
