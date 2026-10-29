#include "win_types.h"
#include <string.h>
#include <math.h>

#include "rect.h"

// FUNCTION: 0x4b7e60
void __stdcall ClipRects(Rect* other, Rect* rect, Rect* bounds)
{
    int d;
    d = rect->left - bounds->left;
    if (d < 0) {
        other->left -= d;
        rect->left -= d;
    }
    d = rect->right - bounds->right;
    if (d > 0) {
        other->right -= d;
        rect->right -= d;
    }
    d = rect->top - bounds->top;
    if (d < 0) {
        other->top -= d;
        rect->top -= d;
    }
    d = rect->bottom - bounds->bottom;
    if (d > 0) {
        other->bottom -= d;
        rect->bottom -= d;
    }
}

struct GafFrameTableEntry {
    int value;
    int unknown_4;
};

struct GafFrameTable {
    unsigned short count;
    char unknown_2[0x26];
    GafFrameTableEntry entries[1];
};

struct GafFrameCursor {
    unsigned short index;
    char unknown_2[6];
    GafFrameTable* table;
};

// FUNCTION: 0x4b7ee0
int __stdcall GetGafSequenceFrame(GafFrameCursor* h)
{
    int result = 0;
    if (h->table != 0)
        result = h->table->entries[h->index].value;
    return result;
}

struct GafFrameCountCursor {
    unsigned short index;
    char unknown_2[6];
    unsigned short* table;
};
// FUNCTION: 0x4b7f00
int __stdcall SetGafSequenceFrame(GafFrameCountCursor* obj, int index)
{
    int result = 0;
    if (obj->table != 0 && *obj->table > index) {
        obj->index = index;
        result = 1;
    }
    return result;
}
// FUNCTION: 0x4b7f30
int __stdcall GetGafFrame(unsigned short* seq, int frame)
{
    int result = 0;
    if (frame >= 0 && frame < (int)*seq && seq != 0) {
        result = *(int*)((char*)seq + frame * 8 + 0x28);
    }
    return result;
}
// FUNCTION: 0x4b7f60
unsigned short __stdcall GetGafFrameCount(void* ptr)
{
    unsigned int result = 0;
    result = *(unsigned short*)ptr;
    return (unsigned short)result;
}

struct GafDurationEntry {
    unsigned short value;
    char unknown_2[6];
};

struct GafDurationCursor {
    unsigned short index;
    char unknown_2[6];
    void* table;
};

static inline GafDurationEntry* GetEntries(GafDurationCursor* obj)
{
    return (GafDurationEntry*)((char*)obj->table + 0x2c);
}
// FUNCTION: 0x4b7f70
unsigned short __stdcall GetGafFrameDuration(GafDurationCursor* param1)
{
    unsigned short result;
    if (param1->table != 0) {
        result = GetEntries(param1)[param1->index].value;
    } else {
        result = 0xffff;
    }
    return result;
}

#include "surface.h"

#include "gaf_frame.h"

int __stdcall LockScreen(Surface* out);
int __stdcall UnlockScreen(Surface* s);
void __cdecl BlitRectKeyed(Surface* dst, Surface* src, Rect* rect,
                           Rect* pos, int colour);
void __cdecl BlitRect(Surface* dst, Surface* src, Rect* rect,
                      Rect* pos);
void __cdecl BlitCompressed(unsigned char* pixels, int pitch, Rect* rect,
                            void* plane, Rect* other);
void __stdcall DrawFrameBlended(Surface* dst, GafFrame* bmp, int x, int y);

// FUNCTION: 0x4b7f90
void __stdcall DrawFrame(Surface* dst, GafFrame* bmp, int x, int y)
{
    Surface screen;
    if (dst == 0) {
        int ok = LockScreen(&screen);
        if (ok != 0)
            dst = (Surface*)&screen;
    }
    if (bmp != 0) {
        if (bmp->layers > 0) {
            for (int i = 0; i < (int)bmp->layers; i++) {
                GafFrame* e = ((GafFrame**)bmp->pixelsOrLayers)[i];
                if (e->blend > 0)
                    DrawFrameBlended(dst, e, x, y);
                else
                    DrawFrame(dst, e, x, y);
            }
        } else {
            Rect other;
            Rect rect;
            Rect bounds;
            Surface desc;
            other.left = 0;
            other.right = bmp->width - 1;
            other.top = 0;
            other.bottom = bmp->height - 1;
            rect.left = x - bmp->xOffset;
            rect.top = y - bmp->yOffset;
            rect.right = rect.left + bmp->width - 1;
            rect.bottom = rect.top + bmp->height - 1;
            dst->GetClipRect(&bounds);
            ClipRects(&other, &rect, &bounds);
            if (rect.right >= rect.left && rect.bottom >= rect.top &&
                other.right >= other.left && other.bottom >= other.top) {
                if (bmp->compressed == 0) {
                    desc.width = bmp->width;
                    desc.pitch = bmp->width;
                    desc.height = bmp->height;
                    desc.pixels = bmp->pixelsOrLayers;
                    BlitRectKeyed(dst, &desc, &other, &rect, bmp->transparency);
                } else {
                    BlitCompressed(dst->pixels, dst->pitch, &rect, bmp->pixelsOrLayers, &other);
                }
            }
        }
    }
    if (dst == (Surface*)&screen)
        UnlockScreen(&screen);
}

