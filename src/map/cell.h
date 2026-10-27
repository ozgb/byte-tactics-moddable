#ifndef CELL_H
#define CELL_H

#pragma pack(push, 1)

struct Cell {
    unsigned short unit;
    unsigned short unit2;
    unsigned char height;
    unsigned char high;
    unsigned char low;
    unsigned char metal;
    unsigned short feature;
    union {
        unsigned short spot;
        struct {
            unsigned char offsetY;
            unsigned char offsetX;
        };
    };
    union {
        unsigned char flags;
        struct {
            unsigned char flag0 : 1;
            unsigned char bits1 : 2;
            unsigned char owner : 4;
            unsigned char bit7 : 1;
        };
        struct {
            unsigned char plotLow : 3;
            unsigned char plotFeature : 5;
        };
        struct {
            unsigned char plotLow4 : 3;
            unsigned char plotFeature4 : 4;
        };
    };
};

#pragma pack(pop)

#endif
