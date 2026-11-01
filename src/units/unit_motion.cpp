#include "win_types.h"
#include <string.h>
#include "../state/block.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"

#include "../graphics/object_pool.h"
#include "../debug/debug_lib.h"

#include "cob_script.h"
#include "../util/vec3.h"

#include "../map/cell.h"
#include "unit_motion.h"

#pragma pack(push, 1)
struct UnitDefMetalWindView {
    char unknown_0[0x1ce];
    float extractsMetal;
    float windGenerator;
};

#include "unit.h"

#include "../game/game.h"
#pragma pack(pop)

union SumFixed {
    int value;
    struct {
        unsigned short fraction;
        short whole;
    } parts;
};

#include "../graphics/gaf_frame.h"

#include "../graphics/model_types.h"
#include "../graphics/memory_cache.h"

#include "../graphics/gaf.h"

#include "../../interfaces/seams.h"

#include "../../link/globals.h"

// FUNCTION: 0x437840
void __stdcall UpdateMetalExtraction(Unit* unit)
{
    if (((UnitDefMetalWindView*)unit->def.get())->extractsMetal > 0.0f) {
        SumFixed total;
        total.value = 0;
        Point16 fp = *(Point16*)&unit->footprint;
        for (int y = unit->cell.y; y < unit->cell.y + fp.y; y++) {
            for (int x = unit->cell.x; x < unit->cell.x + fp.x; x++) {
                Cell* c = g_seams.los_radar->GetMapCell(x, y);
                if (c) {
                    total.parts.whole += c->metal + 1;
                }
            }
        }
        *(float*)&unit->extraction = ((UnitDefMetalWindView*)unit->def.get())->extractsMetal * 1.52587890625e-05 * (float)total.value;
        if (unit->script)
            g_seams.units->StartScriptWithArgs((CobScript*)unit->script.get(), "SetSpeed", 0, 0, 1, total.parts.whole, 0, 0, 0);
    }
}

// FUNCTION: 0x437910
void __stdcall UpdateWindGenerator(Unit* unit)
{
    if (((UnitDefMetalWindView*)unit->def.get())->windGenerator > 0.0f && g_game->windEnabled) {
        ((CobScript*)unit->script.get())->StartScriptWithArgs("SetDirection", 0, 0, 1, g_game->windDirection, 0, 0, 0);
        ((CobScript*)unit->script.get())->StartScriptWithArgs("SetSpeed", 0, 0, 1, g_game->windSpeed << 4, 0, 0, 0);
    }
}

// FUNCTION: 0x4379a0
template <class Cache, class Handle>
Cache* CacheCore<Cache, Handle>::ClearPointers()
{
    Cache* self = static_cast<Cache*>(this);
    CacheSet(self->base, 0);
    CacheSet(self->handle, 0);
    return self;
}

template <class Cache, class Handle>
int CacheCore<Cache, Handle>::Install(void* block, unsigned int newSize)
{
    Cache* self = static_cast<Cache*>(this);
    typename Cache::Chunk* chunk = (typename Cache::Chunk*)block;
    CacheSet(self->base, (int)chunk);
    self->cap = newSize;
    self->cur = chunk;
    chunk->owner = 0;
    self->cur->size = self->cap;
    return 1;
}

// FUNCTION: 0x4379b0
int CMemoryCache::InitCache(unsigned int newSize)
{
    FreeCache();
    return Install(GameAllocIgnoreTag("CMemoryCache CCH", newSize), newSize);
}

int DebrisCache::InitSimCache(unsigned int newSize)
{
    FreeCache();
    return Install(SimAllocTagged("CMemoryCache CCH", newSize), newSize);
}

// FUNCTION: 0x437a00
template <class Cache, class Handle>
void CacheCore<Cache, Handle>::FreeCache()
{
    Cache* self = static_cast<Cache*>(this);
    if (CacheAddr(self->base) != 0) {
        GameFreeThunk((int*)CacheAddr(self->base));
        CacheSet(self->base, 0);
    }
}