// FUNCTION: 0x4b8150
void __stdcall DrawFrameOpaque(Surface* dst, GafFrame* bmp, int x, int y)
{
    Surface screen;
    if (dst == 0) {
        int locked = LockScreen(&screen);
        if (locked != 0)
            dst = (Surface*)&screen;
    }
    if (bmp != 0) {
        if (bmp->layers > 0) {
            for (int i = 0; i < (int)bmp->layers; i++) {
                GafFrame* e = ((GafFrame**)bmp->pixelsOrLayers)[i];
                if (e->blend > 0)
                    DrawFrameBlended(dst, e, x, y);
                else
                    DrawFrame(dst, e, x, y);
            }
        } else {
            Rect other;
            Rect rect;
            Rect bounds;
            Surface desc;
            other.left = 0;
            other.right = bmp->width - 1;
            other.top = 0;
            other.bottom = bmp->height - 1;
            rect.left = x - bmp->xOffset;
            rect.top = y - bmp->yOffset;
            rect.right = rect.left + bmp->width - 1;
            rect.bottom = rect.top + bmp->height - 1;
            dst->GetClipRect(&bounds);
            ClipRects(&other, &rect, &bounds);
            if (rect.right >= rect.left && rect.bottom >= rect.top &&
                other.right >= other.left && other.bottom >= other.top) {
                if (bmp->compressed == 0) {
                    desc.width = bmp->width;
                    desc.pitch = bmp->width;
                    desc.height = bmp->height;
                    desc.pixels = bmp->pixelsOrLayers;
                    BlitRect(dst, &desc, &other, &rect);
                } else {
                    BlitCompressed(dst->pixels, dst->pitch, &rect, bmp->pixelsOrLayers, &other);
                }
            }
        }
    }
    if (dst == (Surface*)&screen)
        UnlockScreen(&screen);
}

struct ScreenSurfaceView {
    char unknown_0[0x20];
    Surface surf;
};

struct BlitSourceView {
    int width;
    int height;
    int pitch;
    GafFrame** pixels;
};

struct Bounds_src_004b8310 {
    Rect bounds;
    BlitSourceView src;
};

#include "display.h"
void __cdecl BlitRectBlended(Surface* p, BlitSourceView* src, Rect* rect,
                             Rect* pos, int colour, unsigned char* palette);
void __cdecl BlitCompressedLit(unsigned char* pixels, int pitch, Rect* drect,
                               GafFrame** src, Rect* srect,
                               unsigned char* palette);
void __cdecl BlitCompressedBlended(unsigned char* pixels, int pitch, Rect* drect,
                                   GafFrame** src, Rect* srect,
                                   unsigned char* palette);

// FUNCTION: 0x4b8500
void __stdcall DrawFrameBlended(Surface* surface, GafFrame* frame, int x, int y)
{
    Display* d = GetDisplay();
    if ((d->flags.byte & 0x20) != 0) {
        ScreenSurfaceView screen;
        if (surface == 0) {
            int locked = LockScreen(&screen.surf);
            if (locked != 0)
                surface = (Surface*)&screen.surf;
        }

        if (frame != 0) {
            if (frame->layers > 0) {
                for (int i = 0; i < frame->layers; i++)
                    DrawFrameBlended(surface, ((GafFrame**)frame->pixelsOrLayers)[i], x, y);
            } else {
                Rect screen_rect;
                Rect sprite_rect = { 0, 0, frame->width - 1, frame->height - 1 };
                int w = frame->width;
                int h = frame->height;
                screen_rect.left = x - frame->xOffset;
                screen_rect.top = y - frame->yOffset;
                screen_rect.right = w + screen_rect.left - 1;
                screen_rect.bottom = h + screen_rect.top - 1;
                Bounds_src_004b8310 bs;
                surface->GetClipRect(&bs.bounds);
                ClipRects(&sprite_rect, &screen_rect, &bs.bounds);
                if (screen_rect.right >= screen_rect.left && screen_rect.bottom >= screen_rect.top
                    && sprite_rect.right >= sprite_rect.left && sprite_rect.bottom >= sprite_rect.top) {
                    if (frame->compressed == 0) {
                        BlitSourceView& src = bs.src;
                        src.width = frame->width;
                        src.height = frame->height;
                        src.pitch = frame->width;
                        src.pixels = (GafFrame**)frame->pixelsOrLayers;
                        BlitRectBlended(surface, &src, &sprite_rect, &screen_rect,
                            frame->transparency, d->alphaTable);
                    } else {
                        BlitCompressedBlended(surface->pixels, surface->pitch, &screen_rect,
                            (GafFrame**)frame->pixelsOrLayers, &sprite_rect, d->alphaTable);
                    }
                }
            }
        }

        if (surface == (Surface*)&screen.surf)
            UnlockScreen(&screen.surf);
    }
}

void __cdecl BlitRectRemapDest(Surface* dst, Surface* src, Rect* rect,
                               Rect* pos, int colour, unsigned char* table);
void __stdcall ResetClipRect(int* surface);

