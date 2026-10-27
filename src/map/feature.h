#ifndef FEATURE_H
#define FEATURE_H

#include "../../interfaces/ref.h"
#include "../graphics/handle.h"
#include "../util/vec3.h"

struct Gaf;

#pragma pack(push, 1)

struct AnimSrc {
    unsigned short count;
    unsigned char kind;
};

struct AnimPair {
    AnimSrc* anim;
    AnimSrc* shadow;
};

struct Feature {
    char name[0x80];
    char description[0x14];
    union {
        Point16 footprint;
        struct {
            short footprintX;
            short footprintZ;
        };
    };
    union {
        void* object;
        char filename[0x10];
    };
    Gaf* anims;
    union {
        unsigned short* animTable;
        AnimSrc* seq;
    };
    union {
        unsigned short* shadowTable;
        AnimSrc* seqshad;
    };
    union {
        struct {
            AnimSrc* seqburn;
            AnimSrc* seqburnshad;
            AnimSrc* seqdie;
            AnimSrc* seqdieshad;
            AnimSrc* seqreclamate;
            AnimSrc* seqreclamateshad;
        };
        struct {
            AnimPair burn;
            AnimPair death[2];
        };
        struct {
            int anim0;
            int field_b8;
            int anim1;
            int field_c0;
            int anim2;
            int field_c8;
        };
    };
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
        unsigned short flags16;
        struct {
            unsigned char flags;
            unsigned char flagsHigh;
        };
        struct {
            unsigned char lowBits : 6;
            unsigned char steep : 1;
        };
        struct {
            unsigned short drawn : 1;
            unsigned short over : 1;
            unsigned short flipAnim : 1;
            unsigned short flipShadow : 1;
            unsigned short unknown_bits : 4;
        };
        struct {
            unsigned short noobject : 1;
            unsigned short animating : 1;
            unsigned short animtrans : 1;
            unsigned short shadtrans : 1;
            unsigned short flamable : 1;
            unsigned short geothermal : 1;
            unsigned short blocking : 1;
            unsigned short reclaimable : 1;
            unsigned short autoreclaimable : 1;
            unsigned short indestructible : 1;
            unsigned short nodisplayinfo : 1;
            unsigned short nodrawundergray : 1;
            unsigned short bits12 : 4;
        };
    };
};

static_assert(sizeof(Feature) == 0x100, "");
static_assert(__builtin_offsetof(Feature, footprint) == 0x94, "");
static_assert(__builtin_offsetof(Feature, object) == 0x98, "");
static_assert(__builtin_offsetof(Feature, anims) == 0xa8, "");
static_assert(__builtin_offsetof(Feature, animTable) == 0xac, "");
static_assert(__builtin_offsetof(Feature, seqburn) == 0xb4, "");
static_assert(__builtin_offsetof(Feature, seqreclamateshad) == 0xc8, "");
static_assert(__builtin_offsetof(Feature, anim) == 0xcc, "");
static_assert(__builtin_offsetof(Feature, shadowAnim) == 0xd8, "");
static_assert(__builtin_offsetof(Feature, burnweapon) == 0xe4, "");
static_assert(__builtin_offsetof(Feature, energy) == 0xec, "");
static_assert(__builtin_offsetof(Feature, metal) == 0xf0, "");
static_assert(__builtin_offsetof(Feature, flags16) == 0xfe, "");

#pragma pack(pop)

#endif
