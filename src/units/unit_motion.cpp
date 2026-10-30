#include "win_types.h"
#include <string.h>
#include "../state/block.h"

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
CMemoryCache* CMemoryCache::ClearPointers()
{
    int zero = 0;
    base = zero;
    handle = (void*)zero;
    return this;
}

// FUNCTION: 0x4379b0
int CMemoryCache::InitCache(unsigned int newSize)
{
    if (base != 0) {
        GameFreeThunk((int*)base);
        base = 0;
    }
    CacheChunk* chunk = (CacheChunk*)GameAllocIgnoreTag("CMemoryCache CCH", newSize);
    base = (int)chunk;
    cap = newSize;
    cur = chunk;
    chunk->owner = 0;
    cur->size = cap;
    return 1;
}

int CMemoryCache::InitSimCache(unsigned int newSize)
{
    if (base != 0) {
        GameFreeThunk((int*)base);
        base = 0;
    }
    CacheChunk* chunk = (CacheChunk*)SimAllocTagged("CMemoryCache CCH", newSize);
    base = (int)chunk;
    cap = newSize;
    cur = chunk;
    chunk->owner = 0;
    cur->size = cap;
    return 1;
}

// FUNCTION: 0x437a00
void CMemoryCache::FreeCache()
{
    if (base != 0) {
        GameFreeThunk((int*)base);
        base = 0;
    }
}

// FUNCTION: 0x437a20
void CMemoryCache::FreeBuffer()
{
    GameFreeThunk((int*)base);
}

static int HandleGet(void** slot, bool ref)
{
    return ref ? (int)((Ref<char>*)slot)->get() : (int)*slot;
}

static void HandleSet(void** slot, int address, bool ref)
{
    if (ref)
        *(Ref<char>*)slot = (char*)address;
    else
        *slot = (void*)address;
}

// FUNCTION: 0x437a30
int CMemoryCache::AllocHandle(void** p, int need, bool ref)
{
    int total = 0;
    if (need <= 0) {
        HandleSet(p, 0, ref);
        return 0;
    }
    int size = need + 8;
    int existing = HandleGet(p, ref);
    if (existing != 0)
        existing = *(int*)(existing - 4);
    if (existing >= size)
        return 1;
    if (size > cap) {
        HandleSet(p, 0, ref);
        return 0;
    }
    CacheChunk* start;
    int end = base + cap;
    if ((int)cur + size > end) {
        int q = (int)cur;
        start = (CacheChunk*)base;
        while (q < end) {
            void** owner = *(void***)q;
            if (owner != 0 && HandleGet(owner, ref) == q + 8)
                HandleSet(owner, 0, ref);
            q += *(int*)(q + 4);
            end = base + cap;
        }
    } else {
        start = cur;
    }
    int q = (int)start;
    while (total < size) {
        total += *(int*)(q + 4);
        void** owner = *(void***)q;
        if (owner != 0 && HandleGet(owner, ref) == q + 8)
            HandleSet(owner, 0, ref);
        q += *(int*)(q + 4);
    }
    if (total - size > 8u) {
        CacheChunk* nc = (CacheChunk*)((char*)start + size);
        nc->owner = 0;
        nc->size = total - size;
        cur = nc;
    } else {
        size = total;
        int e = (int)start + total;
        if (e >= base + cap)
            e = base;
        cur = (CacheChunk*)e;
    }
    start->owner = p;
    start->size = size;
    HandleSet(p, (int)((char*)start + 8), ref);
    return 1;
}

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
    UnitDrawView* owner = list->owner;
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
