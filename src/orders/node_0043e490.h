#ifndef NODE_0043E490_H
#define NODE_0043E490_H

#include "../../interfaces/ref.h"

struct Limits_0043e490;
struct UnitDef;
struct Player;

#pragma pack(push, 1)
struct Node_0043e490 {
    char unknown_0[0x111];
    union {
        unsigned int f111;
        struct {
            unsigned int b0_7 : 8;
            unsigned int b8 : 1;
            unsigned int b9_31 : 23;
        } f111b;
    };
};

struct Feature_0043e490 {
    char unknown_0[0xfe];
    unsigned char flags;
    char unknown_ff;
};

struct Size_0043e490 {
    unsigned int width;
    unsigned int height;
};

struct Stats_0043e490 {
    char unknown_0[0x8c];
    float f8c;
    char unknown_90[0x98 - 0x90];
    float f98;
};

struct Unit_0043e490 {
    int motion;
    char unknown_4[0x10 - 0x4];
    Ref<Node_0043e490> f10;
    char unknown_14[0x48 - 0x14];
    Ref<Limits_0043e490> f48;
    char unknown_4c[0x6a - 0x4c];
    char f6a[0x86 - 0x6a];
    Ref<Unit_0043e490> carrier;
    char unknown_8a[0x92 - 0x8a];
    Ref<UnitDef> def;
    Ref<Player> player;
    char unknown_9a[0xec - 0x9a];
    Ref<Stats_0043e490> fec;
    char unknown_f0[0xfb - 0xf0];
    int postTransferHoldoff;
    unsigned char playerIndex;
    char unknown_100[0x104 - 0x100];
    float buildLeft;
    char unknown_108[0x110 - 0x108];
    unsigned int flags;
};

struct Pos_0043e490 {
    short xf, x;
    short yf, y;
    short zf, z;
};
#pragma pack(pop)

struct Limits_0043e490 {
    char unknown_0[0xc0];
    float fc0;
    float fc4;
};

#endif
