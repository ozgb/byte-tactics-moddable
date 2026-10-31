#ifndef DEBRIS_H
#define DEBRIS_H

#include <stddef.h>
#include "../graphics/handle.h"
#include "../util/vec3.h"
#include "../graphics/object3do.h"
#include "../../interfaces/ref.h"

struct Face;

#pragma pack(push, 1)
struct Debris {
    int field_0;
    Handle ref1;
    Handle ref2;
    Vec3 pos;
    Vec3 size;
    Vec3 vel;
    Vec3 spin;
    short angle_x;
    short angle_y;
    short angle_z;
    unsigned short flag : 1;
    unsigned short : 15;
};

struct DebrisPiece {
    signed char state;
    char pad_1[3];
    int vertexCount;
    int faceCount;
    int firstFace;
    char pad_10[12];
    const char* name;
    int active;
    Ref<Vec3> verts;
    Ref<Face> faces;
    char pad_2c[8];
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Debris) == 84, "Debris");
static_assert(sizeof(DebrisPiece) == sizeof(Object3do), "DebrisPiece");
static_assert(offsetof(DebrisPiece, verts) == offsetof(Object3do, vertices), "DebrisPiece verts");
static_assert(offsetof(DebrisPiece, faces) == offsetof(Object3do, faces), "DebrisPiece faces");
#endif

#endif
