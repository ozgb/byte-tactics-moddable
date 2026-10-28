#ifndef CREATE_UNIT_POS_H
#define CREATE_UNIT_POS_H

#pragma pack(push, 1)
union CreateUnitFixed {
    int i;
    struct {
        short frac;
        short whole;
    } h;
};

struct CreateUnitPos {
    CreateUnitFixed x;
    CreateUnitFixed y;
    CreateUnitFixed z;
};
#pragma pack(pop)

#endif