// FUNCTION: 0x437a20
template <class Cache, class Handle>
void CacheCore<Cache, Handle>::FreeBuffer()
{
    GameFreeThunk((int*)CacheAddr(static_cast<Cache*>(this)->base));
}

// FUNCTION: 0x437a30
template <class Cache, class Handle>
int CacheCore<Cache, Handle>::AllocHandle(Handle* p, int need)
{
    Cache* self = static_cast<Cache*>(this);
    typedef typename Cache::Chunk Chunk;
    int total = 0;
    if (need <= 0) {
        CacheSet(*p, 0);
        return 0;
    }
    int size = need + 8;
    int existing = CacheAddr(*p);
    if (existing != 0)
        existing = *(int*)(existing - 4);
    if (existing >= size)
        return 1;
    if (size > self->cap) {
        CacheSet(*p, 0);
        return 0;
    }
    Chunk* start;
    int base = CacheAddr(self->base);
    int end = base + self->cap;
    if (CacheAddr(self->cur) + size > end) {
        int q = CacheAddr(self->cur);
        start = (Chunk*)base;
        while (q < end) {
            Handle* owner = ((Chunk*)q)->owner;
            if (owner != 0 && CacheAddr(*owner) == q + 8)
                CacheSet(*owner, 0);
            q += *(int*)(q + 4);
            end = base + self->cap;
        }
    } else {
        start = self->cur;
    }
    int q = (int)start;
    while (total < size) {
        total += *(int*)(q + 4);
        Handle* owner = ((Chunk*)q)->owner;
        if (owner != 0 && CacheAddr(*owner) == q + 8)
            CacheSet(*owner, 0);
        q += *(int*)(q + 4);
    }
    if (total - size > 8u) {
        Chunk* nc = (Chunk*)((char*)start + size);
        nc->owner = 0;
        nc->size = total - size;
        self->cur = nc;
    } else {
        size = total;
        int e = (int)start + total;
        if (e >= base + self->cap)
            e = base;
        CacheSet(self->cur, e);
    }
    start->owner = p;
    start->size = size;
    CacheSet(*p, (int)((char*)start + 8));
    return 1;
}

template class CacheCore<CMemoryCache, void*>;
template class CacheCore<DebrisCache, Ref<char> >;

#pragma auto_inline(off)
// FUNCTION: 0x437b50
int CMemoryCache::AllocBitmap(GafFrame** handle, int w, int h)
{
    int n = w * h;
    if (!AllocHandle((void**)handle, n + 0x18))
        return 0;
    GafFrame* b = *handle;
    if (!b)
        return 0;
    b->scratch = 0;
    b->xOffset = 0;
    b->yOffset = 0;
    b->layers = 0;
    b->blend = 0;
    b->reserved = 0;
    b->compressed = 0;
    b->width = w;
    b->height = h;
    b->pixelsOrLayers = (unsigned char*)(b + 1);
    b->transparency = 1;
    memset(b->pixelsOrLayers, 1, n);
    return 1;
}
#pragma auto_inline(on)

// FUNCTION: 0x437be0
int CMemoryCache::AllocTwoPlaneBitmap(GafFrame** handle, int w, int h)
{
    int n = w * h;
    if (!AllocHandle((void**)handle, n * 2 + 0x18))
        return 0;
    GafFrame* b = *handle;
    if (!b)
        return 0;
    unsigned char* p = (unsigned char*)(b + 1);
    b->xOffset = 0;
    b->yOffset = 0;
    b->layers = 0;
    b->blend = 0;
    b->reserved = 0;
    b->compressed = 0;
    b->width = w;
    b->height = h;
    b->pixelsOrLayers = p;
    b->scratch = p + n;
    memset(b->scratch, 0, n);
    b->transparency = 1;
    memset(b->pixelsOrLayers, 1, n);
    return 1;
}

