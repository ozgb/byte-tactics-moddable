#ifndef GRID2_H
#define GRID2_H
#include "../../interfaces/ref.h"

#pragma pack(push, 1)
struct FogCell {
    union {
        struct {
            unsigned char lo;
            unsigned char hi;
        };
        struct {
            unsigned char level0;
            unsigned char level1;
        };
    };
};

struct Grid2 {
    Ref<FogCell> cells;
    unsigned int width;
    unsigned int height;
    int count;
    int widthFixed;
    int heightFixed;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Grid2) == 24, "Grid2");
#endif

#endif
