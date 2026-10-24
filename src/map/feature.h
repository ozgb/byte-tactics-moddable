#ifndef FEATURE_H
#define FEATURE_H

#include "../../interfaces/ref.h"

#pragma pack(push, 1)

struct Feature {
    char name[0x80];
    char description[0x14];
    short footprintX;
    short footprintZ;
    char unknown_98[0x4c];
    Ref<char> burnweapon;
    unsigned short burnTime;
    unsigned short damage;
    float metal;
    float value;
    unsigned short dead;
    unsigned short burnt;
    unsigned short reclamate;
    unsigned char height;
    unsigned char spreadChance;
    unsigned char seedChance;
    unsigned char seedSpread;
    unsigned char flags;
    char unknown_ff;
};

#pragma pack(pop)

#endif
