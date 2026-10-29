#include <string.h>
#include "../../interfaces/host.h"
#include "win_types.h"
#include "display_context.h"


struct RGBA {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};

DisplayContext* GetDisplay(void);
#include "../debug/debug_lib.h"
#include "object_pool.h"
#include "palette.h"

#include "../../link/globals.h"

static inline long Lock()
{
    while (1) {
        long r = host_atomic_exchange((int*)&g_gfxBlitLockHeld, 0x4d41494e);
        if (r == 0) {
            g_gfxBlitLockOwner = 0x4d41494e;
            return 0;
        }
        if (g_gfxBlitLockOwner == 0x4d41494e)
            return r;
        host_event_wait(g_gfxBlitLockEvent, HOST_WAIT_FOREVER);
    }
}

static inline void Unlock(long held)
{
    if (held == 0) {
        g_gfxBlitLockOwner = 0;
        host_atomic_exchange((int*)&g_gfxBlitLockHeld, 0);
        host_event_set(g_gfxBlitLockEvent);
    }
}

// FUNCTION: 0x4ba200
int __stdcall SetPaletteColors(unsigned char* src, int start, int count)
{
    long held = Lock();
    float brightness;
    DisplayContext* d;
    unsigned char local[0x400];
    int i;
    int end;
    d = GetDisplay();
    brightness = *(float*)((char*)d + 0x614);
    unsigned int* p = d->entries + start;
    end = start + count;
    unsigned char* s = src;
    for (i = start; i < end; i++, p++, s += 4)
        *p = *(unsigned int*)s;
    for (i = start; i < end; i++) {
        float v0 = src[i * 4] * brightness;
        if (v0 > 255.0)
            v0 = 255.0f;
        local[i * 4] = (unsigned char)(int)v0;
        float v1 = src[i * 4 + 1] * brightness;
        if (v1 > 255.0)
            v1 = 255.0f;
        local[i * 4 + 1] = (unsigned char)(int)v1;
        float v2 = src[i * 4 + 2] * brightness;
        if (v2 > 255.0)
            v2 = 255.0f;
        local[i * 4 + 2] = (unsigned char)(int)v2;
        local[i * 4 + 3] = 0;
    }

    if (d->windowed != 0) {
        unsigned char* buffer = (unsigned char*)GameAllocIgnoreTag("Palette", 0x408);
        for (i = 0; i < 256; i++)
            ((unsigned int*)buffer)[i] = ((unsigned int*)local)[i];
        host_palette_create(buffer);
        GameFreeThunk(buffer);
    } else if (d->gdi) {
        if (!host_palette_set(start, count, local + start * 4)) {
            Unlock(held);
            return 0;
        }
    }
    Unlock(held);
    return 1;
}

// FUNCTION: 0x4ba4c0
int __stdcall GetPaletteColors(unsigned char* dest, int first, int count)
{
    tagPALETTEENTRY pal[256];
    DisplayContext* d = GetDisplay();
    if (d->gdi) {
        if (!host_palette_get((unsigned char*)pal))
            return 0;
    }
    RGBA* out = (RGBA*)dest;
    count += first;
    for (int i = first; i < count; i++) {
        out[i].r = pal[i].peRed;
        out[i].g = pal[i].peGreen;
        out[i].b = pal[i].peBlue;
        out[i].a = 0;
    }
    return 1;
}

// FUNCTION: 0x4ba590
void __stdcall SetBrightness(int brightness)
{
    DisplayContext* p = GetDisplay();
    p->paletteBrightness = brightness;
    SetPaletteColors((unsigned char*)p->entries, 0, 0x100);
}

// FUNCTION: 0x4ba5c0
int __stdcall AllocAlphaTable(int obj)
{
    void* result = GameAllocIgnoreTag(g_alphaTableName, 0x10000);
    *(void**)((int)obj + 0xc0) = result;
    return 1;
}

// FUNCTION: 0x4ba5f0
void __stdcall FreeAlphaTable(DisplayContext* param)
{
    GameFreeThunk(param->alphaTable);
}