// FUNCTION: 0x4b86e0
void __stdcall DrawFrameGray(Surface* surface, GafFrame* frame, int x, int y)
{
    Display* d = GetDisplay();
    if ((d->flags.word & 0x100) != 0 && frame->compressed == 0) {
        Surface screen;
        if (surface == 0) {
            int locked = LockScreen(&screen);
            if (locked != 0)
                surface = (Surface*)&screen;
        }
        if (frame != 0) {
            if (frame->layers > 0) {
                for (int i = 0; i < frame->layers; i++)
                    DrawFrameGray(surface, ((GafFrame**)frame->pixelsOrLayers)[i], x, y);
            } else {
                Rect other;
                Rect rect;
                other.left = 0;
                other.right = frame->width - 1;
                other.top = 0;
                other.bottom = frame->height - 1;
                rect.left = x - frame->xOffset;
                rect.top = y - frame->yOffset;
                rect.right = rect.left + frame->width - 1;
                rect.bottom = rect.top + frame->height - 1;
                Rect bounds;
                surface->GetClipRect(&bounds);
                ClipRects(&other, &rect, &bounds);
                if (rect.right >= rect.left && rect.bottom >= rect.top &&
                    other.right >= other.left && other.bottom >= other.top) {
                    Surface s;
                    s.width = frame->width;
                    s.height = frame->height;
                    s.pitch = frame->width;
                    s.pixels = (unsigned char*)frame->pixelsOrLayers;
                    s.zPriority = 10000;
                    s.colorKey = -1;
                    s.x = frame->xOffset;
                    s.y = frame->yOffset;
                    s.flag0 = 1;
                    s.flag1 = 0;
                    ResetClipRect((int*)&s);
                    BlitRectRemapDest(surface, &s, &other, &rect, frame->transparency, d->grayTable);
                }
            }
        }
        if (surface == (Surface*)&screen)
            UnlockScreen(&screen);
    }
}

// FUNCTION: 0x4b88d0
void __stdcall EraseFrameDithered(Surface* dst, GafFrame* bmp, int x, int y, int parity)
{
    Surface screen;
    if (bmp->compressed == 0) {
        if (dst == 0) {
            int ok = LockScreen(&screen);
            if (ok != 0)
                dst = (Surface*)&screen;
        }
        if (bmp != 0) {
            if (bmp->layers > 0) {
                for (int i = 0; i < (int)bmp->layers; i++) {
                    GafFrame* e = ((GafFrame**)bmp->pixelsOrLayers)[i];
                    EraseFrameDithered(dst, e, x, y, parity);
                }
            } else {
                Rect rect;
                Rect other;
                Rect bounds;
                other.left = 0;
                other.top = 0;
                other.right = bmp->width - 1;
                other.bottom = bmp->height - 1;
                rect.left = x - bmp->xOffset;
                rect.top = y - bmp->yOffset;
                rect.right = rect.left + bmp->width - 1;
                rect.bottom = rect.top + bmp->height - 1;
                dst->GetClipRect(&bounds);
                ClipRects(&other, &rect, &bounds);
                if (rect.right >= rect.left && rect.bottom >= rect.top &&
                    other.right >= other.left && other.bottom >= other.top) {
                    for (int yy = rect.top, sy = other.top; yy <= rect.bottom; yy++, sy++) {
                        unsigned char* s = (unsigned char*)bmp->pixelsOrLayers
                            + sy * bmp->width + other.left;
                        unsigned char* d = dst->pixels
                            + yy * dst->pitch + rect.left;
                        unsigned char* end = d + rect.right - rect.left;
                        if (((yy + rect.left + parity) & 1) != 0) {
                            d++;
                            s++;
                        }
                        while (d <= end) {
                            if (*s != bmp->transparency)
                                *d = 0;
                            d += 2;
                            s += 2;
                        }
                    }
                }
            }
        }
        if (dst == (Surface*)&screen)
            UnlockScreen(&screen);
    }
}

// FUNCTION: 0x4b8a80
void __stdcall SurfaceFromFrame(Surface* dst, GafFrame* src)
{
    dst->width = src->width;
    dst->height = src->height;
    dst->pitch = src->width;
    dst->pixels = src->pixelsOrLayers;
    dst->zPriority = 10000;
    dst->colorKey = -1;
    dst->x = src->xOffset;
    dst->y = src->yOffset;
    dst->flag1 = 0;
    dst->flag0 = 1;
    ResetClipRect((int*)dst);
}

#include "../files/files_api.h"

// FUNCTION: 0x4b8ae0
void __stdcall FrameFromSurface(GafFrame* dst, Surface* src)
{
    dst->width = src->width;
    dst->height = src->height;
    dst->width = src->pitch;
    dst->pixelsOrLayers = src->pixels;
    dst->xOffset = src->x;
    dst->yOffset = src->y;
    dst->compressed = 0;
    dst->transparency = 0xff;
    dst->layers = 0;
    dst->blend = 0;
}

struct GafSequenceRecord {
    unsigned short count;
    unsigned char kind;
    char unknown_3[5];
    char name[1];
    char unknown_9[31];
    GafFrame* child[1];
    GafDurationEntry entries[1];
};

struct GafSequencePlayer {
    unsigned short index;
    unsigned short value;
    unsigned char kind;
    char unknown_5[3];
    GafSequenceRecord* src;
};

static inline unsigned short GafDurationAt(GafSequenceRecord* s, unsigned short i)
{
    if (s)
        return s->entries[i].value;
    return 0xffff;
}
// FUNCTION: 0x4b8b30
void __stdcall InitGafSequence(GafSequencePlayer* ref, GafSequenceRecord* src, int index)
{
    ref->index = index < src->count ? index : 0;
    ref->src = src;
    ref->value = GafDurationAt(src, ref->index);
    ref->kind = src->kind;
}

// FUNCTION: 0x4b8b90
int __stdcall StepGafSequence(GafSequencePlayer* ref)
{
    GafSequenceRecord* src = ref->src;
    if (src) {
        if (ref->value < 2) {
            ref->index++;
            if (ref->index >= src->count) {
                if (ref->kind) {
                    ref->index = 0;
                } else {
                    ref->src = 0;
                    return 1;
                }
            }
            ref->value = src->entries[ref->index].value;
            return 1;
        }
        ref->value--;
    }
    return 0;
}