// FUNCTION: 0x437c80
void CMemoryCache::FlushCache()
{
    AllocHandle(&handle, cap - 8);
}

// FUNCTION: 0x437c90
void CMemoryCache::ReleaseHandle(int id)
{
    CacheChunk* r = (CacheChunk*)base;
    for (int offset = 0; offset < cap; ) {
        offset += r->size;
        if ((int)r->owner == id) {
            r->owner = 0;
        }
        r = (CacheChunk*)((char*)r + r->size);
    }
}

// FUNCTION: 0x437b30
int __stdcall GetHandleSize(int param_1)
{
    if (param_1 == 0) {
        return 0;
    }
    return *(int*)(param_1 - 4);
}

// FUNCTION: 0x458810
void CMemoryCache::DrawObjectState(ModelState* list, void* result)
{
    int rebuild = 0;
    int x = g_game->scrollX << 16;
    int z = g_game->scrollY << 16;
    int visible;
    UnitDrawView* owner = list->OwnerView();
    if (owner->flags & 0x20000000) {
        int t = (unsigned char)~owner->activateFlags;
        if (t & 1)
            visible = 1;
        else
            visible = 0;
    } else {
        visible = *(int*)((char*)owner->motion + 0x20) == 0;
    }
    if (list->cacheDrawCount == 0)
        rebuild = 1;
    if (list->bitmap == 0)
        list->shadow = 0;
    GafFrame* bitmap = list->bitmap;
    if ((owner->flags & 0x20000000) != 0) {
        if (list->bitmap == 0)
            rebuild = 1;
        else if (owner->buildLeft != 0.0f
                && (owner->flags & 0x2000) != 0
                && list->bitmap->scratch == 0
                && bitmap->scratch == 0)
            rebuild = 1;
    }
    if (list->bitmap == 0 && (owner->flags & 0x20000000) != 0)
        rebuild = 1;
    if ((owner->zBufferFlag & 1) != 0 && list->bitmap == 0)
        rebuild = 1;
    if (rebuild) {
        list->shadow = 0;
        ((UnitTable*)this)->BuildObjectPicture(list, 0, 1);
    }
    Vec3 coords;
    coords.x = x;
    coords.z = z;
    DrawObjectPieces((int)result, list, coords, visible);
    list->cacheDrawCount++;
}

// FUNCTION: 0x459170
void CMemoryCache::CopyPicture(GafFrame* source)
{
    image->width = source->width;
    image->height = source->height;
    image->xOffset = source->xOffset;
    image->yOffset = source->yOffset;
    image->transparency = source->transparency;
    memcpy(image->pixelsOrLayers, source->pixelsOrLayers, source->width * source->height);
    if (source->scratch)
        memcpy(image->scratch, source->scratch, source->width * source->height);
}

// FUNCTION: 0x45a790
void CMemoryCache::BuildShadow(ModelState* obj, GafFrame* dest)
{
    int w, h, x, y;
    MeasureShadow(&w, &h, &x, &y, obj);
    image->width = (unsigned short)w;
    image->height = (unsigned short)h;
    image->xOffset = (unsigned short)x;
    image->yOffset = (unsigned short)y;
    memset(image->pixelsOrLayers, image->transparency, h * w);
    memset(image->scratch, 0, h * w);
    DrawShadowShape(image, obj);
    CutOutFrame(dest, image, 5, 0);
    int size = CompressFrame(image->scratch, image);
    GafFrame** slot = &obj->shadow;
    AllocBitmap(slot, size, 1);
    GafFrame* bmp = *slot;
    memcpy(bmp->pixelsOrLayers, image->scratch, size);
    bmp->width = image->width;
    bmp->height = image->height;
    bmp->xOffset = image->xOffset;
    bmp->yOffset = image->yOffset;
    bmp->compressed = 1;
}
