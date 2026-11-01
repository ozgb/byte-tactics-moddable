#ifndef MEMORY_CACHE_H
#define MEMORY_CACHE_H

#include "../../interfaces/ref.h"

struct GafFrame;
struct CacheChunk {
    void** owner;
    int size;
};
struct DebrisCacheChunk {
    Ref<Ref<char> > owner;
    int size;
};
struct Vec3;
union FixedVec3Union;
struct ModelState;
struct Object3do;
struct ModelBoundsOrigin;

inline int CacheAddr(int v) { return v; }
template <class T>
inline int CacheAddr(T* p) { return (int)p; }
template <class T>
inline int CacheAddr(const Ref<T>& r) { return (int)r.get(); }
inline void CacheSet(int& d, int a) { d = a; }
template <class T>
inline void CacheSet(T*& d, int a) { d = (T*)a; }
template <class T>
inline void CacheSet(Ref<T>& d, int a) { d = (T*)a; }

template <class Cache, class Handle>
class CacheCore {
public:
    Cache* ClearPointers();
    void FreeCache();
    void FreeBuffer();
    int AllocHandle(Handle* handle, int size);

protected:
    int Install(void* chunk, unsigned int size);
};

class CMemoryCache : public CacheCore<CMemoryCache, void*> {
public:
    typedef CacheChunk Chunk;
    int cap;
    union {
        int base;
        int* basePointer;
    };
    CacheChunk* cur;
    void* handle;
    union {
        GafFrame* image;
        GafFrame* bitmap;
        GafFrame* shadow;
        void* buffer;
        void* ptr;
        int field_10;
    };

    int InitCache(unsigned int size);
    int AllocBitmap(GafFrame** handle, int w, int h);
    int AllocTwoPlaneBitmap(GafFrame** handle, int w, int h);
    void FlushCache();
    void ReleaseHandle(int handle);
    void DrawObjectState(ModelState* list, void* context);
    void CopyPicture(GafFrame* source);
    void BuildShadow(ModelState* model, GafFrame* dest);
    void DrawPiece(ModelState* model, void* surface, Vec3* camera,
        Object3do* info, Vec3* vertices, unsigned char palette, int useColor);
    void DrawObjectPicture(int param_2, ModelState* model, FixedVec3Union v, int useColor);
    void DrawObjectPieces(int param_1, ModelState* list, Vec3 v, int param_6);
    void MergeIntoComposite(GafFrame* src, ModelState* model);
    void AddModelBounds(int* minX, int* maxX, int* minY, int* maxY,
        ModelState* model, ModelBoundsOrigin pos);
    void RecolorByShade(GafFrame* img, unsigned char level, int above, int below, int between);
    int ShadeByIntensity(GafFrame* image, ModelState* model);
    void DrawPieceEdges(GafFrame* view, ModelState* model, int color);
    GafFrame* MakeSilhouette(GafFrame* src);
    void MeasureShadow(int* width, int* height, int* originX, int* originY, ModelState* model);
    void DrawShadowShape(GafFrame* view, ModelState* model);
};

class DebrisCache : public CacheCore<DebrisCache, Ref<char> > {
public:
    typedef DebrisCacheChunk Chunk;
    int cap;
    Ref<DebrisCacheChunk> base;
    Ref<DebrisCacheChunk> cur;
    Ref<char> handle;
    int field_10;

    int InitSimCache(unsigned int size);
    template <class T>
    int AllocHandle(Ref<T>* slot, int size) { return CacheCore<DebrisCache, Ref<char> >::AllocHandle((Ref<char>*)slot, size); }
};

class UnitTable : public CMemoryCache {
public:
    UnitTable* Construct(void);
    int Initialize(unsigned int size);
    void Destroy();
    void MeasureModel(int* width, int* height, int* originX, int* originY, ModelState* model, Vec3* offset);
    int BuildObjectPicture(ModelState* list, int param_2, int param_3);
    void DrawPieces(GafFrame* bitmap, ModelState* list, int kind, int useColor);
    void DrawLitPieces(GafFrame* bitmap, ModelState* list, int kind, int useColor);
};

static_assert(sizeof(UnitTable) == 20, "the unit table is allocated as 0x14 bytes");

#endif
