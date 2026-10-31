#ifndef RADAR_TYPES_H
#define RADAR_TYPES_H

#pragma pack(push, 1)

struct MapTileSet {
    int count;
    unsigned char (*cell)[32][32];
};

struct WeaponDef;

struct InterceptorDefFlags {
    unsigned int :29;
    unsigned int bit29 : 1;
    unsigned int :2;
};

#pragma pack(pop)

#endif