struct GafSequencePlayerSigned {
    unsigned short index;
    short value;
    unsigned char kind;
    char unknown_5[3];
    GafSequenceRecord* src;
};

static inline unsigned short GafCurrentDuration(GafSequencePlayerSigned* r)
{
    if (r->src)
        return r->src->entries[r->index].value;
    return 0xffff;
}

// FUNCTION: 0x4b8bf0
void __stdcall AdvanceGafSequence(GafSequencePlayerSigned* ref, short step)
{
    GafSequenceRecord* src = ref->src;
    if (src == 0 || src->count <= 1) {
        return;
    }
    ref->value -= step;
    while (ref->value <= 0) {
        ref->index++;
        if (ref->index >= src->count) {
            if (!ref->kind) {
                ref->src = 0;
                return;
            }
            ref->index = 0;
        }
        ref->value += GafCurrentDuration(ref);
    }
}

struct GafBlockEntry {
    int off;
    int pad;
};
struct Blk {
    unsigned short count;
    unsigned char pad1[8];
    unsigned char subcount;
    unsigned char pad2[29];
    GafBlockEntry e[2];
};
// FUNCTION: 0x4b8c60
void* __stdcall LoadGaf(char* name)
{
    int* base = (int*)HAPI_LoadFile(name, 0);
    if (base == 0)
        return 0;

    for (int i = 0; i < (short)base[1]; i++) {
        Blk* p = (Blk*)((char*)base + base[3 + i]);
        base[3 + i] = (int)p;
        int j = 0;
        if (p->count > 0) {
            do {
                int* q = (int*)((char*)base + p->e[j].off);
                p->e[j].off = (int)q;
                q[4] = q[4] + (int)base;
                if (((unsigned char*)q)[0xa] > 0) {
                    for (int k = 0; k < (int)((unsigned char*)q)[0xa]; k++) {
                        ((int*)q[4])[k] = ((int*)q[4])[k] + (int)base;
                        int* s = (int*)((int*)q[4])[k];
                        s[4] = s[4] + (int)base;
                    }
                }
                j++;
            } while (j < (int)p->count);
        }
    }
    return base;
}

struct GafFileIndex {
    char unknown_0[4];
    short count;
    char unknown_6[6];
    GafSequenceRecord* entries[1];
};

// FUNCTION: 0x4b8d40
GafSequenceRecord* __stdcall FindGafEntry(GafFileIndex* gaf, const char* name)
{
    if (gaf) {
        GafSequenceRecord** p = gaf->entries;
        for (int i = 0; i < gaf->count; i++, p++) {
            if (_strcmpi((*p)->name, name) == 0)
                return *p;
        }
    }
    return 0;
}

#include "../debug/debug_lib.h"
// FUNCTION: 0x4b8da0
GafFrame* __stdcall AllocFrame(const char* name, int width, int height)
{
    GafFrame* p = (GafFrame*)GameAllocIgnoreTag(name, height * width + 0x18);
    if (p == 0) {
        return 0;
    }
    p->width = width;
    p->height = height;
    p->pixelsOrLayers = (unsigned char*)(p + 1);
    p->scratch = 0;
    p->xOffset = 0;
    p->yOffset = 0;
    p->compressed = 0;
    p->layers = 0;
    p->blend = 0;
    return p;
}
// FUNCTION: 0x4b8e00 ?AllocDepthFrame@@YGPAUGafFrame@@IHH@Z
GafFrame* __stdcall AllocDepthFrame(unsigned int heap, int width, int height)
{
    int size = height * width;
    GafFrame* b = (GafFrame*)GameAllocIgnoreTag((const char*)heap, size * 2 + sizeof(GafFrame));
    unsigned char* p = (unsigned char*)(b + 1);
    b->pixelsOrLayers = p;
    b->height = height;
    p += size;
    b->scratch = p;
    b->width = width;
    b->xOffset = 0;
    b->yOffset = 0;
    b->compressed = 0;
    b->layers = 0;
    b->blend = 0;
    return b;
}
// FUNCTION: 0x4b8e50
void __stdcall ClearFrame(GafFrame* b, int color)
{
    if (b->compressed == 0) {
        memset(b->pixelsOrLayers, color, b->width * b->height);
        if (b->scratch != 0) {
            memset(b->scratch, 0, b->width * b->height);
        }
    }
}

void __cdecl BlitRectShadow(Surface* dst, Surface* src, Rect* rect,
                            Rect* pos, int colour, unsigned char* table);
void __cdecl BlitCompressedShadow(unsigned char* pixels, int pitch, Rect* rect,
                                  void* plane, Rect* other, unsigned char* table);

