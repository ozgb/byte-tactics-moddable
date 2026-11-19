#ifndef UNIT_DEF_MOVE_VIEW_H
#define UNIT_DEF_MOVE_VIEW_H

#include "../../interfaces/ref.h"

#pragma pack(push, 1)
class MovementClass;
struct UnitDefMoveView {
    char unknown_0[0x20];
    char name[0x20];
    char unknown_40[0x192 - 0x40];
    int maxvelocity;
    char unknown_196[0x19a - 0x196];
    int brakerate;
    int acceleration;
    int bankscale;
    int pitchscale;
    char unknown_1aa[0x1ae - 0x1aa];
    int moverate1;
    int moverate2;
    Ref<MovementClass> movementclass;
    unsigned short max_turn;
    char unknown_1bc[0x202 - 0x1bc];
    short range;
    char unknown_204[0x214 - 0x204];
    unsigned short maneuverleashlength;
    char unknown_216[0x22c - 0x216];
    unsigned char draft;
    char unknown_22d[0x230 - 0x22d];
    unsigned char defaultmissiontype;
    char unknown_231[0x241 - 0x231];
    union {
        int flags1;
        unsigned char flags;
        struct {
            unsigned int mode_bits : 11;
            unsigned int flag_800 : 1;
        };
        struct {
            unsigned int low : 19;
            unsigned int b19 : 1;
            unsigned int rest : 12;
        };
    };
    char unknown_245[0x249 - 0x245];
};
#pragma pack(pop)

#endif
