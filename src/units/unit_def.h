#ifndef UNIT_DEF_H
#define UNIT_DEF_H
#include "../../interfaces/ref.h"
#include "../util/vec3.h"

#pragma pack(push, 1)

struct WeaponDef;
class MovementClass;

class UnitDef {
public:
    char name[0x20];
    char unitname[0x20];
    char description[0x40];
    char objectname[0x20];
    char side[0x1e];
    char aiWeight[0x40];
    char aiLimit[0x40];
    unsigned int fbiChecksum;
    int scriptChecksum;
    int weaponHashXor;
    union {
        int f14a;
        Point16 origin;
        struct {
            short footprintx;
            short footprintz;
        };
    };
    union {
        int f14e;
        Ref<char> yardmap;
    };
    int count;
    union {
        Ref<unsigned short> ids;
        Ref<short> idsSigned;
    };
    int maxThisUnit;
    int modelMinX;
    int modelMinY;
    int modelMinZ;
    int modelMaxX;
    union {
        int modelMaxY;
        struct {
            short unknown_16e;
            short field_170;
        };
        struct {
            short modelMaxYLow;
            short modelMaxYHigh;
        };
    };
    int modelMaxZ;
    union {
        int modelSizeX;
        struct {
            short modelSizeXLow;
            short modelSizeXHigh;
        };
    };
    int modelSizeY;
    union {
        int modelSizeZ;
        struct {
            short unknown_17e;
            short height180;
        };
    };
    union {
        int radius;
        struct {
            short unknown_182;
            short radius184;
        };
    };
    float energyCost;
    float metalCost;
    void* data;
    int maxvelocity;
    int maxslopevelocity;
    int brakerate;
    int acceleration;
    int bankscale;
    int pitchscale;
    int damagemodifier;
    int moverate1;
    int moverate2;
    Ref<MovementClass> movementclass;
    union {
        short turnrate;
        unsigned short turnrateUnsigned;
    };
    union {
        short corpse;
        unsigned short corpseUnsigned;
    };
    short maxwaterdepth;
    short minwaterdepth;
    float energymake;
    float energyuse;
    float metalmake;
    float extractsMetal;
    float windgenerator;
    float tidalgenerator;
    float cloakcost;
    float cloakcostmoving;
    float energystorage;
    float metalstorage;
    int buildtime;
    Ref<WeaponDef> weapons[3];
    union {
        unsigned int maxHealth;
        short hp;
    };
    unsigned short buildRate;
    union {
        short healtime;
        unsigned short healtimeUnsigned;
    };
    short range;
    short radardistance;
    short sonardistance;
    short mincloakdistance;
    short radardistancejam;
    short sonardistancejam;
    short soundcategory;
    union {
        short buildangle;
        unsigned short buildangleUnsigned;
    };
    unsigned short buildRange;
    union {
        short maneuverleashlength;
        unsigned short maneuverleashUnsigned;
    };
    unsigned short attackrunlength;
    union {
        short kamikazedistance;
        unsigned short kamikazeUnsigned;
    };
    short sortbias;
    short altitude;
    unsigned short id;
    Ref<WeaponDef> explodeas;
    Ref<WeaponDef> selfdestructas;
    unsigned char maxslope;
    unsigned char maxwaterslope;
    unsigned char capacity;
    char transportcapacity;
    unsigned char draft;
    unsigned char makesMetal;
    unsigned char buildMenuPageCount;
    unsigned char mobile;
    char defaultmissiontype;
    unsigned int* weaponCategories[3];
    unsigned int* categories;
    union {
        unsigned int flags1;
        unsigned char flags1Byte;
        struct {
            unsigned int b00 : 2;
            unsigned int b02 : 2;
            unsigned int initCloaked : 1;
            unsigned int downloadable : 1;
            unsigned int canbuild : 1;
            unsigned int b07 : 1;
            unsigned int b08 : 1;
            unsigned int b09 : 1;
            unsigned int b10 : 1;
            unsigned int flying : 1;
            unsigned int b12 : 1;
            unsigned int b13 : 1;
            unsigned int b14 : 1;
            unsigned int b15 : 1;
            unsigned int b16 : 1;
            unsigned int b17 : 1;
            unsigned int b18 : 1;
            unsigned int b19 : 1;
            unsigned int b20 : 1;
            unsigned int b21 : 1;
            unsigned int b22 : 1;
            unsigned int b23 : 1;
            unsigned int b24 : 1;
            unsigned int b25 : 1;
            unsigned int b26 : 1;
            unsigned int b27 : 1;
            unsigned int b28 : 1;
            unsigned int b29 : 1;
            unsigned int b30 : 1;
            unsigned int gui : 1;
        };
    };
    union {
        unsigned int flags2;
        unsigned char flags2Byte;
        struct {
            unsigned int c00 : 1;
            unsigned int c01 : 1;
            unsigned int c02 : 1;
            unsigned int c03 : 1;
            unsigned int c04 : 1;
            unsigned int c05 : 1;
            unsigned int c06 : 1;
            unsigned int c07 : 1;
            unsigned int c08 : 1;
            unsigned int c09 : 1;
            unsigned int c10 : 1;
            unsigned int c11 : 1;
            unsigned int capture : 1;
            unsigned int c13 : 1;
            unsigned int c14 : 1;
            unsigned int norestrict : 1;
            unsigned int wacky : 1;
            unsigned int c17 : 1;
            unsigned int c18 : 1;
            unsigned int c19 : 1;
            unsigned int selfdestructcountdown : 3;
        };
    };

    UnitDef& operator=(const UnitDef& src);
    void AddToCategories(char* category);
    int HasField1ce() { return extractsMetal != 0.0f; }
    float EnergyCost() { return (float)(energyCost * 30); }
    float MetalCost() { return (float)(metalCost * 30); }
    unsigned int MaxHealth() { return maxHealth; }
};

#pragma pack(pop)

#endif
