#ifndef ORDER_TARGET_TYPES_H
#define ORDER_TARGET_TYPES_H

#pragma pack(push, 1)
union CoordHalves {
    int fixed;
    short half[2];
};
#pragma pack(pop)

#endif
