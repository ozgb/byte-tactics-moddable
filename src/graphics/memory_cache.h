#ifndef MEMORY_CACHE_H
#define MEMORY_CACHE_H

struct GafFrame;
struct CacheChunk;
struct Vec3;
union FixedVec3Union;
struct ModelState;
struct Object3do;
struct ModelBoundsOrigin;

class CMemoryCache {
public:
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

    CMemoryCache* ClearPointers();
    int InitCache(unsigned int size);
    int InitSimCache(unsigned int size);
    void FreeCache();
    void FreeBuffer();
    int AllocHandle(void** handle, int size, bool ref = false);
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

#endif
