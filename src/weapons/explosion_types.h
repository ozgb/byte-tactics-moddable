#ifndef EXPLOSION_TYPES_H
#define EXPLOSION_TYPES_H

#include "../util/vec3.h"
#include "../../interfaces/ref.h"

struct Unit;

#pragma pack(push, 1)

struct GafSequence {
    unsigned short count;
    unsigned char kind;
    char unknown_3[0x2c - 3];
    struct { unsigned short value; char unknown_2[6]; } entries[1];
};

struct GafRef {
    union {
        void* pic;
        int tex;
        struct {
            unsigned short index;
            unsigned short value;
        };
    };
    unsigned char kind;
    char unknown_5[3];
    GafSequence* src;
};

struct FaceFlags {
    union {
        unsigned int raw;
        struct {
            unsigned int a : 1;
            unsigned int b : 1;
            unsigned int c : 1;
            unsigned int rest : 29;
        } bits;
    };
};

struct Face {
    int color;
    int nverts;
    int unknown_8;
    unsigned short* vindex;
    GafRef tex;
    FaceFlags flags;
};

struct Object3D {
    signed char state;
    char unknown_1[3];
    int vertexCount;
    int faceCount;
    int firstFace;
    char unknown_10[12];
    const char* name;
    int active;
    Vec3* verts;
    Face* faces;
    char unknown_2c[8];
};

struct PieceRec {
    Object3D* desc;
    char unknown_4[0xc];
    short angle_10;
    short angle_12;
    short angle_14;
    Vec3 pos;
    Ref<Vec3> verts;
    char unknown_26[2];
    unsigned short flags;
    Ref<PieceRec> sibling;
    Ref<PieceRec> child;
    Ref<PieceRec> parent;
};

struct ExplodedPiece {
    Ref<Unit> unit;
    int index;
    int angVelZ;
    int angVelX;
    int angVelY;
    int vel_x;
    int vel_y;
    int vel_z;
    int ticks;
    int scale;
    unsigned int b0 : 1;
    unsigned int b1 : 1;
    unsigned int b2 : 1;
    unsigned int b3 : 1;
    unsigned int b4 : 1;
    unsigned int b5 : 1;
    unsigned int rest : 26;
    Ref<PieceRec> rec;
};

#pragma pack(pop)

#endif