// FUNCTION: 0x4ba610
int __stdcall AllocShadeTable(DisplayContext* obj) {
    void* result = GameAllocIgnoreTag(g_shadeTableName, 0x2000);
    obj->shadeTable = result;
    return 1;
}

// FUNCTION: 0x4ba640
void __stdcall FreeShadeTable(DisplayContext* obj)
{
    GameFreeThunk(obj->shadeTable);
}

// FUNCTION: 0x4ba660
int __stdcall AllocLightTable(DisplayContext* obj)
{
    obj->lightTable = GameAllocIgnoreTag("LIGHT TABLE", 0x2000);
    return 1;
}

// FUNCTION: 0x4ba690
void __stdcall FreeLightTable(int obj)
{
    GameFreeThunk(*(void**)(obj + 0xc8));
}

// FUNCTION: 0x4ba6b0
int __stdcall AllocGrayTable(DisplayContext* obj)
{
    obj->grayTable = GameAllocIgnoreTag("GRAY TABLE", 0x100);
    return 1;
}

// FUNCTION: 0x4ba6e0
void __stdcall FreeGrayTable(DisplayContext* obj)
{
    GameFreeThunk(obj->grayTable);
}

// FUNCTION: 0x4ba700
int __stdcall AllocBlueTable(DisplayContext* obj)
{
    obj->blueTable = GameAllocIgnoreTag("BLUE TABLE", 0x100);
    return 1;
}

// FUNCTION: 0x4ba730
void __stdcall FreeBlueTable(DisplayContext* obj)
{
    GameFreeThunk(obj->blueTable);
}

// FUNCTION: 0x4ba750
unsigned int* __stdcall BuildAlphaTable(unsigned char* data)
{
    DisplayContext* obj = GetDisplay();
    if (obj->has_c0) {
        int sums[256];
        unsigned char idx[256];
        SortByBrightness(data, sums, idx);
        RGBA* pal = (RGBA*)data;
        int row = 0;
        int off = 0;
        for (; off < 0x10000; off += 0x100, row++) {
            for (int col = 0; col < 0x100; col++) {
                if (row == col) {
                    ((unsigned char*)obj->alphaTable)[off + col] = (unsigned char)row;
                } else {
                    tagPALETTEENTRY color;
                    color.peRed = (pal[row].r + pal[col].r) / 2;
                    color.peGreen = (pal[row].g + pal[col].g) / 2;
                    color.peBlue = (pal[row].b + pal[col].b) / 2;
                    ((unsigned char*)obj->alphaTable)[off + col] = NearestColorInBand((tagPALETTEENTRY*)data, sums, idx, color);
                }
            }
        }
        return (unsigned int*)obj->alphaTable;
    }
    return 0;
}

// FUNCTION: 0x4ba880
unsigned char __stdcall NearestColor(tagPALETTEENTRY* palette, tagPALETTEENTRY color)
{
    int best = 1000000000;
    unsigned char bestIndex;
    for (int i = 0; i < 256; i++) {
        int dr = palette[i].peRed - color.peRed;
        int dg = palette[i].peGreen - color.peGreen;
        int db = palette[i].peBlue - color.peBlue;
        int d = dr * dr + dg * dg + db * db;
        if (d < best) {
            bestIndex = i;
            best = d;
        }
    }
    return bestIndex;
}

// FUNCTION: 0x4ba920
void __stdcall SortByBrightness(unsigned char* data, int* sums, unsigned char* idx)
{
    int i;
    RGBA* p = (RGBA*)data;
    for (i = 0; i < 256; i++) {
        sums[i] = p[i].r + p[i].g + p[i].b;
        idx[i] = (unsigned char)i;
    }
    for (int k = 0; k < 256; k++) {
        for (int j = k + 1; j < 256; j++) {
            if (sums[k] > sums[j]) {
                int t = sums[k];
                sums[k] = sums[j];
                sums[j] = t;
                unsigned char tc = idx[k];
                idx[k] = idx[j];
                idx[j] = tc;
            }
        }
    }
}