// FUNCTION: 0x4b8ec0
void __stdcall DrawFrameShadow(Surface* dst, GafFrame* bmp, int x, int y)
{
    Display* d = GetDisplay();
    if (d->flags.bits.has_c8 == 1) {
        Surface screen;
        if (dst == 0) {
            int ok = LockScreen(&screen);
            if (ok != 0)
                dst = (Surface*)&screen;
        }
        if (bmp != 0) {
            if (bmp->layers > 0) {
                for (int i = 0; i < (int)bmp->layers; i++) {
                    GafFrame* e = ((GafFrame**)bmp->pixelsOrLayers)[i];
                    DrawFrameShadow(dst, e, x, y);
                }
            } else if (bmp->pixelsOrLayers != 0) {
                Rect other;
                Rect rect;
                Rect bounds;
                Surface desc;
                other.left = 0;
                other.right = bmp->width - 1;
                other.top = 0;
                other.bottom = bmp->height - 1;
                rect.left = x - bmp->xOffset;
                rect.top = y - bmp->yOffset;
                rect.right = rect.left + bmp->width - 1;
                rect.bottom = rect.top + bmp->height - 1;
                dst->GetClipRect(&bounds);
                ClipRects(&other, &rect, &bounds);
                if (rect.right >= rect.left && rect.bottom >= rect.top &&
                    other.right >= other.left && other.bottom >= other.top) {
                    if (bmp->compressed == 0) {
                        desc.width = bmp->width;
                        desc.pitch = bmp->width;
                        desc.height = bmp->height;
                        desc.pixels = bmp->pixelsOrLayers;
                        BlitRectShadow(dst, &desc, &other, &rect, bmp->transparency, d->lightTable);
                    } else {
                        BlitCompressedShadow(dst->pixels, dst->pitch, &rect, bmp->pixelsOrLayers, &other, d->lightTable);
                    }
                }
            }
        }
        if (dst == (Surface*)&screen)
            UnlockScreen(&screen);
    }
}

GafFrame* __stdcall AllocDepthFrame(const char* name, int width, int height)
{
    int size = height * width;
    GafFrame* b = (GafFrame*)GameAllocIgnoreTag(name, size * 2 + sizeof(GafFrame));
    unsigned char* p = (unsigned char*)(b + 1);
    b->width = width;
    b->pixelsOrLayers = p;
    b->height = height;
    p += size;
    b->scratch = p;
    b->xOffset = 0;
    b->yOffset = 0;
    b->compressed = 0;
    b->layers = 0;
    b->blend = 0;
    return b;
}

// FUNCTION: 0x4b91b0
void* __stdcall BuildLensFrame(int w, int h, int lens)
{
    double scale = lens;
    GafFrame* f = AllocDepthFrame("LensFrame", w * 2, h);
    if (!f)
        return 0;
    f->width /= 2;
    unsigned short* p = (unsigned short*)f->pixelsOrLayers;
    short hw = w / 2;
    f->xOffset = hw;
    short hh = h / 2;
    f->yOffset = hh;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int dx = x - hw;
            int dy = y - hh;
            double dist = sqrt((double)(dx * dx + dy * dy));
            if ((int)dist >= w / 4) {
                p[y * w + x] = 0x7d00;
            } else {
                int i = y * w + x;
                int v = (int)(dx / ((w / 2 - dist) / scale))
                        + (w * ((int)(dy / ((w / 2 - dist) / scale)) + hh) + hw);
                p[i] = v - i;
            }
        }
    }
    return f;
}

static void SwapPtr(unsigned char*& a, unsigned char*& b)
{
    unsigned char* t = a;
    a = b;
    b = t;
}

// FUNCTION: 0x4b9360
void __stdcall DrawLens(void* dst, GafFrame* sprite, int x, int y)
{
    SwapPtr(sprite->pixelsOrLayers, sprite->scratch);

    Surface surface;
    surface.width = sprite->width;
    surface.pitch = sprite->width;
    surface.height = sprite->height;
    surface.pixels = sprite->pixelsOrLayers;
    surface.zPriority = 10000;
    surface.colorKey = -1;
    surface.x = sprite->xOffset;
    surface.y = sprite->yOffset;
    surface.flag0 = 1;
    surface.flag1 = 0;
    ResetClipRect((int*)&surface);

    Rect srcRect;
    srcRect.left = 0;
    srcRect.right = surface.width - 1;
    srcRect.top = 0;
    srcRect.bottom = surface.height - 1;

    Rect dstRect;
    dstRect.left = x - sprite->xOffset;
    dstRect.right = x + surface.width - sprite->xOffset - 1;
    dstRect.top = y - sprite->yOffset;
    dstRect.bottom = y + surface.height - sprite->yOffset - 1;

    CopySurfaceRect(&surface, (Surface *)dst, &dstRect, (Point *)&srcRect);

    SwapPtr(sprite->pixelsOrLayers, sprite->scratch);

    int n = sprite->width * sprite->height;
    unsigned char* cmap = sprite->scratch;
    unsigned short* idx = (unsigned short*)sprite->pixelsOrLayers;
    unsigned char* out = cmap + n;
    int i = n;
    while (i != 0) {
        short c = *idx;
        unsigned char v;
        if (c != 32000)
            v = cmap[c];
        else
            v = sprite->transparency;
        *out = v;
        out++;
        cmap++;
        idx++;
        i--;
    }

    int m = sprite->width * sprite->height;
    int saved = (int)sprite->pixelsOrLayers;
    sprite->pixelsOrLayers = sprite->scratch + m;
    DrawFrame((Surface*)dst, sprite, x, y);
    sprite->pixelsOrLayers = (unsigned char*)saved;
}
// FUNCTION: 0x4b94c0
void __stdcall GrabBackground(void* dst, GafFrame* sprite, int x, int y)
{
    Surface surface;
    surface.width = sprite->width;
    surface.height = sprite->height;
    surface.pitch = sprite->width;
    surface.pixels = sprite->pixelsOrLayers;
    surface.zPriority = 10000;
    surface.colorKey = -1;
    surface.x = sprite->xOffset;
    surface.y = sprite->yOffset;
    surface.flag0 = 1;
    surface.flag1 = 0;
    ResetClipRect((int*)&surface);

    Rect srcRect;
    srcRect.left = 0;
    srcRect.right = surface.width - 1;
    srcRect.top = 0;
    srcRect.bottom = surface.height - 1;

    Rect dstRect;
    dstRect.left = x - sprite->xOffset;
    dstRect.right = surface.width + x - sprite->xOffset - 1;
    dstRect.top = y - sprite->yOffset;
    dstRect.bottom = surface.height + y - sprite->yOffset - 1;

    CopySurfaceRect(&surface, (Surface *)dst, &dstRect, (Point *)&srcRect);
}

