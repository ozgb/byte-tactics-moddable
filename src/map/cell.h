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
    unsigned char flags;
};

#pragma pack(pop)

#endif
