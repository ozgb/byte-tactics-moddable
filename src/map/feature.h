#ifndef FEATURE_H
#define FEATURE_H

#include "../../interfaces/ref.h"
#include "../graphics/handle.h"

#pragma pack(push, 1)

struct Feature {
    char name[0x80];
    char description[0x14];
    short footprintX;
    short footprintZ;
    char unknown_98[0xac - 0x98];
    unsigned short* animTable;
    unsigned short* shadowTable;
    char unknown_b4[0xcc - 0xb4];
    Handle anim;
    Handle shadowAnim;
    Ref<char> burnweapon;
    unsigned short burnTime;
    unsigned short damage;
    float energy;
    float metal;
    unsigned short dead;
    unsigned short burnt;
    unsigned short reclamate;
    unsigned char height;
    unsigned char spreadChance;
    unsigned char seedChance;
    unsigned char seedSpread;
    union {
        struct {
            unsigned short drawn : 1;
            unsigned short over : 1;
            unsigned short flipAnim : 1;
            unsigned short flipShadow : 1;
            unsigned short unknown_bits : 4;
        };
        struct {
            unsigned char flags;
            unsigned char flagsHigh;
        };
    };
};

#pragma pack(pop)

#endif