// FUNCTION: 0x4b96a0
void __stdcall ZeroFramePixels(GafFrame* image)
{
    int count = image->height * image->width;
    unsigned char* p = image->pixelsOrLayers;
    while (count--) {
        if (*p != image->transparency) {
            *p = 0;
        }
        p++;
    }
}

// FUNCTION: 0x4b96e0
void __stdcall TintFrameBelow(GafFrame* image, unsigned char level)
{
    unsigned char* p = image->pixelsOrLayers;
    unsigned char* m = image->scratch;
    int count = image->height * image->width;
    Display* pal = GetDisplay();
    while (count--) {
        if (*m <= level && *p != image->transparency) {
            *p = pal->blueTable[*p];
        }
        p++;
        m++;
    }
}

// FUNCTION: 0x4b9740
void __stdcall DrawFrameScaledBlended(Surface* dst, GafFrame* bmp, int x, int y, double sx, double sy)
{
    Display* d = GetDisplay();
    Surface screen;
    if (dst == 0) {
        int ok = LockScreen(&screen);
        if (ok != 0)
            dst = (Surface*)&screen;
    }
    if (bmp->layers > 0) {
        for (int i = 0; i < (int)bmp->layers; i++)
            DrawFrameScaledBlended(dst, ((GafFrame**)bmp->pixelsOrLayers)[i], x, y, sx, sy);
    } else {
        Rect src;
        Rect dest;
        Rect bounds;
        int w = (int)(bmp->width * sx);
        int h = (int)(bmp->height * sy);
        int dx = (int)(bmp->xOffset * sx);
        int dy = (int)(bmp->yOffset * sy);
        if (w > 0 && h > 0) {
            dest.left = x - dx;
            dest.top = y - dy;
            dest.right = w + dest.left - 1;
            dest.bottom = h + dest.top - 1;
            src.left = 0;
            src.top = 0;
            src.right = w - 1;
            src.bottom = h - 1;
            int stepX = (bmp->width << 16) / w;
            int stepY = (bmp->height << 16) / h;
            dst->GetClipRect(&bounds);
            ClipRects(&src, &dest, &bounds);
            if (dest.right >= dest.left && dest.bottom >= dest.top && src.right >= src.left &&
                src.bottom >= src.top) {
                src.left = (int)(src.left / sx);
                src.top = (int)(src.top / sy);
                src.right = (int)(src.right / sx);
                src.bottom = (int)(src.bottom / sy);
                int fy, row;
                for (row = dest.top, fy = src.top << 16; row <= dest.bottom; row++, fy += stepY) {
                    int srcRow = (fy >> 16) * bmp->width;
                    int col = dest.left;
                    int rowBase = row * dst->pitch;
                    for (int fx = src.left << 16; col <= dest.right; col++, fx += stepX) {
                        unsigned char c = ((unsigned char*)bmp->pixelsOrLayers)[(fx >> 16) + srcRow];
                        if (c != bmp->transparency) {
                            unsigned char* p = rowBase + col + dst->pixels;
                            *p = d->alphaTable[(c << 8) + *p];
                        }
                    }
                }
            }
        }
    }
    if (dst == (Surface*)&screen)
        UnlockScreen(&screen);
}

// FUNCTION: 0x4b9a50
void __stdcall DrawFrameScaled(Surface* dst, GafFrame* bmp, int x, int y, double sx, double sy)
{
    Surface screen;
    if (dst == 0) {
        int ok = LockScreen(&screen);
        if (ok != 0)
            dst = (Surface*)&screen;
    }
    if (bmp->layers > 0) {
        for (int i = 0; i < (int)bmp->layers; i++) {
            GafFrame* e = ((GafFrame**)bmp->pixelsOrLayers)[i];
            if (e->blend > 0)
                DrawFrameScaledBlended(dst, e, x, y, sx, sy);
            else
                DrawFrameScaled(dst, e, x, y, sx, sy);
        }
    } else {
        Rect src;
        Rect dest;
        Rect bounds;
        int w = (int)(bmp->width * sx);
        int h = (int)(bmp->height * sy);
        int dx = (int)(bmp->xOffset * sx);
        int dy = (int)(bmp->yOffset * sy);
        if (w > 0 && h > 0) {
            dest.left = x - dx;
            dest.top = y - dy;
            dest.right = w + dest.left - 1;
            dest.bottom = h + dest.top - 1;
            src.left = 0;
            src.top = 0;
            src.right = w - 1;
            src.bottom = h - 1;
            int stepX = (bmp->width << 16) / w;
            int stepY = (bmp->height << 16) / h;
            dst->GetClipRect(&bounds);
            ClipRects(&src, &dest, &bounds);
            if (dest.right >= dest.left && dest.bottom >= dest.top && src.right >= src.left &&
                src.bottom >= src.top) {
                src.left = (int)(src.left / sx);
                src.top = (int)(src.top / sy);
                src.right = (int)(src.right / sx);
                src.bottom = (int)(src.bottom / sy);
                for (x = dest.top, y = src.top << 16; x <= dest.bottom; x++, y += stepY) {
                    int col = dest.left;
                    int srcRow = (y >> 16) * bmp->width;
                    int rowBase = x * dst->pitch;
                    for (int fx = src.left << 16; col <= dest.right; col++, fx += stepX) {
                        unsigned char c = ((unsigned char*)bmp->pixelsOrLayers)[(fx >> 16) + srcRow];
                        if (c != bmp->transparency) {
                            dst->pixels[rowBase + col] = c;
                        }
                    }
                }
            }
        }
    }
    if (dst == (Surface*)&screen)
        UnlockScreen(&screen);
}