// FUNCTION: 0x4ba9d0
unsigned char __stdcall NearestColorInBand(tagPALETTEENTRY* palette, int* band, unsigned char* order, tagPALETTEENTRY color)
{
    int best = 1000000000;
    unsigned char bestIndex;
    int sum = color.peRed + (color.peGreen + color.peBlue);
    int i;
    for (i = 0; i < 256; i++) {
        if (band[i] >= sum - 40) {
            if (band[i] > sum + 40)
                break;
            int k = order[i];
            int d = (palette[k].peRed - color.peRed) * (palette[k].peRed - color.peRed)
                  + (palette[k].peGreen - color.peGreen) * (palette[k].peGreen - color.peGreen)
                  + (palette[k].peBlue - color.peBlue) * (palette[k].peBlue - color.peBlue);
            if (d < best) {
                bestIndex = i;
                best = d;
            }
        }
    }
    if (best == 1000000000)
        bestIndex = i;
    return order[bestIndex];
}

// FUNCTION: 0x4baad0
void __stdcall SetAlphaTable(unsigned int* table)
{
    DisplayContext* obj = GetDisplay();
    if (obj->has_c0) {
        memcpy(obj->alphaTable, table, 0x4000 * 4);
    }
}

// FUNCTION: 0x4bab00
void __stdcall SetShadeTable(unsigned int* table)
{
    DisplayContext* obj = GetDisplay();
    if (obj->has_c4) {
        memcpy(obj->shadeTable, table, 0x800 * 4);
    }
}

// FUNCTION: 0x4bab30
void __stdcall SetLightTable(unsigned int* table)
{
    DisplayContext* obj = GetDisplay();
    if (obj->has_c8) {
        memcpy(obj->lightTable, table, 0x800 * 4);
    }
}

// FUNCTION: 0x4bab60
void __stdcall SetGrayTable(unsigned int* table)
{
    DisplayContext* obj = GetDisplay();
    if (obj->has_cc) {
        memcpy(obj->grayTable, table, 0x40 * 4);
    }
}

// FUNCTION: 0x4bab90
void __stdcall NopRetC(int, int, int)
{
}

// FUNCTION: 0x4baba0
void __stdcall SetBlueTable(unsigned int* table)
{
    DisplayContext* obj = GetDisplay();
    if (obj->has_d0) {
        memcpy(obj->blueTable, table, 0x40 * 4);
    }
}

// FUNCTION: 0x4babd0
unsigned char* __stdcall BuildLightTable(tagPALETTEENTRY* palette)
{
    DisplayContext* obj = GetDisplay();
    if (obj->has_c8) {
        int sums[256];
        unsigned char order[256];
        SortByBrightness((unsigned char*)palette, sums, order);
        for (int row = 0; row < 32; row++) {
            double factor = 1.0 - row * -0.03333333333333333;
            for (int i = 0; i < 256; i++) {
                int red = (int)(palette[i].peRed * factor);
                int green = (int)(palette[i].peGreen * factor);
                int blue = (int)(palette[i].peBlue * factor);
                tagPALETTEENTRY color;
                if (red > 255) {
                    red = 255;
                }
                if (green > 255) {
                    green = 255;
                }
                if (blue > 255) {
                    blue = 255;
                }
                color.peRed = red;
                color.peGreen = green;
                color.peBlue = blue;
                ((unsigned char*)obj->lightTable)[i + row * 256] =
                    NearestColorInBand(palette, sums, order, color);
            }
        }
        return (unsigned char*)obj->lightTable;
    }
    return 0;
}

// FUNCTION: 0x4bad30
unsigned char* __stdcall BuildGrayTable(tagPALETTEENTRY* palette)
{
    DisplayContext* app = GetDisplay();
    if (app->has_cc) {
        tagPALETTEENTRY color;
        unsigned char order[256];
        int sums[256];
        SortByBrightness((unsigned char*)palette, sums, order);
        for (int i = 0; i < 0x100; i++) {
            unsigned char gray = (unsigned char)((unsigned int)(palette[i].peRed +
                palette[i].peGreen + palette[i].peBlue) / 3);
            color.peGreen = gray;
            color.peBlue = gray;
            color.peRed = gray;
            ((unsigned char*)app->grayTable)[i] = NearestColorInBand(palette, sums, order, color);
        }
        return (unsigned char*)app->grayTable;
    }
    return 0;
}

