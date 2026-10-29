#ifndef MODEL_TYPES_H
#define MODEL_TYPES_H

#include "../util/vec3.h"
#include "../../interfaces/ref.h"
#include "gaf_frame.h"
#include "object3do.h"

struct Vector3s {
    short x;
    short y;
    short z;
};

struct ModelFaceFlags {
    union {
        unsigned int raw;
        struct {
            unsigned int textured : 1;
            unsigned int usePic : 1;
            unsigned int shaded : 1;
            unsigned int rest : 29;
        } bits;
    };
};

struct ModelFace {
    int unknown_0;
    int count;
    int unknown_8;
    unsigned short* indices;
    void* pic;
    int unknown_14;
    unsigned short* color;
    ModelFaceFlags flags;
};

#pragma pack(push, 2)
union ModelPieceFlags {
    unsigned short word;
    struct {
        unsigned short visible : 1;
        unsigned short colored : 1;
        unsigned short lit : 1;
        unsigned short rest : 13;
    } bits;
};

struct ModelPiece {
    Object3do* object;
    int rot_x;
    int rot_y;
    int rot_z;
    short short_10;
    short short_12;
    short short_14;
    Vec3 offset;
    Ref<Vec3> points;
    short modified;
    ModelPieceFlags flags;
    ModelPiece* sibling;
    ModelPiece* child;
    ModelPiece* parent;
};
#pragma pack(pop)

struct UnitDef;
struct ModelState;

#pragma pack(push, 1)
struct UnitDrawView {
    Ref<char> motion;
    char unknown_4[0x60];
    Vector3s angles;
    int pos_x;
    int pos_y;
    int pos_z;
    char unknown_76[0x86 - 0x76];
    int carrier;
    Ref<UnitDrawView> cargo;
    Ref<UnitDrawView> cargoNext;
    Ref<UnitDef> def;
    char unknown_96[8];
    Ref<ModelState> state;
    char unknown_a2[4];
    short unitDefIndex;
    unsigned short id;
    char unknown_aa[0xff - 0xaa];
    unsigned char playerIndex;
    char unknown_100[4];
    float buildLeft;
    char unknown_108[6];
    unsigned char activateFlags;
    char cobStateFlags;
    unsigned int flags;
    unsigned char zBufferFlag;
};
#pragma pack(pop)

#pragma pack(push, 2)
struct ModelState {
    int count;
    int cacheDrawCount;
    int animDirty;
    UnitDrawView* owner;
    GafFrame* bitmap;
    GafFrame* shadow;
    Vector3s pos;
    ModelPiece* root;
    ModelPiece pieces[1];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct FixedVec3Parts {
    Fixed x;
    Fixed y;
    Fixed z;
};

union FixedVec3Union {
    int v[3];
    FixedVec3Parts p;
    Vec3 i;
};
#pragma pack(pop)

#endif