// FUNCTION: 0x4b9d70
void __stdcall CutOutFrame(GafFrame* src, GafFrame* dst, int x, int y)
{
    int srcCol, dstCol, srcRow, dstRow;
    x += src->xOffset - dst->xOffset;
    y = (dst->yOffset - src->yOffset) - y;
    if (y < 0) {
        srcRow = -y;
        dstRow = 0;
    } else {
        srcRow = 0;
        dstRow = y;
    }

    dstCol = -x;
    if (dstCol < 0) { srcCol = -dstCol; dstCol = 0; } else { srcCol = 0; }

    int n;
    if (src->width - srcCol > dst->width)
        n = dst->width;
    else
        n = src->width - srcCol;

    for (; srcRow < src->height; srcRow++, dstRow++) {
        if (dstRow >= dst->height)
            break;
        unsigned char* s = src->pixelsOrLayers + srcRow * src->width + srcCol;
        unsigned char* d = dst->pixelsOrLayers + dstRow * dst->width + dstCol;
        int i = n;
        while (i != 0) {
            if (*s != src->transparency)
                *d = dst->transparency;
            s++;
            d++;
            i--;
        }
    }
}

#include "gaf.h"

// FUNCTION: 0x4b9e60
int __stdcall CompressFrame(unsigned char* dest, GafFrame* img)
{
    int total = 0;
    unsigned char* src = img->pixelsOrLayers;
    int width = img->width;
    int height = img->height;
    while (height-- > 0) {
        unsigned short* len;
        if (dest) {
            len = (unsigned short*)dest;
            dest += 2;
        }
        int n = CompressRow((char*)dest, (char*)src, width, img->transparency);
        src += width;
        total += n + 2;
        if (dest) {
            dest += n;
            *len = n;
        }
    }
    return total;
}

// GLOBAL: 0x51fcb0
extern unsigned char g_gafEncodeLiteralBuffer[];
// GLOBAL: 0x51fdb0
extern int g_gafEncodeOutputSize;
// FUNCTION: 0x4b9ed0
char* __stdcall EmitCopyRun(char* out, int count)
{
    int index = 0;
    do {
        int n = count;
        if (n > 0x40) {
            n = 0x40;
        }
        count -= n;
        unsigned char header = (unsigned char)(n + 0x3f);
        header <<= 2;
        if (out != 0) {
            *out++ = header;
        }
        g_gafEncodeOutputSize++;
        for (int i = 0; i < n; i++) {
            int value = g_gafEncodeLiteralBuffer[index++];
            for (unsigned int shift = 0; shift < 8; shift += 8) {
                if (out != 0) {
                    *out++ = (char)(value >> shift);
                }
                g_gafEncodeOutputSize++;
            }
        }
    } while (count > 0);
    return out;
}
// FUNCTION: 0x4b9f50
char* __stdcall EmitRepeatRun(char* out, int count, unsigned char a, unsigned char b)
{
    if (a == b) {
        do {
            int n = count;
            if (n > 0x7f) {
                n = 0x7f;
            }
            count -= n;
            unsigned char header = (unsigned char)n;
            header <<= 1;
            header |= 1;
            if (out != 0) {
                *out++ = header;
            }
            g_gafEncodeOutputSize++;
        } while (count > 0);
    } else {
        int value = a;
        do {
            int n = count;
            if (n > 0x40) {
                n = 0x40;
            }
            count -= n;
            unsigned char header = (unsigned char)(n + 0x3f);
            header <<= 2;
            header |= 2;
            if (out != 0) {
                *out++ = header;
            }
            g_gafEncodeOutputSize++;
            for (unsigned int shift = 0; shift < 8; shift += 8) {
                if (out != 0) {
                    *out++ = (char)(value >> shift);
                }
                g_gafEncodeOutputSize++;
            }
        } while (count > 0);
    }
    return out;
}