// FUNCTION: 0x4badf0
unsigned char* __stdcall BuildShadeTable(tagPALETTEENTRY* palette)
{
    DisplayContext* app = GetDisplay();
    if (app->has_c4) {
        tagPALETTEENTRY color;
        unsigned char order[256];
        int sums[256];
        SortByBrightness((unsigned char*)palette, sums, order);
        double factor = 0.0;
        int offset = 0;
        do {
            for (int i = 0; i < 256; i++) {
                unsigned short v;
                v = (unsigned short)(palette[i].peRed * factor);
                color.peRed = v;
                if (v > 0xff)
                    color.peRed = 0xff;
                v = (unsigned short)(palette[i].peGreen * factor);
                color.peGreen = v;
                if (v > 0xff)
                    color.peGreen = 0xff;
                v = (unsigned short)(palette[i].peBlue * factor);
                color.peBlue = v;
                if (v > 0xff)
                    color.peBlue = 0xff;
                ((unsigned char*)app->shadeTable)[offset + i] = NearestColorInBand(palette, sums, order, color);
            }
            factor -= g_shadeStep;
            offset += 0x100;
        } while (offset < 0x2000);
        return (unsigned char*)app->shadeTable;
    }
    return 0;
}

// FUNCTION: 0x4baf30
unsigned char* __stdcall BuildBlueTable(tagPALETTEENTRY* palette)
{
    DisplayContext* app = GetDisplay();
    if (app->has_d0) {
        tagPALETTEENTRY color;
        unsigned char order[256];
        int sums[256];
        SortByBrightness((unsigned char*)palette, sums, order);
        for (int i = 0; i < 0x100; i++) {
            color.peRed = palette[i].peRed >> 1;
            color.peGreen = palette[i].peGreen >> 1;
            int b = palette[i].peBlue >> 1;
            if (b + 0x3c > 0xff)
                color.peBlue = 0xff;
            else
                color.peBlue = b + 0x32;
            ((unsigned char*)app->blueTable)[i] = NearestColorInBand(palette, sums, order, color);
        }
        return (unsigned char*)app->blueTable;
    }
    return 0;
}

// FUNCTION: 0x4baff0
char* __stdcall ChangeExtension(char* a, char* b, char* c)
{
    strcpy(b, a);
    char* p = b + strlen(b) - 1;
    while (p >= b) {
        if (*p == '\\')
            break;
        if (*p == '.') {
            *p = '\0';
            break;
        }
        p--;
    }
    strcat(b, ".");
    strcat(b, c);
    return b;
}

// FUNCTION: 0x4bb0a0
int __stdcall HasExtension(char* name)
{
    for (unsigned int i = 0; i < strlen(name); i++) {
        if (name[i] == '.')
            return 1;
    }
    return 0;
}

// FUNCTION: 0x4bb0f0
char* __stdcall StripExtension(char* name)
{
    for (int i = strlen(name); i >= 0; i--) {
        if (name[i] == '.') {
            name[i] = '\0';
            break;
        }
    }
    return name;
}

// FUNCTION: 0x4bb120
char* __stdcall StripFileName(char* path)
{
    for (int i = strlen(path); i >= 0; i--) {
        if (path[i] == '\\') {
            path[i + 1] = '\0';
            break;
        }
    }
    return path;
}

// FUNCTION: 0x4bb150
char* __stdcall StripPath(char* path)
{
    int len = strlen(path);
    int i = len - 1;
    while (i >= 0 && path[i] != '\\')
        i--;
    int j = i + 1;
    int k = 0;
    do {
        path[k] = path[j];
        k++;
    } while (path[j++] != 0);
    return path;
}