// FUNCTION: 0x4ba000
int __stdcall CompressRow(char* dest, char* src, int width, unsigned char key)
{
    int runStart = 0;
    int skip;
    for (skip = 0; skip < width; skip++) {
        if (key != (unsigned char)src[skip]) {
            break;
        }
    }
    if (skip >= width) {
        return 0;
    }

    g_gafEncodeOutputSize = runStart;
    char* out = dest;
    char* p = src;
    unsigned char c = *p;
    p++;
    width--;
    unsigned char value = c;
    unsigned char prev = c;
    g_gafEncodeLiteralBuffer[0] = c;
    int n = 1;
    int state = (c == key);
    while (width) {
        width--;
        g_gafEncodeLiteralBuffer[n++] = c = *p++;
        value = c;
        switch (state) {
        case 0:
            if (c == key) {
                n--;
                out = EmitCopyRun(out, n);
                n = 1;
                g_gafEncodeLiteralBuffer[0] = c;
                runStart = 0;
                state = n;
                break;
            }
            if (n > 0x80) {
                n--;
                out = EmitCopyRun(out, n);
                g_gafEncodeLiteralBuffer[0] = c;
                n = 1;
                runStart = 0;
                break;
            }
            if (c == prev) {
                if (n - runStart >= 3) {
                    if (runStart > 0) {
                        out = EmitCopyRun(out, runStart);
                    }
                    state = 1;
                } else if (runStart == 0) {
                    state = 1;
                }
            } else {
                runStart = n - 1;
                break;
            }
        case 1:
            if (c == prev && n - runStart <= 0x80) {
                break;
            }
            out = EmitRepeatRun(out, n - runStart - 1, prev, key);
            runStart = 0;
            g_gafEncodeLiteralBuffer[0] = c;
            n = 1;
            state = (c == key);
            break;
        }
        prev = c;
    }
    switch (state) {
    case 0:
        EmitCopyRun(out, n);
        break;
    case 1:
        EmitRepeatRun(out, n - runStart, value, key);
        return g_gafEncodeOutputSize;
    }
    return g_gafEncodeOutputSize;
}

// FUNCTION: 0x4ba1b0
void __stdcall CutFrameBelow(GafFrame* image, unsigned char level)
{
    unsigned char* p = image->pixelsOrLayers;
    unsigned char* m = image->scratch;
    int count = image->height * image->width;
    Display* pal = GetDisplay();
    while (count--) {
        if (*m <= level) {
            *p = image->transparency;
        }
        p++;
        m++;
    }
}

// FUNCTION: 0x4b8310
void __stdcall DrawFrameLit(Surface* surface, GafFrame* frame, int x, int y, int paletteRow)
{
    Display* d = GetDisplay();
    if ((d->flags.byte & 0x80) == 0x80) {
        ScreenSurfaceView screen;
        if (surface == 0) {
            int locked = LockScreen(&screen.surf);
            if (locked != 0)
                surface = (Surface*)&screen.surf;
        }

        if (frame != 0) {
            if (frame->layers > 0) {
                for (int i = 0; i < frame->layers; i++)
                    DrawFrameBlended(surface, ((GafFrame**)frame->pixelsOrLayers)[i], x, y);
            } else {
                Rect screen_rect;
                Rect sprite_rect = { 0, 0, frame->width - 1, frame->height - 1 };
                int w = frame->width;
                int h = frame->height;
                screen_rect.left = x - frame->xOffset;
                screen_rect.top = y - frame->yOffset;
                screen_rect.right = w + screen_rect.left - 1;
                screen_rect.bottom = h + screen_rect.top - 1;
                Bounds_src_004b8310 bs;
                surface->GetClipRect(&bs.bounds);
                ClipRects(&sprite_rect, &screen_rect, &bs.bounds);
                if (screen_rect.right >= screen_rect.left && screen_rect.bottom >= screen_rect.top
                    && sprite_rect.right >= sprite_rect.left && sprite_rect.bottom >= sprite_rect.top) {
                    if (frame->compressed == 0) {
                        BlitSourceView& src = bs.src;
                        src.width = frame->width;
                        src.height = frame->height;
                        src.pitch = frame->width;
                        src.pixels = (GafFrame**)frame->pixelsOrLayers;
                        BlitRectBlended(surface, &src, &sprite_rect, &screen_rect, paletteRow, d->lightTable);
                    } else {
                        BlitCompressedLit(surface->pixels, surface->pitch, &screen_rect,
                            (GafFrame**)frame->pixelsOrLayers, &sprite_rect, d->lightTable + (paletteRow << 8));
                    }
                }
            }
        }

        if (surface == (Surface*)&screen.surf)
            UnlockScreen(&screen.surf);
    }
}

// FUNCTION: 0x4b90a0
void __stdcall DrawFrameDepth(GafFrame* src, GafFrame* dst, int x, int y, int level)
{
    int yoff;
    int xoff;
    xoff = dst->xOffset - src->xOffset + x;
    yoff = dst->yOffset - src->yOffset + y;
    if (xoff < 0 || yoff < 0) {
        return;
    }
    unsigned char* sp0 = src->pixelsOrLayers;
    unsigned char* sp1 = src->scratch;
    for (int row = 0; row < src->height; row++) {
        unsigned char* dp0 = xoff + dst->pixelsOrLayers + dst->width * (yoff + row);
        unsigned char* dp1 = xoff + dst->scratch + dst->width * (yoff + row);
        int n = src->width;
        while (n--) {
            unsigned char c = *sp0;
            if (c != src->transparency && *dp1 <= *sp1 + level) {
                *dp0 = c;
                *dp1 = *sp1 + level;
            }
            dp0++;
            sp0++;
            dp1++;
            sp1++;
        }
    }
}

// FUNCTION: 0x4b95a0
void __stdcall DownsampleFrame(GafFrame* src, GafFrame* dst)
{
    Display* pal = GetDisplay();
    int row = 0;
    for (; row < dst->height; row++) {
        for (int x = 0; x < dst->width; x++) {
            unsigned char* p = src->pixelsOrLayers + (row * src->width + x) * 2;
            unsigned char* table = pal->alphaTable;
            unsigned char* q = src->pixelsOrLayers + (row * 2 + 1) * src->width + x * 2;
            int a = p[0];
            int b = p[1];
            int p1 = table[(a << 8) + b];
            int c = q[0];
            int d = q[1];
            int p2 = table[(c << 8) + d];
            dst->pixelsOrLayers[row * dst->width + x] = table[(p1 << 8) + p2];
        }
    }
}
